/*
 * Copyright (c) 2008-2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lib/io.h>

#include <kern/err.h>
#include <ctype.h>
#include <kern/debug.h>
#include <assert.h>
#include <kern/list.h>
#include <string.h>
#include <lib/cbuf.h>
#include <arch/ops.h>
#include <platform.h>
#include <platform/debug.h>
#include <kernel/mutex.h>
#include <kernel/thread.h>
#include <kern/init.h>

/* routines for dealing with main console io */

#if WITH_LIB_SM
#define PRINT_LOCK_FLAGS SPIN_LOCK_FLAG_IRQ_FIQ
#else
#define PRINT_LOCK_FLAGS SPIN_LOCK_FLAG_INTERRUPTS
#endif

/*
 * The console is protected by two locks - print_mutex and print_spin_lock.
 * When printing from a context where interrupts are enabled, print_mutex is
 * acquired to guard the entire output, and then print_spin_lock is periodically
 * acquired and released.
 * When printing from a context where interrupts and disabled, print_spin_lock
 * is aquired to guard the entire output.
 * This setup means that, in general, writes to the console will complete
 * atomically. It is possible, however, for a context with disabled interrupts
 * to preempt a context with enabled interrupts. This reduces the chance a print
 * operation inside an interrupt handler will be blocked, at the cost of some
 * log corruption.
 */
static mutex_t print_mutex = MUTEX_INITIAL_VALUE(print_mutex);
static spin_lock_t print_spin_lock = 0;
static spin_lock_saved_state_t print_saved_state = 0;
static unsigned int lock_held_by = SMP_MAX_CPUS;
static struct list_node print_callbacks = LIST_INITIAL_VALUE(print_callbacks);


#if CONSOLE_HAS_INPUT_BUFFER
#ifndef CONSOLE_BUF_LEN
#define CONSOLE_BUF_LEN 256
#endif

/* global input circular buffer */
cbuf_t console_input_cbuf;
static uint8_t console_cbuf_buf[CONSOLE_BUF_LEN];
#endif // CONSOLE_HAS_INPUT_BUFFER

/* print lock must be held when invoking out, outs, outc */
static void out_count(const char *str, size_t len) {
    print_callback_t *cb;
    size_t i;

    /* print to any registered loggers */
    if (!list_is_empty(&print_callbacks)) {
        spin_lock_saved_state_t state;
        spin_lock_save(&print_spin_lock, &state, PRINT_LOCK_FLAGS);

        list_for_every_entry(&print_callbacks, cb, print_callback_t, entry) {
            if (cb->print)
                cb->print(cb, str, len);
        }

        spin_unlock_restore(&print_spin_lock, state, PRINT_LOCK_FLAGS);
    }

    /* write out the serial port */
    for (i = 0; i < len; i++) {
        platform_dputc(str[i]);
    }
}

/* Signal that the write operation is complete. */
static void out_commit(void)
{
    print_callback_t *cb;

    int need_lock = !arch_ints_disabled();
    spin_lock_saved_state_t state = 0;

    DEBUG_ASSERT(need_lock || lock_held_by == arch_curr_cpu_num());

    /* commit to any registered loggers */
    if (!list_is_empty(&print_callbacks)) {
        if (need_lock) {
            spin_lock_save(&print_spin_lock, &state, PRINT_LOCK_FLAGS);
        }
        list_for_every_entry(&print_callbacks, cb, print_callback_t, entry) {
            if (cb->commit) {
                cb->commit(cb);
            }
        }
        if (need_lock) {
            spin_unlock_restore(&print_spin_lock, state, PRINT_LOCK_FLAGS);
        }
    }
}

static void out_lock(void)
{
    if (arch_ints_disabled()) {
        /*
         * Even though interupts are disabled, FIQs may not be. So save and
         * restore the interrupt state like a normal spin lock. Due to how
         * lock/unlock gets called, we need to stash the state in a global.
         */
        spin_lock_saved_state_t state;
        spin_lock_save(&print_spin_lock, &state, PRINT_LOCK_FLAGS);
        print_saved_state = state;
        lock_held_by = arch_curr_cpu_num();
    } else {
        mutex_acquire(&print_mutex);
    }
}

static void out_unlock(void)
{
    if (arch_ints_disabled()) {
        DEBUG_ASSERT(lock_held_by == arch_curr_cpu_num());
        lock_held_by = SMP_MAX_CPUS;
        spin_unlock_restore(&print_spin_lock, print_saved_state,
                            PRINT_LOCK_FLAGS);
    } else {
        mutex_release(&print_mutex);
    }
}


void register_print_callback(print_callback_t *cb) {
    spin_lock_saved_state_t state;
    spin_lock_save(&print_spin_lock, &state, PRINT_LOCK_FLAGS);

    list_add_head(&print_callbacks, &cb->entry);

    spin_unlock_restore(&print_spin_lock, state, PRINT_LOCK_FLAGS);
}

void unregister_print_callback(print_callback_t *cb) {
    spin_lock_saved_state_t state;
    spin_lock_save(&print_spin_lock, &state, PRINT_LOCK_FLAGS);

    list_delete(&cb->entry);

    spin_unlock_restore(&print_spin_lock, state, PRINT_LOCK_FLAGS);
}

static ssize_t __debug_stdio_write(io_handle_t *io, const char *s, size_t len) {
    out_count(s, len);
    return len;
}

static void __debug_stdio_write_commit(io_handle_t *io)
{
    out_commit();
}

static void __debug_stdio_lock(io_handle_t *io)
{
    out_lock();
}

static void __debug_stdio_unlock(io_handle_t *io)
{
    out_unlock();
}

static ssize_t __debug_stdio_read(io_handle_t *io, char *s, size_t len) {
    if (len == 0)
        return 0;

#if CONSOLE_HAS_INPUT_BUFFER
    ssize_t err = cbuf_read(&console_input_cbuf, s, len, true);
    return err;
#else
    int err = platform_dgetc(s, true);
    if (err < 0)
        return err;

    return 1;
#endif
}

#if CONSOLE_HAS_INPUT_BUFFER
static void console_init_hook(uint level) {
    cbuf_initialize_etc(&console_input_cbuf, sizeof(console_cbuf_buf), console_cbuf_buf);
}

INIT_HOOK(console, console_init_hook, INIT_LEVEL_PLATFORM_EARLY - 1);
#endif

/* global console io handle */
static const io_handle_hooks_t console_io_hooks = {
    .write  = __debug_stdio_write,
    .read   = __debug_stdio_read,
    .lock          = __debug_stdio_lock,
    .unlock        = __debug_stdio_unlock,    
    .write_commit  = __debug_stdio_write_commit,    
};

io_handle_t console_io = IO_HANDLE_INITIAL_VALUE(&console_io_hooks);


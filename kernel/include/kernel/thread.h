/*
 * Copyright (c) 2008-2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <arch/defines.h>
#include <arch/ops.h>
#include <arch/arch_ops.h>
#include <kern/compiler.h>
#include <kernel/spinlock.h>
#include <kernel/thread_defines.h>

__BEGIN_CDECLS

#if WITH_SMP
#define thread_curr_cpu(t) ((t)->curr_cpu)
#define thread_pinned_cpu(t) ((t)->pinned_cpu)
#define thread_set_curr_cpu(t,c) ((t)->curr_cpu = (c))
#define thread_set_pinned_cpu(t, c) ((t)->pinned_cpu = (c))
#else
#define thread_curr_cpu(t) (0)
#define thread_pinned_cpu(t) (-1)
#define thread_set_curr_cpu(t,c) do {} while(0)
#define thread_set_pinned_cpu(t, c) do {} while(0)
#endif

/* thread priority */
#define NUM_PRIORITIES 32
#define LOWEST_PRIORITY 0
#define HIGHEST_PRIORITY (NUM_PRIORITIES - 1)
#define DPC_PRIORITY (NUM_PRIORITIES - 2)
#define IDLE_PRIORITY LOWEST_PRIORITY
#define LOW_PRIORITY (NUM_PRIORITIES / 4)
#define DEFAULT_PRIORITY (NUM_PRIORITIES / 2)
#define HIGH_PRIORITY ((NUM_PRIORITIES / 4) * 3)

/* stack size */
#ifdef CUSTOM_DEFAULT_STACK_SIZE
#define DEFAULT_STACK_SIZE CUSTOM_DEFAULT_STACK_SIZE
#else
#define DEFAULT_STACK_SIZE ARCH_DEFAULT_STACK_SIZE
#endif

/* functions */
void thread_init_early(void);
void thread_init(void);
void thread_become_idle(void) __NO_RETURN;
void thread_secondary_cpu_init_early(void);
void thread_secondary_cpu_entry(void) __NO_RETURN;
void thread_set_name(const char *name);
void thread_set_priority(int priority);
thread_t *thread_create(const char *name, thread_start_routine entry, void *arg, int priority, size_t stack_size);
thread_t *thread_create_etc(thread_t *t, const char *name, thread_start_routine entry, void *arg, int priority, void *stack, size_t stack_size);
status_t thread_resume(thread_t *);
void thread_exit(int retcode) __NO_RETURN;
void thread_sleep(sys_time_t delay);
status_t thread_detach(thread_t *t);
status_t thread_join(thread_t *t, int *retcode, sys_time_t timeout);
status_t thread_detach_and_resume(thread_t *t);
status_t thread_set_real_time(thread_t *t);

void dump_thread(thread_t *t);
void arch_dump_thread(thread_t *t);
void dump_all_threads(void);
void dump_all_threads_unlocked(void);
void dump_threads_stats(void);

/* scheduler routines */
void thread_yield(void); /* give up the cpu voluntarily */
void thread_preempt(void); /* get preempted (inserted into head of run queue) */
void thread_block(void); /* block on something and reschedule */
void thread_unblock(thread_t *t, bool resched); /* go back in the run queue */


/* called on every timer tick for the scheduler to do quantum expiration */
struct timer;
enum handler_return thread_timer_tick(struct timer *, sys_time_t now, void *arg);

/* the current thread */
static inline thread_t *get_current_thread(void) {
    return arch_get_current_thread();
}

static inline void set_current_thread(thread_t *t) {
    arch_set_current_thread(t);
}

/* scheduler lock */
extern spin_lock_t thread_lock;

#define THREAD_LOCK(state) spin_lock_saved_state_t state; spin_lock_irqsave(&thread_lock, state)
#define THREAD_UNLOCK(state) spin_unlock_irqrestore(&thread_lock, state)

static inline bool thread_lock_held(void) {
    return spin_lock_held(&thread_lock);
}

/* thread local storage */
static inline __ALWAYS_INLINE uintptr_t tls_get(uint entry) {
    return get_current_thread()->tls[entry];
}

static inline __ALWAYS_INLINE uintptr_t __tls_set(uint entry, uintptr_t val) {
    uintptr_t oldval = get_current_thread()->tls[entry];
    get_current_thread()->tls[entry] = val;
    return oldval;
}

#define tls_set(e,v) \
    ({ \
        STATIC_ASSERT((e) < MAX_TLS_ENTRY); \
        __tls_set(e, v); \
    })

/* thread level statistics */
#if THREAD_STATS
struct thread_stats {
    sys_bigtime_t idle_time;
    sys_bigtime_t last_idle_timestamp;
    ulong reschedules;
    ulong context_switches;
    ulong preempts;
    ulong yields;
    ulong interrupts; /* platform code increment this */
    ulong timer_ints; /* timer code increment this */
    ulong timers; /* timer code increment this */

#if WITH_SMP
    ulong reschedule_ipis;
#endif
};

extern struct thread_stats thread_stats[SMP_MAX_CPUS];

#define THREAD_STATS_INC(name) do { thread_stats[arch_curr_cpu_num()].name++; } while(0)

#else

#define THREAD_STATS_INC(name) do { } while (0)

#endif

#define CPU_CONTEXT_NUM  2

typedef struct world {
    struct pcb_user_ctx arch;
} world_t; 

extern world_t world_server[SMP_MAX_CPUS][CPU_CONTEXT_NUM];

__END_CDECLS

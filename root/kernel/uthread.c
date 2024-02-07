/*
 * Copyright (c) 2013, Google Inc. All rights reserved
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <root/uthread.h>
#include <stdlib.h>
#include <string.h>
#include <kern/compiler.h>
#include <assert.h>
#include <kern/init.h>
#include <kern/trace.h>
#include <kernel/mutex.h>

#ifdef WITH_HYPER_MODE
#include <arch/vcpu.h>
#endif

/* Global list of all userspace threads */
static struct list_node uthread_list;

/* Monotonically increasing thread id for now */
static uint32_t next_utid;
static spin_lock_t uthread_lock;


/* TODO: implement a utid hashmap */
static uint32_t uthread_alloc_utid(void)
{
	spin_lock_saved_state_t state;

	spin_lock_save(&uthread_lock, &state, SPIN_LOCK_FLAG_INTERRUPTS);
	next_utid++;
	spin_unlock_restore(&uthread_lock, state, SPIN_LOCK_FLAG_INTERRUPTS);

	return next_utid;
}

/* TODO: */
static void uthread_free_utid(uint32_t utid)
{
}

#define MAX_USR_VA	USER_ASPACE_SIZE

extern user_addr_t thread_write_elf_tables(void* thread, user_addr_t stack_ptr);

void arch_uthread_startup(void)
{
#ifdef USER_32BIT
	bool is_32bit_uspace = true;
#else
	bool is_32bit_uspace = false;
#endif
	struct uthread *ut = (struct uthread *) tls_get(TLS_ENTRY_UTHREAD);
	struct thread *ct = ut->thread;
	user_addr_t elf_tables = thread_write_elf_tables(ut->app, ut->start_stack);

    	vaddr_t kernel_stack_top = (uintptr_t)ct->stack + ct->stack_size;
    	kernel_stack_top = ROUNDDOWN(kernel_stack_top, 16);
	vaddr_t sp_usr  = ROUNDDOWN(ut->start_stack, is_32bit_uspace ? 8 : 16);
	vaddr_t entry  = ut->entry;

	vmm_set_active_aspace(ct->aspace);

	arch_enter_uspace(entry, sp_usr, kernel_stack_top, elf_tables);
}

uthread_t *uthread_create(const char *name, vaddr_t entry, int priority, vaddr_t start_stack, size_t stack_size, void *private_data)
{
	uthread_t *ut = NULL;
	status_t err;
	vaddr_t stack_bot;
	spin_lock_saved_state_t state;

	ut = (uthread_t *)calloc(1, sizeof(uthread_t));
	if (!ut)
		goto err_done;

	ut->id = uthread_alloc_utid(); //asid
	ut->private_data = private_data;
	ut->entry = entry;
	ut->ns_va_bottom = MAX_USR_VA;

	ut->thread = thread_create(name, (thread_start_routine)arch_uthread_startup, NULL, priority, DEFAULT_STACK_SIZE);
	if (!ut->thread)
		goto err_free_ut;

	err = vmm_create_aspace(&ut->thread->aspace, name, 0);
	if (err)
		goto err_free_ut;
	/* Allocate and map in a stack region */
	stack_bot = start_stack - stack_size;
	u_int flags = ARCH_MMU_FLAG_CACHED | ARCH_MMU_FLAG_PERM_USER | ARCH_MMU_FLAG_PERM_NO_EXECUTE | ARCH_MMU_FLAG_NS;
	err = vmm_alloc_contiguous(ut->thread->aspace, "stack", stack_size, &stack_bot, 0, VMM_FLAG_VALLOC_SPECIFIC, flags);
	if (err) 
		goto err_free_ut;

	ut->start_stack = start_stack;
	ut->stack_size = stack_size;

	/* store user thread struct into TLS slot 0 */
	ut->thread->tls[TLS_ENTRY_UTHREAD] = (uintptr_t) ut;

#ifdef WITH_HYPER_MODE
	ut->thread->flags |= THREAD_FLAG_VM;
    	vcpu_init(&ut->thread->arch);
#endif

	/* Put it in global uthread list */
	spin_lock_save(&uthread_lock, &state, SPIN_LOCK_FLAG_INTERRUPTS);
	list_add_head(&uthread_list, &ut->uthread_list_node);
	spin_unlock_restore(&uthread_lock, state, SPIN_LOCK_FLAG_INTERRUPTS);

	return ut;

err_free_ut:
	uthread_free_utid(ut->id);
	free(ut);
err_done:
	return NULL;
}

status_t uthread_start(uthread_t *ut)
{
	if (!ut || !ut->thread)
		return ERR_INVALID_ARGS;

	return thread_resume(ut->thread);
}

void __NO_RETURN uthread_exit(int retcode)
{
	uthread_t *ut;

	ut = uthread_get_current();
	if (ut) {
		vmm_free_region(ut->thread->aspace, ut->start_stack - ut->stack_size);
		vmm_free_aspace(ut->thread->aspace);
		uthread_free_utid(ut->id);
		free(ut);
	} else {
		TRACEF("WARNING: unexpected call on kernel thread %s!",
				get_current_thread()->name);
	}

	thread_exit(retcode);
}

static void uthread_init(uint level)
{
	list_initialize(&uthread_list);
//	arch_uthread_init();
}

/* this has to come up early because we have to reinitialize the MMU on
 * some arch's
 */
INIT_HOOK(libuthread, uthread_init, INIT_LEVEL_ARCH_EARLY);

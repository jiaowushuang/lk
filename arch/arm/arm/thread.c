/*
 * Copyright (c) 2008-2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <kern/debug.h>
#include <kern/trace.h>
#include <kernel/thread.h>
#include <arch/arm.h>


#define LOCAL_TRACE 0


extern void arm_context_switch(addr_t *old_sp, addr_t new_sp);

static void initial_thread_func(void) __NO_RETURN;
static void initial_thread_func(void) {
    int ret;

//  dprintf("initial_thread_func: thread %p calling %p with arg %p\n", current_thread, current_thread->entry, current_thread->arg);
//  dump_thread(current_thread);

    /* release the thread lock that was implicitly held across the reschedule */
    spin_unlock(&thread_lock);
    arch_enable_ints();

    thread_t *ct = get_current_thread();
#if defined(ARCH_HAS_MPU) && defined(CONFIG_MULTIPARTITIONING)
    z_arm_configure_dynamic_mpu_regions(ct);
#endif    
    ret = ct->entry(ct->arg);

//  dprintf("initial_thread_func: thread %p exiting with %d\n", current_thread, ret);

    thread_exit(ret);
}

void arch_thread_initialize(thread_t *t) {
    // create a default stack frame on the stack
    vaddr_t stack_top = (vaddr_t)t->stack + t->stack_size;

    // make sure the top of the stack is 8 byte aligned for EABI compliance
    stack_top = ROUNDDOWN(stack_top, 8);

    struct context_switch_frame *frame = (struct context_switch_frame *)(stack_top);
    frame--;

    // fill it in
    memset(frame, 0, sizeof(*frame));
    frame->lr = (vaddr_t)&initial_thread_func;

    // set the stack pointer
    t->arch.tcpu_sp = (vaddr_t)frame;

#if ARM_WITH_VFP
    arm_fpu_thread_initialize(t);
#endif

}

void arch_context_switch(thread_t *oldthread, thread_t *newthread) {
LTRACEF("arch_context_switch: cpu %u old %p (%s), new %p (%s)\n", arch_curr_cpu_num(), oldthread, oldthread->name, newthread, newthread->name);
#if ARM_WITH_VFP
    arm_fpu_thread_swap(oldthread, newthread);
#endif

    arm_context_switch(&oldthread->arch.tcpu_sp, newthread->arch.tcpu_sp);
    /* CONTINUE EXE */
}

void arch_dump_thread(thread_t *t) {
    if (t->state != THREAD_RUNNING) {
        dprintf(INFO, "\tarch: ");
        dprintf(INFO, "tcpu_sp 0x%lx\n", t->arch.tcpu_sp);
    }
}


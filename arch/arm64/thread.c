/*
 * Copyright (c) 2008 Travis Geiselbrecht
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
#include <arch/arm64.h>
#include <arch/cpu_data.h>

#define LOCAL_TRACE 0

/*
 * when caller(current) function is switched, 
 * the callee-saved registers of callee function is temporary, 
 * so the regisers need be stored the caller stack frame
 */

/*
 * 
 * ...
 * t->stack+t->stack_size == stack_top(t->frame_end)
 * ...
 * t->frame_start
 * t->stack
 */

static void dump_calleeframe(const struct context_switch_frame *iframe) {
    LTRACEF("iframe %p:\n", iframe);
    LTRACEF("lr 0x%16lx\n", iframe->r[0]);
    LTRACEF("sp_el0 0x%16lx\n", iframe->r[1]);    
    LTRACEF("x18 0x%16lx x19 0x%16lx x20 0x%16lx x21 0x%16lx\n", iframe->r[2], iframe->r[3], iframe->r[4], iframe->r[5]);
    LTRACEF("x22 0x%16lx x23 0x%16lx x24 0x%16lx x25 0x%16lx\n", iframe->r[6], iframe->r[7], iframe->r[8], iframe->r[9]);
    LTRACEF("x26 0x%16lx x27 0x%16lx x28 0x%16lx x29 0x%16lx\n", iframe->r[10], iframe->r[11], iframe->r[12], iframe->r[13]);
}

extern void arm64_context_switch(addr_t *old_sp, addr_t new_sp);
extern void arm64_sysregs_switch(vaddr_t old_sysregs, vaddr_t new_sysregs);
static void initial_thread_func(void) __NO_RETURN;
static void initial_thread_func(void) {
    int ret;

    thread_t *current_thread = get_current_thread();

    LTRACEF("initial_thread_func: thread %p calling %p with arg %p\n", 
        current_thread, current_thread->entry, current_thread->arg);

    /* release the thread lock that was implicitly held across the reschedule */
    spin_unlock(&thread_lock);
    arch_enable_ints();
#if defined (ARCH_HAS_MPU) && defined(CONFIG_MULTIPARTITIONING)
    z_arm64_thread_mem_domains_init(current_thread);
#endif
    ret = current_thread->entry(current_thread->arg);

    LTRACEF("initial_thread_func: kernel thread %p exiting with %d\n", current_thread, ret);

    thread_exit(ret);
}

void arch_thread_initialize(thread_t *t) {
    // create a default stack frame on the stack
    vaddr_t stack_top = (vaddr_t)t->stack + t->stack_size;

    // make sure the top of the stack is 16 byte aligned for EABI compliance
    stack_top = ROUNDDOWN(stack_top, 16);

    // tcpu_context_t *frame = (tcpu_context_t *)(stack_top);
    struct context_switch_frame *frame = (struct context_switch_frame *)(stack_top);
    frame--;

    // fill it in
    memset(frame, 0, sizeof(*frame));
    // gp_regs_t *ctx = get_gpregs_ctx(frame);
    // write_ctx_reg(ctx, CTX_GPREG_LR, (vaddr_t)&initial_thread_func); // func ptr ???
    frame->r[0] = (vaddr_t)&initial_thread_func;

    // set the stack pointer
    t->arch.tcpu_sp = (vaddr_t)frame;

    LTRACEF("sp %lx, stack %p, top %lx, frame %p, lr=pc %p\n", 
        t->arch.tcpu_sp, t->stack, stack_top, frame, &initial_thread_func);
}

void arch_context_switch(thread_t *oldthread, thread_t *newthread) {
    LTRACEF("old %p (%s), new %p (%s)\n", oldthread, oldthread->name, newthread, newthread->name);
    LTRACEF("old sp %lx, new sp %lx\n", oldthread->arch.tcpu_sp, newthread->arch.tcpu_sp);
    dump_calleeframe((struct context_switch_frame *)newthread->arch.tcpu_sp);

    arm64_fpu_pre_context_switch(oldthread);
#if WITH_SMP
    DSB; /* broadcast tlb operations in case the thread moves to another cpu */
#endif
    arm64_sysregs_switch((uintptr_t)&oldthread->arch.pcpu, 
        (uintptr_t)&newthread->arch.pcpu);
    arm64_context_switch(&oldthread->arch.tcpu_sp, newthread->arch.tcpu_sp);
    /* CONTINUE EXE */
}

void arch_dump_thread(thread_t *t) {
    if (t->state != THREAD_RUNNING) {
        dprintf(INFO, "\tarch: ");
        dprintf(INFO, "sp 0x%lx\n", t->arch.tcpu_sp);
    }
}

/*
 * Copyright (c) 2012-2015 Travis Geiselbrecht
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
#include <assert.h>
#include <kernel/thread.h>
#include <arch/arm.h>
#include <arch/arm/cm.h>
#include <arch/arch_thread.h>

#define LOCAL_TRACE 0


/* macros for saving and restoring a context switch frame, depending on what version of
 * the architecture you are */
#if  (__CORTEX_M >= 0x03)

/* cortex-m3 and above (armv7-m) */
#define SAVE_REGS       "push   { r4-r11, lr };" /* note: saves 9 words */
#define RESTORE_REGS    "pop    { r4-r11, lr };"
#define RESTORE_REGS_PC "pop    { r4-r11, pc };"
#define SAVE_SP(basereg, tempreg, offset) "str   sp, [" #basereg "," #offset "];"
#define LOAD_SP(basereg, tempreg, offset) "ldr   sp, [" #basereg "," #offset "];"
#define CLREX           "clrex;"

#else

/* cortex-m0 and cortex-m0+ (armv6-m) */
#define SAVE_REGS \
        "push   { r4-r7, lr };" \
        "mov    r4, r8;" \
        "mov    r5, r9;" \
        "mov    r6, r10;" \
        "mov    r7, r11;" \
        "push   { r4-r7 };" /* note: saves 9 words */
#define RESTORE_REGS \
        "pop    { r4-r7 };" \
        "mov    r8 , r4;" \
        "mov    r9 , r5;" \
        "mov    r10, r6;" \
        "mov    r11, r7;" \
        "pop    { r4-r7 };" \
        "pop    { r0 };" \
        "mov    lr, r0;" /* NOTE: trashes r0 */
#define RESTORE_REGS_PC \
        "pop    { r4-r7 };" \
        "mov    r8 , r4;" \
        "mov    r9 , r5;" \
        "mov    r10, r6;" \
        "mov    r11, r7;" \
        "pop    { r4-r7, pc };"
#define SAVE_SP(basereg, tempreg, offset) \
        "mov    " #tempreg ", sp;" \
        "str    " #tempreg ", [" #basereg "," #offset "];"
#define LOAD_SP(basereg, tempreg, offset) \
        "ldr    " #tempreg ", [" #basereg "," #offset "];" \
        "mov    sp, " #tempreg ";"

/* there is no clrex on armv6m devices */
#define CLREX           ""

#endif

/* since we're implicitly uniprocessor, store a pointer to the current thread here */
thread_t *_current_thread;

static void initial_thread_func(void) __NO_RETURN;
static void initial_thread_func(void) {
    int ret;

    LTRACEF("thread %p calling %p with arg %p\n", _current_thread, _current_thread->entry, _current_thread->arg);
#if LOCAL_TRACE
    dump_thread(_current_thread);
#endif

    /* release the thread lock that was implicitly held across the reschedule */
    spin_unlock(&thread_lock);
    arch_enable_ints();

#if defined(ARCH_HAS_MPU) && defined(CONFIG_MULTIPARTITIONING)
    z_arm_configure_dynamic_mpu_regions(_current_thread);
#endif

    ret = _current_thread->entry(_current_thread->arg);

    LTRACEF("thread %p exiting with %d\n", _current_thread, ret);

    thread_exit(ret);
}

void arch_thread_initialize(struct thread *t) {
    LTRACEF("thread %p, stack %p\n", t, t->stack);

    /* find the top of the stack and align it on an 8 byte boundary */
    uint32_t *sp = (void *)ROUNDDOWN((vaddr_t)t->stack + t->stack_size, 8);

    struct context_switch_frame *frame = (void *)sp;
    frame--;

    /* arrange for lr to point to our starting routine */
    frame->lr = (uint32_t)&initial_thread_func;

    t->arch.tcpu_sp = (addr_t)frame;
    t->arch.was_preempted = false;

#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    /* zero the fpu register state */
    memset(t->arch.fpregs, 0, sizeof(t->arch.fpregs));
    t->arch.fpused = false;
#endif
}

static volatile struct arm_cm_exception_frame_long *preempt_frame;
extern unsigned int *app_info_header;

static void pendsv(struct arm_cm_exception_frame_long *frame) {
    arch_disable_ints();

    /* make sure the stack is 8 byte aligned */
    DEBUG_ASSERT(((uintptr_t)__GET_FRAME() & 0x7) == 0);

    DEBUG_ASSERT_MSG(!spin_lock_held(&thread_lock),
        "PENDSV: thread lock was held when preempted! pc %#x\n", frame->pc);

    LTRACEF("preempting thread %p (%s)\n", _current_thread, _current_thread->name);

    /* save the iframe the pendsv fired on and hit the preemption code */
    preempt_frame = frame;
    thread_preempt();

    LTRACEF("fell through\n");

    /* if we got here, there wasn't anything to switch to, so just fall through and exit */
    preempt_frame = NULL;

    DEBUG_ASSERT(!spin_lock_held(&thread_lock));

    arch_enable_ints();
}

/*
 * raw pendsv exception handler, triggered by interrupt glue to schedule
 * a preemption check.
 */
__NAKED void _pendsv(void) {
    __asm__ volatile(
        SAVE_REGS
        "mov    r0, sp;"
        "sub    sp, #4;" /* adjust the stack to be 8 byte aligned */
        "bl     %c0;"
        "add    sp, #4;"
        RESTORE_REGS_PC
        :: "i" (pendsv)
    );
    __UNREACHABLE;
}

void arch_context_enter_pls(struct thread *partthx, void *newthx);
static void svc_handler(struct arm_cm_exception_frame_short *frame) {
    uint8_t svc_number;
    uint32_t frame_pc;
    /* The stack contains: r0, r1, r2, r3, r12, LR, PC and xPSR.  The first
     * argument (r0) is pulParam[ 0 ]. */
    frame_pc = frame->pc;
    svc_number = ((uint8_t *)frame_pc)[-2];

    switch (svc_number) {
    case 0:
            __asm__ volatile(
                /* load the pointer to the original exception frame we want to restore */
                "mov    sp, r4;"
                RESTORE_REGS_PC
            );
		    break;
    case 1: {
        arch_disable_ints();
	    void *(*curr_thx_func)(void) = app_info_header[6];
	    void *curr_thx = curr_thx_func();
	    struct thread *curr = get_current_thread();
	    LTRACEF("%lx, %lx\n", curr_thx_func, curr_thx);
	    arch_context_enter_pls(curr, curr_thx);
	    break;
    }
	}
}
/*
 * svc handler, used to hard switch the cpu into exception mode to return
 * to preempted thread.
 */
__NAKED void _svc(void) {
	__asm volatile(
        "tst lr, #4                    \n"
        "ite eq                        \n"
        "mrseq r0, msp                 \n"
        "mrsne r0, psp                 \n"
        "b %0                          \n"
        ::"i"(svc_handler) : "r0", "memory");
}

#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
__NAKED static void _half_save_and_svc(struct thread *oldthread, struct thread *newthread, bool fpu_save, bool restore_fpu)
#else
__NAKED static void _half_save_and_svc(struct thread *oldthread, struct thread *newthread)
#endif
{
    __asm__ volatile(
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
        /* see if we need to save fpu context */
        "tst    r2, #1;"
        "beq    0f;"

        /* save part of the fpu context on the stack */
        "vmrs   r2, fpscr;"
        "push   { r2 };"
        "vpush  { s0-s15 };"

        /* save the top regs into the thread struct */
        "add    r2, r0, %[fp_off];"
        "vstm   r2, { s16-s31 };"

        "0:"
#endif

        /* save regular context */
        SAVE_REGS
        SAVE_SP(r0, r2, %[sp_off])

        /* restore the new thread's stack pointer, but not the integer state (yet) */
        LOAD_SP(r1, r2, %[sp_off])

#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
        /* see if we need to restore fpu context */
        "tst    r3, #1;"
        "beq    0f;"

        /* restore the top part of the fpu context */
        "add    r3, r1, %[fp_off];"
        "vldm   r3, { s16-s31 };"

        /* restore the bottom part of the context, stored up the frame a little bit */
        "add    r3, sp, %[fp_exc_off];"
        "vldm   r3!, { s0-s15 };"
        "ldr    r3, [r3];"
        "vmsr   fpscr, r3;"
        "b      1f;"

        /* disable fpu context if we're not restoring anything */
        "0:"
        "mrs    r3, CONTROL;"
        "bic    r3, #(1<<2);" /* unset FPCA */
        "msr    CONTROL, r3;"
        "isb;"

        "1:"
#endif

        CLREX
        "cpsie  i;"

        /* make a svc call to get us into handler mode.
         * use r4 as an arg, since r0 is saved on the stack for the svc */
        "mov    r4, sp;"
        "svc    #0;"
        ::  [sp_off] "i"(offsetof(thread_t, arch.tcpu_sp))
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
        ,[fp_off] "i"(offsetof(thread_t, arch.fpregs))
        ,[fp_exc_off] "i"(sizeof(struct arm_cm_exception_frame_long))
#endif
    );
}

/* simple scenario where the to and from thread yielded */
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
__NAKED static void _arch_non_preempt_context_switch(struct thread *oldthread, struct thread *newthread, bool save_fpu, bool restore_fpu)
#else
__NAKED static void _arch_non_preempt_context_switch(struct thread *oldthread, struct thread *newthread)
#endif
{
    __asm__ volatile(
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
        /* see if we need to save fpu context */
        "tst    r2, #1;"
        "beq    0f;"

        /* save part of the fpu context on the stack */
        "vmrs   r2, fpscr;"
        "push   { r2 };"
        "vpush  { s0-s15 };"

        /* save the top regs into the thread struct */
        "add    r2, r0, %[fp_off];"
        "vstm   r2, { s16-s31 };"

        "0:"
#endif

        /* save regular context */
        SAVE_REGS
        SAVE_SP(r0, r2, %[sp_off])

        /* restore new context */
        LOAD_SP(r1, r2, %[sp_off])
        RESTORE_REGS

#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
        /* see if we need to restore fpu context */
        "tst    r3, #1;"
        "beq    0f;"

        /* restore fpu context */
        "add    r3, r1, %[fp_off];"
        "vldm   r3, { s16-s31 };"

        "vpop   { s0-s15 };"
        "pop    { r3 };"
        "vmsr   fpscr, r3;"
        "b      1f;"

        /* disable fpu context if we're not restoring anything */
        "0:"
        "mrs    r3, CONTROL;"
        "bic    r3, #(1<<2);" /* unset FPCA */
        "msr    CONTROL, r3;"
        "isb;"

        "1:"
#endif

        CLREX
        "bx     lr;"
        ::  [sp_off] "i"(offsetof(thread_t, arch.tcpu_sp))
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
        , [fp_off] "i"(offsetof(thread_t, arch.fpregs))
#endif
    );
}

__NAKED static void _thread_mode_bounce(bool fpused) {
	__asm__ volatile(
		/* restore main context */
		RESTORE_REGS

#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
		/* see if we need to restore fpu context */
		"tst    r0, #1;"
		"beq    0f;"

		/* restore fpu context */
		"vpop   { s0-s15 };"
		"pop    { r0 };"
		"vmsr   fpscr, r0;"
		"b      1f;"

		/* disable fpu context if we're not restoring anything */
		"0:"
		"mrs    r3, CONTROL;"
		"bic    r3, #(1<<2);" /* unset FPCA */
		"msr    CONTROL, r3;"
		"isb;"

		"1:"
#endif

		"bx     lr;");
    __UNREACHABLE;
}

/*
 * The raw context switch routine. Called by the scheduler when it decides to switch.
 * Called either in the context of a thread yielding or blocking (interrupts disabled,
 * on the system stack), or inside the pendsv handler on a thread that is being preempted
 * (interrupts disabled, in handler mode). If preempt_frame is set the thread
 * is being preempted.
 */
/* 这里面主要区分两种情况，一个是由pendsv触发的上下文切换，称为抢占型；另一种是调用任务相关函数，例如yield,block等
 * 来主动放弃CPU，称为合作型；由于存在新和旧两个任务，所以有四种上下文切换组合
 * 对于抢占型任务，保存和恢复的上下文存在异常栈内，即Main Stack；而对于合作型任务，保存和恢复的上下文存在tcpu_sp，
 * 即任务自己的栈区
 * 初始情况下,A/B...都是合作的
 * 1. A抢占->B抢占： 二者都是被中断打断的
 * 2. A抢占->B合作： A是被中断打断，B是自动放弃的
 * 3. A合作->B抢占： ...
 * 4. A合作->B合作： ...
 * https://www.cnblogs.com/utank/p/11263073.html
 * https://zhuanlan.zhihu.com/p/639230009
 * 
 * 
 */
static uint16_t pls_num;

static bool arch_is_new_pls(struct thread *partthx, void *thx)
{
	for (int i = 0; i < pls_num; i++) {
        if (partthx->pls[i] == POINTER_TO_UINT(thx))
		    return false;
    }
    return true;
}

void arch_context_enter_pls(struct thread *partthx, void *newthx)
{
	addr_t tcpu_sp = *((addr_t *)newthx);
	struct arm_cm_exception_frame_short *frame = ((addr_t *)tcpu_sp);
    struct arm_cm_exception_frame_long *exc_frame = ((addr_t *)tcpu_sp);

	LTRACEF("sp %lx - %lx, r0 %lx, r1 %lx, r2 %lx, r3 %lx, r12 %lx, lr %lx, pc %lx, psr %lx\n", 
        exc_frame, tcpu_sp, exc_frame->r0, exc_frame->r1, exc_frame->r2, exc_frame->r3,
        exc_frame->r12, exc_frame->exc_lr, exc_frame->pc, exc_frame->psr);
    
	frame--;

	frame->pc = (uint32_t)&_thread_mode_bounce;
	// frame->psr = (1 << 24); /* thread bit set, IPSR 0 */
	// frame->r0 = frame->r1 = frame->r2 = frame->r3 = frame->r12 = frame->lr = 0;
	frame->r0 = exc_frame->r0;
    frame->r1 = exc_frame->r1;
    frame->r2 = exc_frame->r2;
    frame->r3 = exc_frame->r3;
    frame->r12 = exc_frame->r12;
    frame->lr = exc_frame->exc_lr;
    frame->psr = exc_frame->psr;

    partthx->pls[pls_num++] = POINTER_TO_UINT(newthx);
    LTRACEF("iretting to user space\n");

    arch_enable_ints();
	__asm__ volatile(
        CLREX 
        "mov    sp, %0;"
		"bx     %1;" 
        ::"r"(frame),
		"r"(0xfffffff9));
	__UNREACHABLE;
}

void arch_context_switch_pls(struct thread *partthx, void* oldthx, void* newthx) {
    DEBUG_ASSERT(spin_lock_held(&thread_lock));

    *((addr_t *)oldthx) =  (addr_t)preempt_frame;
    preempt_frame = NULL;
    arch_spin_unlock(&thread_lock); 
    
    if (arch_is_new_pls(partthx, newthx)) {
	    arch_context_enter_pls(partthx, newthx);
    } else {
        LTRACEF("switch to user space\n");
        __asm__ volatile(
            "mov    sp, %0;"
            "cpsie  i;"
            CLREX
            RESTORE_REGS_PC
            :: "r"(*((addr_t *)newthx))
        );
        __UNREACHABLE;
    }
}

void arch_context_switch(struct thread *oldthread, struct thread *newthread) {
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    LTRACEF("FPCCR.LSPACT %lu, FPCAR 0x%x, CONTROL.FPCA %lu\n",
            FPU->FPCCR & FPU_FPCCR_LSPACT_Msk, FPU->FPCAR, __get_CONTROL() & CONTROL_FPCA_Msk);
#endif

    DEBUG_ASSERT(spin_lock_held(&thread_lock));

    /* if preempt_frame is set, we are being preempted */
    if (preempt_frame) {
        LTRACEF("we're preempted, old frame %p, old lr 0x%x, pc 0x%x, new preempted bool %d\n",
                preempt_frame, preempt_frame->lr, preempt_frame->pc, newthread->arch.was_preempted);

#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
        /* see if extended fpu frame was pushed */
        if ((preempt_frame->lr & (1<<4)) == 0) {
            LTRACEF("thread %s pushed fpu frame\n", oldthread->name);

            /* save the top part of the context */
            /* note this should also trigger a lazy fpu save if it hasn't already done so */
            asm volatile("vstm %0, { s16-s31 }" :: "r" (&oldthread->arch.fpregs[0]));
            oldthread->arch.fpused = true;

            /* verify that FPCCR.LSPACT was cleared and CONTROL.FPCA was set */
            DEBUG_ASSERT((FPU->FPCCR & FPU_FPCCR_LSPACT_Msk) == 0);
            DEBUG_ASSERT(__get_CONTROL() & CONTROL_FPCA_Msk);
        } else {
            DEBUG_ASSERT(oldthread->arch.fpused == false);
        }
#endif

        oldthread->arch.was_preempted = true;
        oldthread->arch.tcpu_sp = (addr_t)preempt_frame;
        preempt_frame = NULL;

#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
        /* if new thread has saved fpu state, restore it */
        if (newthread->arch.fpused) {
            LTRACEF("newthread FPCCR.LSPACT %lu, FPCAR 0x%x, CONTROL.FPCA %lu\n",
                    FPU->FPCCR & FPU_FPCCR_LSPACT_Msk, FPU->FPCAR, __get_CONTROL() & CONTROL_FPCA_Msk);

            /* enable the fpu manually */
            __set_CONTROL(__get_CONTROL() | CONTROL_FPCA_Msk);
            asm volatile("isb");

            DEBUG_ASSERT((FPU->FPCCR & FPU_FPCCR_LSPACT_Msk) == 0);
            DEBUG_ASSERT(__get_CONTROL() & CONTROL_FPCA_Msk);

            /* restore the top of the fpu state, the rest will happen below */
            asm volatile("vldm %0, { s16-s31 }" :: "r" (&newthread->arch.fpregs[0]));
        }
#endif

        if (newthread->arch.was_preempted) {
            /* return directly to the preempted thread's iframe */
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
            LTRACEF("newthread2 FPCCR.LSPACT %lu, FPCAR 0x%x, CONTROL.FPCA %lu\n",
                    FPU->FPCCR & FPU_FPCCR_LSPACT_Msk, FPU->FPCAR, __get_CONTROL() & CONTROL_FPCA_Msk);
#endif
            /*
             * We were preempted and thus came through thread_preempt() to get to here
             * which grabbed the thread lock on the way in. We're returning to a thread that
             * was preempted and not holding the thread lock, so drop it now.
             */
            arch_spin_unlock(&thread_lock);

            __asm__ volatile(
                "mov    sp, %0;"
                "cpsie  i;"
                CLREX
                RESTORE_REGS_PC
                :: "r"(newthread->arch.tcpu_sp)
            );
            __UNREACHABLE;
        } else {
            /* we're inside a pendsv, switching to a user mode thread */
            /* set up a fake frame to exception return to */
            struct arm_cm_exception_frame_short *frame = (void *)newthread->arch.tcpu_sp;
            frame--;

            frame->pc = (uint32_t)&_thread_mode_bounce;
            frame->psr = (1 << 24); /* thread bit set, IPSR 0 */
            frame->r0 = frame->r1 = frame->r2 = frame->r3 = frame->r12 = frame->lr = 0;
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
            /* pass the fpused bool to _thread_mode_bounce */
            frame->r0 = newthread->arch.fpused;
#endif

#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
            LTRACEF("iretting to user space, fpused %u\n", newthread->arch.fpused);
#else
            LTRACEF("iretting to user space\n");
#endif

	    /* 当进入异常时，sp=msp
         * 使用bx 0xfffffff9 表示从 handler mode->thread mode，并使用msp指向的栈帧
         * 恢复msp指向的栈帧，并跳转到 frame->pc，所以bx 0xfffffff9 会执行_thread_mode_bounce
         */
	    __asm__ volatile(
                CLREX
                "mov    sp, %0;"
                "bx     %1;"
                :: "r"(frame), "r"(0xfffffff9)
            );
            __UNREACHABLE;
        }
    } else {
        oldthread->arch.was_preempted = false;

#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
        /* see if we have fpu state we need to save */
        if (!oldthread->arch.fpused && __get_CONTROL() & CONTROL_FPCA_Msk) {
            /* mark this thread as using float */
            LTRACEF("thread %s uses float\n", oldthread->name);
            oldthread->arch.fpused = true;
        }
#endif

        if (newthread->arch.was_preempted) {
            LTRACEF("not being preempted, but switching to preempted thread\n");

            /* The thread lock is held now because we entered via a normal reschedule
             * (ie, not a preemption). We're returning directly to a thread that was preempted
             * so drop the thread lock now.
             */
            arch_spin_unlock(&thread_lock);

#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
            _half_save_and_svc(oldthread, newthread, oldthread->arch.fpused, newthread->arch.fpused);
#else
            _half_save_and_svc(oldthread, newthread);
#endif
        } else {
            /* fast path, both sides did not preempt */
            LTRACEF("both sides are not preempted newsp 0x%lx\n", newthread->arch.tcpu_sp);
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
            _arch_non_preempt_context_switch(oldthread, newthread, oldthread->arch.fpused, newthread->arch.fpused);
#else
            _arch_non_preempt_context_switch(oldthread, newthread);
#endif
        }
    }

}

void arch_dump_thread(thread_t *t) {
    if (t->state != THREAD_RUNNING) {
        dprintf(INFO, "\tarch: ");
        dprintf(INFO, "sp 0x%lx, was preempted %u", t->arch.tcpu_sp, t->arch.was_preempted);
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
        dprintf(INFO, ", fpused %u", t->arch.fpused);
#endif
        dprintf(INFO, "\n");
    }
}


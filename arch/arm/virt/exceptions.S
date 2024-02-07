/*
 * Copyright (c) 2008-2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <kern/asm.h>
#include <arch/asm.h>
#include <arch/arm/cores.h>

/* exception handling glue.
 * NOTE: only usable on armv6+ cores
 */

#define TIMESTAMP_IRQ 0
/* HSR macros */
#define HSREC_SHIFT          26
#define HSREC_MASK           (0x3f << HSREC_SHIFT)
#define HSREC_UNKNOWN        0x00
#define HSREC_WFI            0x01
#define HSREC_SVC            0x11
#define HSREC_HVC            0x12
#define HSREC_SMC            0x13
#define HSREC_PREFETCH_ABORT 0x20
#define HSREC_DATA_ABORT     0x24
#define HSRIL32              (1 << 25)

/* macros to align and unalign the stack on 8 byte boundary for ABI compliance */
.macro stack_align, tempreg
    /* make sure the stack is aligned */
    mov     \tempreg, sp
    tst     sp, #4
    subeq   sp, #4
    push    { \tempreg }

    /* tempreg holds the original stack */
.endm

.macro stack_restore, tempreg
    /* restore the potentially unaligned stack */
    pop     { \tempreg }
    mov     sp, \tempreg
.endm

/* save and disable the vfp unit */
.macro vfp_save, temp1
    /* save old fpexc */
    vmrs    \temp1, fpexc

    push    { \temp1 }

    /* hard disable the vfp unit */
    bic     \temp1, #(1<<30)
    vmsr    fpexc, \temp1
.endm

/* restore the vfp enable/disable state */
.macro vfp_restore, temp1
    /* restore fpexc */
    pop     { \temp1 }

    vmsr    fpexc, \temp1
.endm

/* Exception taken to a PL2 mode, also as hyp mode, the und/abt/irq/fiq/svc/hvc taken from PL0/1 mode Non-secure
 * Exception taken from PL2 mode Non-secure such as und/abt/svc, must taken to PL2 mode(default mode)
 * The hyp trap exception must taken to PL2 mode, include und/abt/svc/hvc taken from PL0/1 mode Non-secure
 * The physics abt/irq/fiq exception can taken to PL2 mode, the virtual abt/irq/fiq must be taken to PL1 mode 
 * The svc trap to hyp mode is not necessary
 * 
 * elr_hyp no offset, 
 * HCR.TGE/AMO/FMO/IMO controls the exception routing to PL2 mode and generate virtual exception(VA/VI/VF)
 * HSTR/HCPTR controls the system instructions/cop routing to PL2 mode
 * Refer by B1-1174 and Table B1-6 Exception return addresses 
 */

/* Save callee trashed registers.
 * At exit r0 contains a pointer to the register frame.
 */
.macro save
    /* User mode, System mode, and Hyp mode share the same LR, so lr->lk_usr
     * In hyp mode, taken from PL0, R0-12_usr, LR_usr(R13), SP_usr(R14) maybe tempreg
     * spsr_hyp/elr_hyp <- cpsr/pc in PL0
     * Taken from PL1, R0-12_usr, LR_usr, SP_usr(PL1 reuse) maybe tempreg
     * spsr_hyp/elr_hyp <- spsr/pc in PL1
     */
    /* if the exception from PL0/PL1, we just need save/restore sp_usr/lr_usr,
     * because the sp_svc/lr_svc .etc have the banked registers.
     */
    /* save spsr_hyp and elr_hyp onto the hyp stack */
    str     lr, [sp, #-36] /* save lr_usr to ulr=4*9 */
    str     lr, [sp, #-12] /* save lr_usr to lr=4*3 */
    mrs     lr, spsr /* spsr_hyp */
    push    {lr}
    mrs     lr, elr_hyp
    push    {lr}

    /* hyp mode, interrupts disabled */
    cpsid   i,#0x1a

    /* save callee trashed regs */
    sub     sp, #4
    push    { r0-r3, r12 }

    /* save user space sp_usr */
    sub     sp, #8
    mrs     lr, sp_usr
    str     lr, [sp]
#if ARM_WITH_VFP
    /* save and disable the vfp unit */
    vfp_save    r0
#endif

    /* make sure the stack is 8 byte aligned */
    stack_align r0

    /* r0 now holds the pointer to the original iframe (before alignment) */
.endm

.macro restore
    /* undo the stack alignment we did before */
    stack_restore r0

#if ARM_WITH_VFP
    /* restore the old state of the vfp unit */
    vfp_restore r0
#endif

    /* restore user space sp_usr */    
    ldr     lr, [sp]
    msr     sp_usr, lr
    add     sp, #8

    pop     { r0-r3, r12 }
    add     sp, #4

    /* return to whence we came from */
    pop     {lr}
    msr     elr_hyp, lr
    pop     {lr}
    msr     spsr, lr /* spsr_hyp */
    ldr     lr, [sp, #36]
    eret
.endm


/* Save all registers.
 * At exit r0 contains a pointer to the register frame.
 */
.macro saveall
    /* save spsr_hyp and elr_hyp onto the hyp stack */
    str     lr, [sp, #-68] /* save lr_usr to ulr=4*17 */
    str     lr, [sp, #-12] /* save lr_usr to lr=4*3 */
    mrs     lr, spsr /* spsr_hyp */
    push    {lr}
    mrs     lr, elr_hyp
    push    {lr}

    /* hyp mode, interrupts disabled */
    cpsid   i,#0x1a

    /* save all regs */
    sub     sp, #4
    push    { r0-r12 }

    /* save user space sp_usr */
    sub     sp, #8
    mrs     lr, sp_usr
    str     lr, [sp]

#if ARM_WITH_VFP
    /* save and disable the vfp unit */
    vfp_save    r0
#endif

    /* make sure the stack is 8 byte aligned */
    stack_align r0

    /* r0 now holds the pointer to the original iframe (before alignment) */
.endm

.macro restoreall
    /* undo the stack alignment we did before */
    stack_restore r0

#if ARM_WITH_VFP
    /* restore the old state of the vfp unit */
    vfp_restore r0
#endif

    /* restore user space sp_usr */
    ldr     lr, [sp]
    msr     sp_usr, lr
    add     sp, #8

    pop     { r0-r12 }
    add     sp, #4

    /* return to whence we came from */
    pop     {lr}
    msr     elr_hyp, lr
    pop     {lr}
    msr     spsr, lr /* spsr_hyp */
    ldr     lr, [sp, #68]
    eret
.endm

/***********************************************
 *** Traps taken to HYP mode from PL1/0 mode ***
 ***********************************************/

FUNCTION(arm_trap)
    saveall
    /* r0 now holds pointer to iframe */
    /* ARM_ARM B3.13.6 */
    mrc     p15, 4, lr, c5 , c2, 0 // HSR

    and     r1, lr, #(HSREC_MASK)
    cmp     r1, #(HSREC_SVC << HSREC_SHIFT)
    beq     arm_vsyscall
    cmp     r1, #(HSREC_HVC << HSREC_SHIFT)
    beq     arm_vsyscall
    cmp     r1, #(HSREC_PREFETCH_ABORT << HSREC_SHIFT)
    beq     arm_vprefetch_abort
    cmp     r1, #(HSREC_DATA_ABORT << HSREC_SHIFT)
    beq     arm_vdata_abort
    cmp     r1, #(HSREC_UNKNOWN << HSREC_SHIFT)
    beq     arm_vundefined_inst

    /** Everything else is assumed to be a VCPU trap **/
    /* r1 now holds register of hsr */
    mov     r1, lr
    bl      arm_vcpu_handler
    restoreall

arm_vsyscall:
#ifndef WITH_LIB_SYSCALL
    /* r1 now holds register of hsr */
    mov     r1, lr
    bl      arm_vsyscall_handler
#else
    /* ARM syscall ABI
     * ===============
     * Only syscalls with 4 args (max) are currently supported
     * r0-r3 = args
     * r0-r1 = return value (r0 only if 32-bit retval)
     * r12   = syscall number, expected to be trashed.
     * syscalls run with interrupts enabled
    */
    /* restore r0 */
#if ARM_WITH_VFP    
    ldr     lr, [r0, #12] /* r0 = 3*4 */
#else
    ldr     lr, [r0, #8] /* r0 = 2*4 */
#endif
    mov     r0, lr
    ldr	    lr, =nr_syscalls
	ldr	    lr, [lr]
	cmp	    r12, lr

	ldrlo	lr, =syscall_table
	ldrlo	lr, [lr, r12, lsl#2]
    /* lr ?= 0 */
    cmp     lr, #0
	// rsbslo	r12, lr, #1
	// ldrhs	lr,=sys_undefined
	ldreq   lr,=sys_undefined
    blx	    lr
#endif    
    restoreall

arm_vprefetch_abort:    /* generate virtual abt exception */
    /* r1 now holds register of hsr */
    mov     r1, lr
    bl      arm_vprefetch_abort_handler
    restoreall

arm_vdata_abort:        /* generate virtual abt exception */
    /* r1 now holds register of hsr */
    mov     r1, lr
    bl      arm_vdata_abort_handler
    restoreall

arm_vundefined_inst:
    /* r1 now holds register of hsr */
    mov     r1, lr
    bl      arm_vundefined_handler
    restoreall

/*********************************
 *** Traps taken from HYP mode ***
 *********************************/

FUNCTION(arm_undefined)
    save
    /* r0 now holds pointer to iframe */
    mrc     p15, 4, lr, c5 , c2, 0 // HSR
    /* r1 now holds register of hsr */
    mov     r1, lr

    bl      arm_vundefined_handler

    restore

FUNCTION(arm_syscall_hyp)
    saveall
    /* r0 now holds pointer to iframe */
    mrc     p15, 4, lr, c5 , c2, 0 // HSR
    /* r1 now holds register of hsr */
    mov     r1, lr

    bl      arm_syscall_handler

    restoreall

FUNCTION(arm_prefetch_abort)
    saveall
    /* r0 now holds pointer to iframe */
    mrc     p15, 4, lr, c5 , c2, 0 // HSR
    /* r1 now holds register of hsr */
    mov     r1, lr

    bl      arm_vprefetch_abort_handler

    restoreall

FUNCTION(arm_data_abort)
    saveall
    /* r0 now holds pointer to iframe */
    mrc     p15, 4, lr, c5 , c2, 0 // HSR
    /* r1 now holds register of hsr */
    mov     r1, lr

    bl      arm_vdata_abort_handler

    restoreall

FUNCTION(arm_reserved)
    b   .

/**************************************************
 *** Other exceptions taken from PL0/1/2 mode   ***
 *** Generate virtual irq/fiq exception for PL1 ***
 *************************************************/

FUNCTION(arm_irq)
#if TIMESTAMP_IRQ
    /* read the cycle count */
    mrc     p15, 0, sp, c9, c13, 0
    str     sp, [pc, #__irq_cycle_count - . - 8]
#endif

    save

    /* r0 now holds pointer to iframe */

    /* track that we're inside an irq handler */
    LOADCONST(r2, __arm_in_handler)
    mov     r1, #1
    str     r1, [r2]

    /* call into higher level code */
    bl  platform_irq

    /* clear the irq handler status */
    LOADCONST(r1, __arm_in_handler)
    mov     r2, #0
    str     r2, [r1]

    /* reschedule if the handler returns nonzero */
    cmp     r0, #0
    blne    thread_preempt

    restore

FUNCTION(arm_fiq)
    save
    /* r0 now holds pointer to iframe */

    bl  platform_fiq

    restore

.ltorg

#if TIMESTAMP_IRQ
DATA(__irq_cycle_count)
    .word   0
#endif

.data
DATA(__arm_in_handler)
    .word   0

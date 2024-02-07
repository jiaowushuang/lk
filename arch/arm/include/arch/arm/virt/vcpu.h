/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <arch/arch_helpers.h>
#include <arch/arch_thread.h>
#include <arch/vcpu.h>
#include <platform/interrupts.h>
#include <platform/gic.h>

#ifdef ARM_ISA_ARMV8R
#define HCR_COMMON ( HCR_TWE | HCR_TWI | HCR_AMO | HCR_IMO \
                   | HCR_FMO | HCR_DC  | HCR_VM)
/* Allow native tasks to run at PL1, but restrict access */
/* VCPU disable */
#define HCR_NATIVE ( HCR_COMMON | HCR_TGE | HCR_TVM | HCR_TCACHE \
			   | HCR_TAC | HCR_SWIO)
#else
#ifdef CONFIG_DISABLE_WFI_WFE_TRAPS
/* Trap SMC and override CPSR.AIF */
#define HCR_COMMON ( HCR_TSC | HCR_AMO | HCR_IMO \
                   | HCR_FMO | HCR_DC  | HCR_VM)
#else
/* Trap WFI/WFE/SMC and override CPSR.AIF */
#define HCR_COMMON ( HCR_TSC | HCR_TWE | HCR_TWI | HCR_AMO | HCR_IMO \
                   | HCR_FMO | HCR_DC  | HCR_VM)
#endif
/* Allow native tasks to run at PL1, but restrict access */
/* VCPU disable */
#define HCR_NATIVE ( HCR_COMMON | HCR_TGE | HCR_TVM | HCR_TTLB | HCR_TCACHE \
                   | HCR_TAC | HCR_SWIO)
#endif

/* VCPU enable */                   
#define HCR_VCPU   (HCR_COMMON)

/* Amongst other things we set the caches to enabled by default. This
 * may cause problems when booting guests that expect caches to be
 * disabled */
#ifdef ARM_ISA_ARMV8R
#define SCTLR_DEFAULT 0xc20078 // BR=0(Background Region disable), BR=1 0xc20078
#else
#define SCTLR_DEFAULT 0xc50078 // 0xc5187c
#endif

#define ACTLR_DEFAULT 0x40 
// TODO: if ACTLR_DEFAULT==0, 
// hypervisor occurs the exception of FIQ
// (platform_fiq:515 unimplemented)

static inline uint32_t get_lr_svc(void)
{
    uint32_t ret;
    asm("mrs %[ret], lr_svc" : [ret]"=r"(ret));
    return ret;
}

static inline void set_lr_svc(uint32_t val)
{
    asm("msr lr_svc, %[val]" :: [val]"r"(val));
}

static inline uint32_t get_sp_svc(void)
{
    uint32_t ret;
    asm("mrs %[ret], sp_svc" : [ret]"=r"(ret));
    return ret;
}

static inline void set_sp_svc(uint32_t val)
{
    asm("msr sp_svc, %[val]" :: [val]"r"(val));
}

static inline uint32_t get_spsr_svc(void)
{
    uint32_t ret;
    asm("mrs %[ret], spsr_svc" : [ret]"=r"(ret));
    return ret;
}

static inline void set_spsr_svc(uint32_t val)
{
    asm("msr spsr_svc, %[val]" :: [val]"r"(val));
}

static inline uint32_t get_lr_abt(void)
{
    uint32_t ret;
    asm("mrs %[ret], lr_abt" : [ret]"=r"(ret));
    return ret;
}

static inline void set_lr_abt(uint32_t val)
{
    asm("msr lr_abt, %[val]" :: [val]"r"(val));
}

static inline uint32_t get_sp_abt(void)
{
    uint32_t ret;
    asm("mrs %[ret], sp_abt" : [ret]"=r"(ret));
    return ret;
}

static inline void set_sp_abt(uint32_t val)
{
    asm("msr sp_abt, %[val]" :: [val]"r"(val));
}

static inline uint32_t get_spsr_abt(void)
{
    uint32_t ret;
    asm("mrs %[ret], spsr_abt" : [ret]"=r"(ret));
    return ret;
}

static inline void set_spsr_abt(uint32_t val)
{
    asm("msr spsr_abt, %[val]" :: [val]"r"(val));
}

static inline uint32_t get_lr_und(void)
{
    uint32_t ret;
    asm("mrs %[ret], lr_und" : [ret]"=r"(ret));
    return ret;
}

static inline void set_lr_und(uint32_t val)
{
    asm("msr lr_und, %[val]" :: [val]"r"(val));
}

static inline uint32_t get_sp_und(void)
{
    uint32_t ret;
    asm("mrs %[ret], sp_und" : [ret]"=r"(ret));
    return ret;
}

static inline void set_sp_und(uint32_t val)
{
    asm("msr sp_und, %[val]" :: [val]"r"(val));
}

static inline uint32_t get_spsr_und(void)
{
    uint32_t ret;
    asm("mrs %[ret], spsr_und" : [ret]"=r"(ret));
    return ret;
}

static inline void set_spsr_und(uint32_t val)
{
    asm("msr spsr_und, %[val]" :: [val]"r"(val));
}

static inline uint32_t get_lr_irq(void)
{
    uint32_t ret;
    asm("mrs %[ret], lr_irq" : [ret]"=r"(ret));
    return ret;
}

static inline void set_lr_irq(uint32_t val)
{
    asm("msr lr_irq, %[val]" :: [val]"r"(val));
}

static inline uint32_t get_sp_irq(void)
{
    uint32_t ret;
    asm("mrs %[ret], sp_irq" : [ret]"=r"(ret));
    return ret;
}

static inline void set_sp_irq(uint32_t val)
{
    asm("msr sp_irq, %[val]" :: [val]"r"(val));
}

static inline uint32_t get_spsr_irq(void)
{
    uint32_t ret;
    asm("mrs %[ret], spsr_irq" : [ret]"=r"(ret));
    return ret;
}

static inline void set_spsr_irq(uint32_t val)
{
    asm("msr spsr_irq, %[val]" :: [val]"r"(val));
}

static inline uint32_t get_lr_fiq(void)
{
    uint32_t ret;
    asm("mrs %[ret], lr_fiq" : [ret]"=r"(ret));
    return ret;
}

static inline void set_lr_fiq(uint32_t val)
{
    asm("msr lr_fiq, %[val]" :: [val]"r"(val));
}

static inline uint32_t get_sp_fiq(void)
{
    uint32_t ret;
    asm("mrs %[ret], sp_fiq" : [ret]"=r"(ret));
    return ret;
}

static inline void set_sp_fiq(uint32_t val)
{
    asm("msr sp_fiq, %[val]" :: [val]"r"(val));
}

static inline uint32_t get_spsr_fiq(void)
{
    uint32_t ret;
    asm("mrs %[ret], spsr_fiq" : [ret]"=r"(ret));
    return ret;
}

static inline void set_spsr_fiq(uint32_t val)
{
    asm("msr spsr_fiq, %[val]" :: [val]"r"(val));
}

static inline uint32_t get_r8_fiq(void)
{
    uint32_t ret;
    asm("mrs %[ret], r8_fiq" : [ret]"=r"(ret));
    return ret;
}

static inline void set_r8_fiq(uint32_t val)
{
    asm("msr r8_fiq, %[val]" :: [val]"r"(val));
}

static inline uint32_t get_r9_fiq(void)
{
    uint32_t ret;
    asm("mrs %[ret], r9_fiq" : [ret]"=r"(ret));
    return ret;
}

static inline void set_r9_fiq(uint32_t val)
{
    asm("msr r9_fiq, %[val]" :: [val]"r"(val));
}

static inline uint32_t get_r10_fiq(void)
{
    uint32_t ret;
    asm("mrs %[ret], r10_fiq" : [ret]"=r"(ret));
    return ret;
}

static inline void set_r10_fiq(uint32_t val)
{
    asm("msr r10_fiq, %[val]" :: [val]"r"(val));
}

static inline uint32_t get_r11_fiq(void)
{
    uint32_t ret;
    asm("mrs %[ret], r11_fiq" : [ret]"=r"(ret));
    return ret;
}

static inline void set_r11_fiq(uint32_t val)
{
    asm("msr r11_fiq, %[val]" :: [val]"r"(val));
}

static inline uint32_t get_r12_fiq(void)
{
    uint32_t ret;
    asm("mrs %[ret], r12_fiq" : [ret]"=r"(ret));
    return ret;
}

static inline void set_r12_fiq(uint32_t val)
{
    asm("msr r12_fiq, %[val]" :: [val]"r"(val));
}


static inline uint32_t get_cntv_tval(void)
{
    uint32_t ret = 0;
    ret = read_cntv_tval();
    return ret;
}

static inline void set_cntv_tval(uint32_t val)
{
    write_cntv_tval(val);
}

static inline uint32_t get_cntv_ctl(void)
{
    uint32_t ret = 0;
    ret = read_cntv_ctl();
    return ret;
}

static inline void set_cntv_ctl(uint32_t val)
{
    write_cntv_ctl(val);
}


static inline uint32_t get_vmpidr(void)
{
    uint32_t ret = 0;
    ret = read_vmpidr();
    return ret;
}

static inline void set_vmpidr(uint32_t val)
{
    write_vmpidr(val);
}


/** MODIFIES: phantom_machine_state */
/** DONT_TRANSLATE */
static inline void set_cntv_cval_64(uint64_t val)
{
	write64_cntv_cval(val);
}

/** MODIFIES: */
/** DONT_TRANSLATE */
static inline uint64_t get_cntv_cval_64(void)
{
    uint64_t ret = 0;
    ret = read64_cntv_cval();
    return ret;
}

static inline void set_cntv_cval_high(uint32_t val)
{
    uint64_t ret = get_cntv_cval_64();
    uint64_t cval_high = (uint64_t) val << 32 ;
    uint64_t cval_low = (ret << 32) >> 32;
    set_cntv_cval_64(cval_high | cval_low);
}

static inline uint32_t get_cntv_cval_high(void)
{
    uint64_t ret = get_cntv_cval_64();
    return (uint32_t)(ret >> 32);
}

static inline void set_cntv_cval_low(uint32_t val)
{
    uint64_t ret = get_cntv_cval_64();
    uint64_t cval_high = (ret >> 32) << 32;
    uint64_t cval_low = (uint64_t) val;
    set_cntv_cval_64(cval_high | cval_low);
}

static inline uint32_t get_cntv_cval_low(void)
{
    uint64_t ret = get_cntv_cval_64();
    return (uint32_t) ret;
}

/** MODIFIES: phantom_machine_state */
/** DONT_TRANSLATE */
static inline void set_cntv_off_64(uint64_t val)
{
	write64_cntvoff(val);
}

/** MODIFIES: */
/** DONT_TRANSLATE */
static inline uint64_t get_cntv_off_64(void)
{
    uint64_t ret = 0;
    ret = read64_cntvoff();
    return ret;
}

static inline void set_cntv_off_high(uint32_t val)
{
    uint64_t ret = get_cntv_off_64();
    uint64_t cv_off_high = (uint64_t) val << 32 ;
    uint64_t cv_off_low = (ret << 32) >> 32;
    set_cntv_off_64(cv_off_high | cv_off_low);
}

static inline uint32_t get_cntv_off_high(void)
{
    uint64_t ret = get_cntv_off_64();
    return (uint32_t)(ret >> 32);
}

static inline void set_cntv_off_low(uint32_t val)
{
    uint64_t ret = get_cntv_off_64();
    uint64_t cv_off_high = (ret >> 32) << 32;
    uint64_t cv_off_low = (uint64_t) val;
    set_cntv_off_64(cv_off_high | cv_off_low);
}

static inline uint32_t get_cntv_off_low(void)
{
    uint64_t ret = get_cntv_off_64();
    return (uint32_t) ret;
}

#ifdef ARM_WITH_VFP
static inline uint32_t read_fpexc(void) {
    uint32_t val;
    /* use legacy encoding of vmsr reg, fpexc */
    __asm__("mrc  p10, 7, %0, c8, c0, 0" : "=r" (val));
    return val;
}

static inline void write_fpexc(uint32_t val) {
    /* use legacy encoding of vmrs fpexc, reg */
    __asm__ volatile("mcr  p10, 7, %0, c8, c0, 0" :: "r" (val));
}

static inline void access_fpexc(pcb_user_ctx_t *pcb, bool write)
{
    /* save a copy of the current status since
     * the enableFpuHyp modifies the armHSFPUEnabled
     */
    bool flag = pcb->fpused;
    if (!flag) {
	    write_hcptr(read_hcptr() & HCPTR_MASK);
    }
    if (write) {
	    write_fpexc(vcpu_read_reg(pcb, CTX_FPEXC));
    } else {
        uint32_t fpexc;
	    fpexc = read_fpexc();
	    vcpu_write_reg(pcb, CTX_FPEXC, fpexc);
    }
    /* restore the status */
    if (!flag) {
        write_hcptr(read_hcptr() | ~HCPTR_MASK);
    }
}
#endif

static inline void armv_vcpu_boot_init(void)
{
#if defined(ARM_HYP_TRAP_CP14_IN_VCPU_THREADS) || defined(ARM_HYP_TRAP_CP14_IN_NATIVE_USER_THREADS)
    /* On the verified build, we have implemented a workaround that ensures
     * that we don't need to save and restore the debug coprocessor's state
     * (and therefore don't have to expose the CP14 registers to verification).
     *
     * This workaround is simple: we just trap and intercept all Guest VM
     * accesses to the debug coprocessor, and deliver them as VMFault
     * messages to the VM Monitor. To that end, the VM Monitor can then
     * choose to either kill the Guest VM, or it can also choose to silently
     * step over the Guest VM's accesses to the debug coprocessor, thereby
     * silently eliminating the communication channel between the Guest VMs
     * (because the debug coprocessor acted as a communication channel
     * unless we saved/restored its state between VM switches).
     *
     * This workaround delegates the communication channel responsibility
     * from the kernel to the VM Monitor, essentially.
     */
    // hdcr bits
#endif
}

extern void save_virt_timer(pcb_user_ctx_t *pcb);
extern void restore_virt_timer(pcb_user_ctx_t *pcb);
static inline void armv_vcpu_save(pcb_user_ctx_t *pcb, bool active)
{
    /* save registers */
    vcpu_save_reg_range(pcb, CTX_ACTLR, CTX_SPSRfiq);

#ifdef ARM_HYP_CP14_SAVE_AND_RESTORE_VCPU_THREADS
    /* This is done when we are asked to save and restore the CP14 debug context
     * of VCPU threads; the register context is saved into the underlying TCB.
     */
    // dcc
#endif
    isb();
#ifdef ARM_WITH_VFP
    /* Other FPU registers are still lazily saved and restored when
     * handleFPUFault is called. See the comments in vcpu_enable
     * for more information.
     */
    if (active && pcb->fpused) {
        access_fpexc(pcb, false);
    }
#endif
}

static inline void vcpu_enable(pcb_user_ctx_t *pcb)
{
    vcpu_restore_reg(pcb, CTX_SCTLR);
    write_hcr(HCR_VCPU);
    isb();

    /* Turn on the VGIC */
    set_gic_vcpu_ctrl_hcr(pcb->vgic.hcr);

#if !defined(ARM_CP14_SAVE_AND_RESTORE_NATIVE_THREADS) && defined(ARM_HYP_CP14_SAVE_AND_RESTORE_VCPU_THREADS)
    /* This is guarded by an #ifNdef (negation) ARM_CP14_SAVE_AND_RESTORE_NATIVE_THREADS
     * because if it wasn't, we'd be calling restore_user_debug_context twice
     * on a debug-API build; recall that restore_user_debug_context is called
     * in restore_user_context.
     *
     * We call restore_user_debug_context here, because vcpu_restore calls this
     * function (vcpu_enable). It's better to embed the
     * restore_user_debug_context call in here than to call it in the outer
     * level caller (vcpu_switch), because if the structure of this VCPU code
     * changes later on, it will be less likely that the person who changes
     * the code will be able to omit the debug register context restore, if
     * it's done here.
     */
    // dcc
#endif
#if defined(ARM_HYP_TRAP_CP14_IN_NATIVE_USER_THREADS)
    /* Disable debug exception trapping and let the PL1 Guest VM handle all
     * of its own debug faults.
     */
    // hdcr
#endif
#ifdef ARM_WITH_VFP
    /* We need to restore the FPEXC value early for the following reason:
     *
     * 1: When an application inside a VM is trying to execute an FPU
     * instruction and the EN bit of FPEXC is disabled, an undefined
     * instruction exception is sent to the guest Linux kernel instead of
     * the seL4. Until the Linux kernel examines the EN bit of the FPEXC
     * to determine if the exception FPU related, a VCPU trap is sent to
     * the seL4 kernel. However, it can be too late to restore the value
     * of saved FPEXC in the VCPU trap handler: if the EN bit of the saved
     * FPEXC is enabled, the Linux kernel thinks the FPU is enabled and
     * thus refuses to handle the exception. The result is the application
     * is killed with the cause of illegal instruction.
     *
     * Note that we restore the FPEXC here, but the current FPU owner
     * can be a different thread. Thus, it seems that we are modifying
     * another thread's FPEXC. However, the modification is OK.
     *
     * 1: If the other thread is a native thread, even if the EN bit of
     * the FPEXC is enabled, a trap th HYP mode will be triggered when
     * the thread tries to use the FPU.
     *
     * 2: If the other thread has a VCPU, the FPEXC is already saved
     * in the VCPU's pcb->fpexc when the VCPU is saved or disabled.
     *
     * We also overwrite the fpuState.fpexc with the value saved in
     * pcb->fpexc. Since the following scenario can happen:
     *
     * VM0 (the FPU owner) -> VM1 (update the FPEXC in vcpu_enable) ->
     * switchLocalFpuOwner (save VM0 with modified FPEXC) ->
     * VM1 (the new FPU owner)
     *
     * In the case above, the fpuState.fpexc of VM0 saves the value written
     * by the VM1, but the pcb->fpexc of VM0 still contains the correct
     * value when VM0 is disabed (vcpu_disable) or saved (vcpu_save).
     *
     *
     */

    pcb->fpexc = vcpu_read_reg(pcb, CTX_FPEXC);
    access_fpexc(pcb, true);
#endif
    /* Restore virtual timer state */
    restore_virt_timer(pcb);
}

static inline void vcpu_disable(pcb_user_ctx_t *pcb)
{
    uint32_t hcr;
    dsb();
    if (likely(pcb)) {
        hcr = get_gic_vcpu_ctrl_hcr();
        pcb->vgic.hcr = hcr;
        vcpu_save_reg(pcb, CTX_SCTLR);
        isb();
#ifdef ARM_WITH_VFP
        if (pcb->fpused) {
            access_fpexc(pcb, false);
        }
#endif
    }
    /* Turn off the VGIC */
    set_gic_vcpu_ctrl_hcr(0);
    isb();

    /* Stage 1 MMU off */
    write_sctlr(SCTLR_DEFAULT);
    write_hcr(HCR_NATIVE);

#if defined(ARM_HYP_CP14_SAVE_AND_RESTORE_VCPU_THREADS)
    /* Disable all breakpoint registers from triggering their
     * respective events, so that when we switch from a guest VM
     * to a native thread, the native thread won't trigger events
     * that were caused by things the guest VM did.
     */
    // dcc
#endif
#if defined(ARM_HYP_TRAP_CP14_IN_NATIVE_USER_THREADS)
    /* Enable debug exception trapping and let seL4 trap all PL0 (user) native
     * seL4 threads' debug exceptions, so it can deliver them as fault messages.
     */
    // hdcr
#endif
    isb();
    if (likely(pcb)) {
        /* Save virtual timer state */
        save_virt_timer(pcb);
        /* Mask the virtual timer interrupt */
        unmask_interrupt(ARM_GENERIC_TIMER_VIRTUAL_INT);
    }
}

static inline void armv_vcpu_init(pcb_user_ctx_t *pcb)
{
    vcpu_write_reg(pcb, CTX_SCTLR, SCTLR_DEFAULT);
    vcpu_write_reg(pcb, CTX_ACTLR, ACTLR_DEFAULT);
}

static inline bool vcpu_reg_saved_when_disabled(uint32_t field)
{
    switch (field) {
    case CTX_SCTLR:
        return true;
    default:
        return false;
    }
}



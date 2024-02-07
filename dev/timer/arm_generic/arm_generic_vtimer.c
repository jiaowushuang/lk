/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include <arch/arm/virt/vcpu.h>
#include <platform/interrupts.h>
#include <platform/gic.h>

static inline uint64_t read_cntpct(void)
{
    uint64_t val;
    val = read64_cntpct();
    return val;
}

void save_virt_timer(pcb_user_ctx_t *pcpu)
{
    /* Save control register */
    vcpu_save_reg(pcpu, CTX_CNTV_CTL);
    vcpu_hw_write_reg(CTX_CNTV_CTL, 0);
    /* Save Compare Value and Offset registers */
#ifdef IS_64BIT
    vcpu_save_reg(pcpu, CTX_CNTV_CVAL);
    vcpu_save_reg(pcpu, CTX_CNTVOFF);
    vcpu_save_reg(pcpu, CTX_CNTKCTL_EL1);
    // check_export_arch_timer();
#else
    uint64_t cval = get_cntv_cval_64();
    uint64_t cntvoff = get_cntv_off_64();
    vcpu_write_reg(pcpu, CTX_CNTV_CVALhigh, (uint32_t)(cval >> 32));
    vcpu_write_reg(pcpu, CTX_CNTV_CVALlow, (uint32_t)cval);
    vcpu_write_reg(pcpu, CTX_CNTV_OFFhigh, (uint32_t)(cntvoff >> 32));
    vcpu_write_reg(pcpu, CTX_CNTV_OFFlow, (uint32_t)cntvoff);
#endif
#ifdef CONFIG_VTIMER_UPDATE_VOFFSET
    /* Save counter value at the time the pcpu is disabled */
    pcpu->vtimer.last_pcount = read_cntpct();
#endif
}

void restore_virt_timer(pcb_user_ctx_t *pcpu)
{
    /* Restore virtual timer state */
#ifdef IS_64BIT
    vcpu_restore_reg(pcpu, CTX_CNTV_CVAL);
    vcpu_restore_reg(pcpu, CTX_CNTKCTL_EL1);
#else
    uint32_t cval_high = vcpu_read_reg(pcpu, CTX_CNTV_CVALhigh);
    uint32_t cval_low = vcpu_read_reg(pcpu, CTX_CNTV_CVALlow);
    uint64_t cval = ((uint64_t)cval_high << 32) | (uint64_t) cval_low;
    set_cntv_cval_64(cval);
#endif

    /* Set virtual timer offset */
#ifdef CONFIG_VTIMER_UPDATE_VOFFSET
    uint64_t pcount_delta;
    uint64_t current_cntpct = read_cntpct();
    pcount_delta = current_cntpct - pcpu->vtimer.last_pcount;
#endif

#ifdef IS_64BIT
#ifdef CONFIG_VTIMER_UPDATE_VOFFSET
    uint64_t offset = vcpu_read_reg(pcpu, CTX_CNTVOFF);
    offset += pcount_delta;
    vcpu_write_reg(pcpu, CTX_CNTVOFF, offset);
#endif
    vcpu_restore_reg(pcpu, CTX_CNTVOFF);
#else
    uint32_t offset_high = vcpu_read_reg(pcpu, CTX_CNTV_OFFhigh);
    uint32_t offset_low = vcpu_read_reg(pcpu, CTX_CNTV_OFFlow);
    uint64_t offset = ((uint64_t)offset_high << 32) | (uint64_t) offset_low;
#ifdef CONFIG_VTIMER_UPDATE_VOFFSET
    offset += pcount_delta;
    vcpu_write_reg(pcpu, CTX_CNTV_OFFhigh, (uint32_t)(offset >> 32));
    vcpu_write_reg(pcpu, CTX_CNTV_OFFlow, (uint32_t) offset);
#endif
    set_cntv_off_64(offset);
#endif
    /* For verification, need to ensure we don't unmask an inactive interrupt;
     * the virtual timer should never get disabled, but the knowledge is not
     * available at this point */
    /* Restore interrupt mask state */
    if (pcpu->vppi_masked) {
	    unmask_interrupt(ARM_GENERIC_TIMER_VIRTUAL_INT);
    } else {
	    mask_interrupt(ARM_GENERIC_TIMER_VIRTUAL_INT);
    }

    /* Restore virtual timer control register */
    vcpu_restore_reg(pcpu, CTX_CNTV_CTL);
}

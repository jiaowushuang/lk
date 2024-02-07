
#include <arch/arm/virt/vcpu.h>
#include <kern/debug.h>
#include <assert.h>

void vcpu_boot_init(void)
{
    armv_vcpu_boot_init();
    gic_vcpu_num_list_regs = VGIC_VTR_NLISTREGS(get_gic_vcpu_ctrl_vtr());
    if (gic_vcpu_num_list_regs > GIC_VCPU_MAX_NUM_LR) {
        printf("Warning: VGIC is reporting more list registers than we support. Truncating\n");
        gic_vcpu_num_list_regs = GIC_VCPU_MAX_NUM_LR;
    }
    vcpu_disable(NULL);
}

void vcpu_save(pcb_user_ctx_t *pcb, bool active)
{
    uint32_t i;
    unsigned int lr_num;

    DEBUG_ASSERT(pcb);
    dsb();
    /* If we aren't active then this state already got stored when
     * we were disabled */
    if (active) {
        vcpu_save_reg(pcb, CTX_SCTLR);
        pcb->vgic.hcr = get_gic_vcpu_ctrl_hcr();
        save_virt_timer(pcb);
    }

    /* Store GIC VCPU control state */
    pcb->vgic.vmcr = get_gic_vcpu_ctrl_vmcr();
    pcb->vgic.apr = get_gic_vcpu_ctrl_apr();
    lr_num = gic_vcpu_num_list_regs;
    for (i = 0; i < lr_num; i++) {
        pcb->vgic.lr[i] = get_gic_vcpu_ctrl_lr(i);
    }
    armv_vcpu_save(pcb, active);
}

void vcpu_restore(pcb_user_ctx_t *pcb)
{
    DEBUG_ASSERT(pcb);

    uint32_t i;
    unsigned int lr_num;
    
    /* Turn off the VGIC */
    set_gic_vcpu_ctrl_hcr(0);
    isb();

    /* Restore GIC VCPU control state */
    set_gic_vcpu_ctrl_vmcr(pcb->vgic.vmcr);
    set_gic_vcpu_ctrl_apr(pcb->vgic.apr);
    lr_num = gic_vcpu_num_list_regs;
    for (i = 0; i < lr_num; i++) {
        set_gic_vcpu_ctrl_lr(i, pcb->vgic.lr[i]);
    }

    /* restore registers */
#ifdef IS_64BIT
    vcpu_restore_reg_range(pcb, CTX_TTBR0, CTX_SPSR_EL1);
#else
    vcpu_restore_reg_range(pcb, CTX_ACTLR, CTX_SPSRfiq);
#endif
    vcpu_enable(pcb);
}

void vcpu_init(pcb_user_ctx_t *pcb)
{
    armv_vcpu_init(pcb);
    /* GICH VCPU interface control */
    pcb->vgic.hcr = VGIC_HCR_EN;
#ifdef CONFIG_VTIMER_UPDATE_VOFFSET
    /* Virtual Timer interface */
    pcb->vtimer.last_pcount = 0;
#endif
}

void vcpu_switch(pcb_user_ctx_t *old, pcb_user_ctx_t *new)
{
	DEBUG_ASSERT(new);
    if (old)
	    vcpu_save(old, true);
    vcpu_restore(new);
}
 
uint32_t vcpu_hw_read_reg(uint32_t reg_index)
{
    uint32_t reg = 0;
    switch (reg_index) {
    case CTX_SCTLR:
        return read_sctlr();
    case CTX_ACTLR:
        return read_actlr();
    case CTX_TTBCR:
#ifdef ARM_CPU_CORTEX_R   
        return reg;
#else
        return read_ttbcr();
#endif        
    case CTX_TTBR0:
#ifdef ARM_CPU_CORTEX_R  
        return reg;
#else     
        return read_ttbr0();
#endif        
    case CTX_TTBR1:
#ifdef ARM_CPU_CORTEX_R   
        return reg;
#else    
        return read_ttbr1();
#endif        
    case CTX_DACR:
        return read_dacr();
    case CTX_DFSR:
        return read_dfsr();
    case CTX_IFSR:
        return read_ifsr();
    case CTX_ADFSR:
        return read_adfsr();
    case CTX_AIFSR:
        return read_aifsr();
    case CTX_DFAR:
        return read_dfar();
    case CTX_IFAR:
        return read_ifar();
    case CTX_PRRR:
#ifdef ARM_CPU_CORTEX_R 
        return reg;
#else      
        return read_prrr();
#endif        
    case CTX_NMRR:
#ifdef ARM_CPU_CORTEX_R   
        return reg;
#else    
        return read_nmrr();
#endif        
    case CTX_CIDR:
        return read_contextidr();
    case CTX_TPIDRPRW:
        return read_tpidrprw();
    case CTX_FPEXC:
        return reg;
    case CTX_LRsvc:
        return get_lr_svc();
    case CTX_SPsvc:
        return get_sp_svc();
    case CTX_LRabt:
        return get_lr_abt();
    case CTX_SPabt:
        return get_sp_abt();
    case CTX_LRund:
        return get_lr_und();
    case CTX_SPund:
        return get_sp_und();
    case CTX_LRirq:
        return get_lr_irq();
    case CTX_SPirq:
        return get_sp_irq();
    case CTX_LRfiq:
        return get_lr_fiq();
    case CTX_SPfiq:
        return get_sp_fiq();
    case CTX_R8fiq:
        return get_r8_fiq();
    case CTX_R9fiq:
        return get_r9_fiq();
    case CTX_R10fiq:
        return get_r10_fiq();
    case CTX_R11fiq:
        return get_r11_fiq();
    case CTX_R12fiq:
        return get_r12_fiq();
    case CTX_SPSRsvc:
        return get_spsr_svc();
    case CTX_SPSRabt:
        return get_spsr_abt();
    case CTX_SPSRund:
        return get_spsr_und();
    case CTX_SPSRirq:
        return get_spsr_irq();
    case CTX_SPSRfiq:
        return get_spsr_fiq();
    case CTX_CNTV_CTL:
        return get_cntv_ctl();
    case CTX_CNTV_CVALhigh:
        return get_cntv_cval_high();
    case CTX_CNTV_CVALlow:
        return get_cntv_cval_low();
    case CTX_CNTV_OFFhigh:
        return get_cntv_off_high();
    case CTX_CNTV_OFFlow:
        return get_cntv_off_low();
    case CTX_VMPIDR:
        return get_vmpidr();
    default:
	    return reg;
    }
}

void vcpu_hw_write_reg(uint32_t reg_index, uint32_t reg)
{
    switch (reg_index) {
    case CTX_SCTLR:
        write_sctlr(reg);
        break;
    case CTX_ACTLR:
        write_actlr(reg);
        break;
    case CTX_TTBCR:
#ifdef ARM_CPU_CORTEX_R   
        break;
#else  
        write_ttbcr(reg);
#endif         
        break;
    case CTX_TTBR0:
#ifdef ARM_CPU_CORTEX_R  
        break;
#else   
        write_ttbr0(reg);
#endif         
        break;
    case CTX_TTBR1:
#ifdef ARM_CPU_CORTEX_R  
        break;
#else   
        write_ttbr1(reg);
#endif         
        break;
    case CTX_DACR:
        write_dacr(reg);
        break;
    case CTX_DFSR:
        write_dfsr(reg);
        break;
    case CTX_IFSR:
        write_ifsr(reg);
        break;
    case CTX_ADFSR:
        write_adfsr(reg);
        break;
    case CTX_AIFSR:
        write_aifsr(reg);
        break;
    case CTX_DFAR:
        write_dfar(reg);
        break;
    case CTX_IFAR:
        write_ifar(reg);
        break;
    case CTX_PRRR:
#ifdef ARM_CPU_CORTEX_R
        break;
#else     
        write_prrr(reg);
#endif         
        break;
    case CTX_NMRR:
#ifdef ARM_CPU_CORTEX_R    
        break;
#else 
        write_nmrr(reg);
#endif         
        break;
    case CTX_CIDR:
        write_contextidr(reg);
        break;
    case CTX_TPIDRPRW:
        write_tpidrprw(reg);
        break;
    case CTX_FPEXC:
        break;
    case CTX_LRsvc:
        set_lr_svc(reg);
        break;
    case CTX_SPsvc:
        set_sp_svc(reg);
        break;
    case CTX_LRabt:
        set_lr_abt(reg);
        break;
    case CTX_SPabt:
        set_sp_abt(reg);
        break;
    case CTX_LRund:
        set_lr_und(reg);
        break;
    case CTX_SPund:
        set_sp_und(reg);
        break;
    case CTX_LRirq:
        set_lr_irq(reg);
        break;
    case CTX_SPirq:
        set_sp_irq(reg);
        break;
    case CTX_LRfiq:
        set_lr_fiq(reg);
        break;
    case CTX_SPfiq:
        set_sp_fiq(reg);
        break;
    case CTX_R8fiq:
        set_r8_fiq(reg);
        break;
    case CTX_R9fiq:
        set_r9_fiq(reg);
        break;
    case CTX_R10fiq:
        set_r10_fiq(reg);
        break;
    case CTX_R11fiq:
        set_r11_fiq(reg);
        break;
    case CTX_R12fiq:
        set_r12_fiq(reg);
        break;
    case CTX_SPSRsvc:
        set_spsr_svc(reg);
        break;
    case CTX_SPSRabt:
        set_spsr_abt(reg);
        break;
    case CTX_SPSRund:
        set_spsr_und(reg);
        break;
    case CTX_SPSRirq:
        set_spsr_irq(reg);
        break;
    case CTX_SPSRfiq:
        set_spsr_fiq(reg);
        break;
    case CTX_CNTV_CTL:
        set_cntv_ctl(reg);
        break;
    case CTX_CNTV_CVALhigh:
        set_cntv_cval_high(reg);
        break;
    case CTX_CNTV_CVALlow:
        set_cntv_cval_low(reg);
        break;
    case CTX_CNTV_OFFhigh:
        set_cntv_off_high(reg);
        break;
    case CTX_CNTV_OFFlow:
        set_cntv_off_low(reg);
        break;
    case CTX_VMPIDR:
        set_vmpidr(reg);
        break;
    default:
	    break;
    }
}
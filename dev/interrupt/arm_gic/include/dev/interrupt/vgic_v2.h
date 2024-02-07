#pragma once

#include <sys/types.h>

#define GIC_VCPU_MAX_NUM_LR 64

/* Helpers for VGIC */
#define VGIC_HCR_EOI_INVALID_COUNT(hcr) (((hcr) >> 27) & 0x1f)
#define VGIC_HCR_VGRP1DIE               (1U << 7)
#define VGIC_HCR_VGRP1EIE               (1U << 6)
#define VGIC_HCR_VGRP0DIE               (1U << 5)
#define VGIC_HCR_VGRP0EIE               (1U << 4)
#define VGIC_HCR_NPIE                   (1U << 3)
#define VGIC_HCR_LRENPIE                (1U << 2)
#define VGIC_HCR_UIE                    (1U << 1)
#define VGIC_HCR_EN                     (1U << 0)
#define VGIC_MISR_VGRP1D                VGIC_HCR_VGRP1DIE
#define VGIC_MISR_VGRP1E                VGIC_HCR_VGRP1EIE
#define VGIC_MISR_VGRP0D                VGIC_HCR_VGRP0DIE
#define VGIC_MISR_VGRP0E                VGIC_HCR_VGRP0EIE
#define VGIC_MISR_NP                    VGIC_HCR_NPIE
#define VGIC_MISR_LRENP                 VGIC_HCR_LRENPIE
#define VGIC_MISR_U                     VGIC_HCR_UIE
#define VGIC_MISR_EOI                   VGIC_HCR_EN
#define VGIC_VTR_NLISTREGS(vtr)         ((((vtr) >>  0) & 0x3f) + 1)
#define VGIC_VTR_NPRIOBITS(vtr)         ((((vtr) >> 29) & 0x07) + 1)
#define VGIC_VTR_NPREBITS(vtr)          ((((vtr) >> 26) & 0x07) + 1)

/* GIC VCPU Control Interface */
struct gich_vcpu_ctrl_map {
    uint32_t hcr;    /* 0x000 RW 0x00000000 Hypervisor Control Register */
    uint32_t vtr;    /* 0x004 RO IMPLEMENTATION DEFINED VGIC Type Register */
    /* Save restore on VCPU switch */
    uint32_t vmcr;   /* 0x008 RW IMPLEMENTATION DEFINED Virtual Machine Control Register */
    uint32_t res1[1];
    /* IRQ pending flags */
    uint32_t misr;   /* 0x010 RO 0x00000000 Maintenance Interrupt Status Register */
    uint32_t res2[3];
    /* Bitfield of list registers that have EOI */
    uint32_t eisr0;  /* 0x020 RO 0x00000000 End of Interrupt Status Registers 0 and 1, see EISRn */
    uint32_t eisr1;  /* 0x024 RO 0x00000000 */
    uint32_t res3[2];
    /* Bitfield of list registers that are empty */
    uint32_t elsr0;  /* 0x030 RO IMPLEMENTATION DEFINED a */
    uint32_t elsr1;  /* 0x034 RO IMPLEMENTATION DEFINED a Empty List Register Status Registers 0 and 1, see ELRSRn */
    uint32_t res4[46];
    /* Active priority: bitfield of active priorities */
    uint32_t apr;    /* 0x0F0 RW 0x00000000 Active Priorities Register */
    uint32_t res5[3];
    uint32_t lr[64]; /* 0x100 RW 0x00000000 List Registers 0-63, see LRn */
};

extern volatile struct gich_vcpu_ctrl_map *gic_vcpu_ctrl;
extern unsigned int gic_vcpu_num_list_regs;

static inline uint32_t get_gic_vcpu_ctrl_hcr(void)
{
    return gic_vcpu_ctrl->hcr;
}

static inline void set_gic_vcpu_ctrl_hcr(uint32_t hcr)
{
    gic_vcpu_ctrl->hcr = hcr;
}

static inline uint32_t get_gic_vcpu_ctrl_vmcr(void)
{
    return gic_vcpu_ctrl->vmcr;
}

static inline void set_gic_vcpu_ctrl_vmcr(uint32_t vmcr)
{
    gic_vcpu_ctrl->vmcr = vmcr;
}

static inline uint32_t get_gic_vcpu_ctrl_apr(void)
{
    return gic_vcpu_ctrl->apr;
}

static inline void set_gic_vcpu_ctrl_apr(uint32_t apr)
{
    gic_vcpu_ctrl->apr = apr;
}

static inline uint32_t get_gic_vcpu_ctrl_vtr(void)
{
    return gic_vcpu_ctrl->vtr;
}

static inline uint32_t get_gic_vcpu_ctrl_eisr0(void)
{
    return gic_vcpu_ctrl->eisr0;
}

static inline uint32_t get_gic_vcpu_ctrl_eisr1(void)
{
    return gic_vcpu_ctrl->eisr1;
}

static inline uint32_t get_gic_vcpu_ctrl_misr(void)
{
    return gic_vcpu_ctrl->misr;
}

static inline virq_t get_gic_vcpu_ctrl_lr(int num)
{
    virq_t virq;
    virq.words[0] = gic_vcpu_ctrl->lr[num];
    return virq;
}

static inline void set_gic_vcpu_ctrl_lr(int num, virq_t lr)
{
    gic_vcpu_ctrl->lr[num] = lr.words[0];
}



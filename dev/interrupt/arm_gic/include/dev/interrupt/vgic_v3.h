#pragma once

#include <sys/types.h>
#include <assert.h>
#include <arch/arch_helpers.h>

#define GIC_VCPU_MAX_NUM_LR 16

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


static inline uint32_t get_gic_vcpu_ctrl_hcr(void)
{
    uint32_t reg;
    reg = read_ich_hcr_el2();
    return reg;
}

static inline void set_gic_vcpu_ctrl_hcr(uint32_t hcr)
{
    write_ich_hcr_el2(hcr);
}

static inline uint32_t get_gic_vcpu_ctrl_vmcr(void)
{
    uint32_t reg;
    reg = read_ich_vmcr_el2();
    return reg;
}

static inline void set_gic_vcpu_ctrl_vmcr(uint32_t vmcr)
{
    write_ich_vmcr_el2(vmcr);
}

/* Note: On the GICv3 there are potentially up to 128 preemption
 * levels, and as such up to 4 APR registers.
 *
 * At this point the code assumes a maximum of 32 preemption levels.
 */
static inline uint32_t get_gic_vcpu_ctrl_apr(void)
{
    uint32_t reg;
    reg = read_ich_ap0r0_el2();
    return reg;
}

static inline void set_gic_vcpu_ctrl_apr(uint32_t apr)
{
    write_ich_ap0r0_el2(apr);
}

static inline uint32_t get_gic_vcpu_ctrl_vtr(void)
{
    uint32_t reg;
    reg = read_ich_vtr_el2();
    return reg;
}

static inline uint32_t get_gic_vcpu_ctrl_eisr0(void)
{
    uint32_t reg;
    reg = read_ich_eisr_el2();
    return reg;
}

/* Note: GICv3 supports a maximum of 16 list registers
 * so there is no need to support EISR1 on GICv3.
 */
static inline uint32_t get_gic_vcpu_ctrl_eisr1(void)
{
    return 0;
}

static inline uint32_t get_gic_vcpu_ctrl_misr(void)
{
    uint32_t reg;
    reg = read_ich_misr_el2();
    return reg;
}

static inline virq_t get_gic_vcpu_ctrl_lr(int num)
{
    virq_t virq;
    uint64_t reg = 0;
    switch (num) {
    case 0:
        reg = read_ich_lr0_el2();
        break;
    case 1:
        reg = read_ich_lr1_el2();
        break;
    case 2:
        reg = read_ich_lr2_el2();
        break;
    case 3:
        reg = read_ich_lr3_el2();
        break;
    case 4:
        reg = read_ich_lr4_el2();
        break;
    case 5:
        reg = read_ich_lr5_el2();
        break;
    case 6:
        reg = read_ich_lr6_el2();
        break;
    case 7:
        reg = read_ich_lr7_el2();
        break;
    case 8:
        reg = read_ich_lr8_el2();
        break;
    case 9:
        reg = read_ich_lr9_el2();
        break;
    case 10:
        reg = read_ich_lr10_el2();
        break;
    case 11:
        reg = read_ich_lr11_el2();
        break;
    case 12:
        reg = read_ich_lr12_el2();
        break;
    case 13:
        reg = read_ich_lr13_el2();
        break;
    case 14:
        reg = read_ich_lr14_el2();
        break;
    case 15:
        reg = read_ich_lr15_el2();
        break;
    default:
        DEBUG_ASSERT_MSG(0, "gicv3: invalid lr");
    }
    virq.words[0] = reg;
    return virq;
}

static inline void set_gic_vcpu_ctrl_lr(int num, virq_t lr)
{
    uint64_t reg = lr.words[0];
    switch (num) {
    case 0:
        write_ich_lr0_el2(reg);
        break;
    case 1:
        write_ich_lr1_el2(reg);
        break;
    case 2:
        write_ich_lr2_el2(reg);
        break;
    case 3:
        write_ich_lr3_el2(reg);
        break;
    case 4:
        write_ich_lr4_el2(reg);
        break;
    case 5:
        write_ich_lr5_el2(reg);
        break;
    case 6:
        write_ich_lr6_el2(reg);
        break;
    case 7:
        write_ich_lr7_el2(reg);
        break;
    case 8:
        write_ich_lr8_el2(reg);
        break;
    case 9:
        write_ich_lr9_el2(reg);
        break;
    case 10:
        write_ich_lr10_el2(reg);
        break;
    case 11:
        write_ich_lr11_el2(reg);
        break;
    case 12:
        write_ich_lr12_el2(reg);
        break;
    case 13:
        write_ich_lr13_el2(reg);
        break;
    case 14:
        write_ich_lr14_el2(reg);
        break;
    case 15:
        write_ich_lr15_el2(reg);
        break;
    default:
        DEBUG_ASSERT_MSG(0, "gicv3: invalid lr");
    }
}

extern unsigned int gic_vcpu_num_list_regs;

/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#pragma once

#include <arch/arch_thread.h>
#include <kern/utils.h>
#include <stdio.h>

#define HCR_RW       BIT_32(31)     /* Execution state control        [v8r RES]*/
#define HCR_TRVM     BIT_32(30)     /* trap reads of VM controls      */
#define HCR_HCD      BIT_32(29)     /* Disable HVC                    */
#define HCR_TDZ      BIT_32(28)     /* trap DC ZVA AArch64 only       [v8r RES]*/
#define HCR_TGE      BIT_32(27)     /* Trap general exceptions        */
#define HCR_TVM      BIT_32(26)     /* Trap MMU access                */
#define HCR_TTLB     BIT_32(25)     /* Trap TLB operations            [v8r RES]*/
#define HCR_TPU      BIT_32(24)     /* Trap cache maintenance         */
#define HCR_TPC      BIT_32(23)     /* Trap cache maintenance PoC     */
#define HCR_TSW      BIT_32(22)     /* Trap cache maintenance set/way */
#define HCR_TCACHE   (HCR_TPU | HCR_TPC | HCR_TSW)
#define HCR_TAC      BIT_32(21)     /* Trap ACTLR access              */
#define HCR_TIDCP    BIT_32(20)     /* Trap lockdown                  */
#define HCR_TSC      BIT_32(19)     /* Trap SMC instructions          [v8r RES]*/
#define HCR_TID3     BIT_32(18)     /* Trap ID register 3             */
#define HCR_TID2     BIT_32(17)     /* Trap ID register 2             */
#define HCR_TID1     BIT_32(16)     /* Trap ID register 1             */
#define HCR_TID0     BIT_32(15)     /* Trap ID register 0             */
#define HCR_TID      (HCR_TID0 | HCR_TID1 | HCR_TID2 | HCR_TID3)
#define HCR_TWE      BIT_32(14)     /* Trap WFE                       */
#define HCR_TWI      BIT_32(13)     /* Trap WFI                       */
#define HCR_DC       BIT_32(12)     /* Default cacheable              */
#define HCR_BSU(x)   ((x) << 10) /* Barrier sharability upgrade    */
#define HCR_FB       BIT_32( 9)     /* Force broadcast                */
#define HCR_VA       BIT_32( 8)     /* Virtual async abort            */
#define HCR_VI       BIT_32( 7)     /* Virtual IRQ                    */
#define HCR_VF       BIT_32( 6)     /* Virtual FIRQ                   */
#define HCR_AMO      BIT_32( 5)     /* CPSR.A override enable         */
#define HCR_IMO      BIT_32( 4)     /* CPSR.I override enable         */
#define HCR_FMO      BIT_32( 3)     /* CPSR.F override enable         */
#define HCR_PTW      BIT_32( 2)     /* Protected table walk           [v8r RES]*/
#define HCR_SWIO     BIT_32( 1)     /* set/way invalidate override    */
#define HCR_VM       BIT_32( 0)     /* Virtualization MMU enable      [v8r mpu]*/


extern uint32_t vcpu_hw_read_reg(uint32_t reg_index);
extern void vcpu_hw_write_reg(uint32_t reg_index, uint32_t reg);


static inline void vcpu_save_reg(pcb_user_ctx_t *pcpu, uint32_t reg)
{
    if (reg >= CTX_REGS_END || pcpu == NULL) {
        printf("ARM/HYP: Invalid register index or NULL VCPU");
        return;
    }
    write_ctx_reg(get_sysregs_ctx(&pcpu->pcpu), reg, vcpu_hw_read_reg(reg));
}

static inline void vcpu_save_reg_range(pcb_user_ctx_t *pcpu, uint32_t start, uint32_t end)
{
    for (uint32_t i = start; i <= end; i++) {
        vcpu_save_reg(pcpu, i);
    }
}

static inline void vcpu_restore_reg(pcb_user_ctx_t *pcpu, uint32_t reg)
{
    if (reg >= CTX_REGS_END || pcpu == NULL) {
        printf("ARM/HYP: Invalid register index or NULL VCPU");
        return;
    }
    vcpu_hw_write_reg(reg, read_ctx_reg(get_sysregs_ctx(&pcpu->pcpu), reg));
}

static inline void vcpu_restore_reg_range(pcb_user_ctx_t *pcpu, uint32_t start, uint32_t end)
{
    for (uint32_t i = start; i <= end; i++) {
        vcpu_restore_reg(pcpu, i);
    }
}

static inline uint32_t vcpu_read_reg(pcb_user_ctx_t *pcpu, uint32_t reg)
{
    if (reg >= CTX_REGS_END || pcpu == NULL) {
        printf("ARM/HYP: Invalid register index or NULL VCPU");
        return 0;
    }
    return read_ctx_reg(get_sysregs_ctx(&pcpu->pcpu), reg);
}

static inline uint32_t *vcpu_read_reg_ptr(pcb_user_ctx_t *pcpu, uint32_t reg)
{
    if (reg >= CTX_REGS_END || pcpu == NULL) {
        printf("ARM/HYP: Invalid register index or NULL VCPU");
        return 0;
    }
    return &read_ctx_reg(get_sysregs_ctx(&pcpu->pcpu), reg);
}

static inline void vcpu_write_reg(pcb_user_ctx_t *pcpu, uint32_t reg, uint32_t value)
{
    if (reg >= CTX_REGS_END || pcpu == NULL) {
        printf("ARM/HYP: Invalid register index or NULL VCPU");
        return;
    }
    write_ctx_reg(get_sysregs_ctx(&pcpu->pcpu), reg, value);
}

void vcpu_boot_init(void);
void vcpu_save(pcb_user_ctx_t *pcpu, bool active);
void vcpu_restore(pcb_user_ctx_t *pcpu);
void vcpu_switch(pcb_user_ctx_t *old, pcb_user_ctx_t *new);
void vcpu_init(pcb_user_ctx_t *pcpu);
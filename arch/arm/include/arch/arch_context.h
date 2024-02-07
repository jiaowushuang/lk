/*
 * Copyright (c) 2016-2020, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <kern/utils.h>

/*******************************************************************************
 * Constants that allow assembler code to access members of and the 'regs'
 * structure at their correct offsets.
 ******************************************************************************/
#define CTX_REGS_OFFSET			U(0x0)
#define CTX_SCTLR			U(0x0)
#define CTX_ACTLR			U(0x4)
#define CTX_TTBCR			U(0x8)
#define CTX_TTBR0			U(0xc)
#define CTX_TTBR1			U(0x10)
#define CTX_DACR			U(0x14)
#define CTX_DFSR			U(0x18)
#define CTX_IFSR			U(0x1c)
#define CTX_ADFSR			U(0x20)
#define CTX_AIFSR			U(0x24)
#define CTX_DFAR			U(0x28)
#define CTX_IFAR			U(0x2c)
#define CTX_PRRR			U(0x30)
#define CTX_NMRR			U(0x34)
#define CTX_CIDR			U(0x38)
#define CTX_TPIDRPRW			U(0x3c)
#define CTX_FPEXC			U(0x40)
#define CTX_LRsvc			U(0x44)
#define CTX_SPsvc			U(0x48)
#define CTX_LRabt			U(0x4c)
#define CTX_SPabt			U(0x50)
#define CTX_LRund			U(0x54)
#define CTX_SPund			U(0x58)
#define CTX_LRirq			U(0x5c)
#define CTX_SPirq			U(0x60)
#define CTX_LRfiq			U(0x64)
#define CTX_SPfiq			U(0x68)
#define CTX_R8fiq			U(0x6c)
#define CTX_R9fiq			U(0x70)
#define CTX_R10fiq			U(0x74)
#define CTX_R11fiq			U(0x78)
#define CTX_R12fiq			U(0x7c)
#define CTX_VMPIDR			U(0x80)
#define CTX_SPSRsvc			U(0x84)
#define CTX_SPSRabt			U(0x88)
#define CTX_SPSRund			U(0x8c)
#define CTX_SPSRirq			U(0x90)
#define CTX_SPSRfiq			U(0x94)
#define CTX_CNTV_CTL			U(0x98)
#define CTX_CNTV_CVALhigh		U(0x9c)
#define CTX_CNTV_CVALlow		U(0x100)
#define CTX_CNTV_OFFhigh		U(0x104)
#define CTX_CNTV_OFFlow			U(0x108)
#define CTX_VPIDR			U(0x10c)
#define CTX_REGS_END			U(0x200)

/* These are offsets to registers in gp_reg_t */
#define CTX_GPREG_R0	U(0x0)
#define CTX_GPREG_R1	U(0x4)
#define CTX_GPREG_R2	U(0x8)
#define CTX_GPREG_R3	U(0xC)
#define CTX_GPREG_R4	U(0x10)
#define CTX_GPREG_R5	U(0x14)
#define CTX_GPREG_R6	U(0x18)
#define CTX_GPREG_R7	U(0x1C)
#define CTX_GPREG_R8	U(0x20)
#define CTX_GPREG_R9	U(0x24)
#define CTX_GPREG_R10	U(0x28)
#define CTX_GPREG_R11	U(0x2C)
#define CTX_GPREG_R12	U(0x30)

#define CTX_SP_USR	U(0x34)
#define CTX_LR_USR	U(0x38)

#define CTX_SP_SVC	U(0x3C)
#define CTX_LR_SVC	U(0x40)
#define CTX_SPSR_SVC	U(0x44)

#define CTX_SP_HYP	U(0x48)
#define CTX_LR_HYP	U(0x4C)
#define CTX_SPSR_HYP	U(0x50)

#define CTX_SP_IRQ	U(0x54)
#define CTX_LR_IRQ	U(0x58)
#define CTX_SPSR_IRQ	U(0x5C)

#define CTX_SP_FIQ	U(0x60)
#define CTX_LR_FIQ	U(0x64)
#define CTX_SPSR_FIQ	U(0x68)

#define CTX_SP_ABT	U(0x6C)
#define CTX_LR_ABT	U(0x70)
#define CTX_SPSR_ABT	U(0x74)

#define CTX_SP_UND	U(0x78)
#define CTX_LR_UND	U(0x7C)
#define CTX_SPSR_UND	U(0x80)

#define CTX_SP_MON	U(0x84)
#define CTX_LR_MON	U(0x88)
#define CTX_SPSR_MON	U(0x8C)

#define CTX_SIZE	U(0x90)

#ifndef __ASSEMBLER__

#include <stdint.h>

#include <cassert.h>

/*
 * Common constants to help define the 'pcpu_context' structure and its
 * members below.
 */
#define WORD_SHIFT		U(2)
#define DEFINE_REG_STRUCT(name, num_regs)	\
	typedef struct name {			\
		uint32_t ctx_regs[num_regs];	\
	}  __aligned(8) name##_t

/* Constants to determine the size of individual context structures */
#define CTX_REG_ALL		(CTX_REGS_END >> WORD_SHIFT)

DEFINE_REG_STRUCT(sysregs, CTX_REG_ALL);

#undef CTX_REG_ALL

#define read_ctx_reg(ctx, offset)	((ctx)->ctx_regs[offset >> WORD_SHIFT])
#define write_ctx_reg(ctx, offset, val)	(((ctx)->ctx_regs[offset >> WORD_SHIFT]) \
					 = val)
typedef struct pcpu_context {
	sysregs_t regs_ctx;
} pcpu_context_t;


/* Macros to access members of the 'pcpu_context_t' structure */
#define get_sysregs_ctx(h)		(&((pcpu_context_t *) h)->regs_ctx)

/*
 * Compile time assertions related to the 'pcpu_context' structure to
 * ensure that the assembler and the compiler view of the offsets of
 * the structure members is the same.
 */
CASSERT(CTX_REGS_OFFSET == __builtin_offsetof(pcpu_context_t, regs_ctx), \
	assert_core_context_regs_offset_mismatch);

/*
 * The generic structure to save arguments and callee saved registers during
 * an CALL. Also this structure is used to store the result return values after
 * the completion of CALL service.
 */
typedef struct gp_reg {
	u_register_t r0;
	u_register_t r1;
	u_register_t r2;
	u_register_t r3;
	u_register_t r4;
	u_register_t r5;
	u_register_t r6;
	u_register_t r7;
	u_register_t r8;
	u_register_t r9;
	u_register_t r10;
	u_register_t r11;
	u_register_t r12;
	
	u_register_t sp_usr;
	u_register_t lr_usr;
	/* spsr_usr doesn't exist */

	u_register_t sp_svc;
	u_register_t lr_svc;
	u_register_t spsr_svc;
	
	u_register_t sp_hvc;
	u_register_t lr_hvc;
	u_register_t spsr_hvc;

	u_register_t sp_irq;
	u_register_t lr_irq;
	u_register_t spsr_irq;

	u_register_t sp_fiq;
	u_register_t lr_fiq;
	u_register_t spsr_fiq;

	u_register_t sp_abt;
	u_register_t lr_abt;
	u_register_t spsr_abt;

	u_register_t sp_und;
	u_register_t lr_und;
	u_register_t spsr_und;

	/*
	 * `sp_mon` will point to the C runtime stack in monitor mode. But prior
	 * to exit from CALL, this will point to the `gp_reg_t` so that
	 * on next entry due to CALL, the `gp_reg_t` can be easily accessed.
	 */
	u_register_t sp_mon;
	u_register_t lr_mon;
	u_register_t spsr_mon;
	
	/*
	 * The workaround for CVE-2017-5715 requires storing information in
	 * the bottom 3 bits of the stack pointer.  Add a padding field to
	 * force the size of the struct to be a multiple of 8.
	 */
} gp_reg_t __aligned(8);

#if CTX_INCLUDE_FPREGS
#define CTX_FP_Q0		U(0x0)
#define CTX_FP_Q1		U(0x8)
#define CTX_FP_Q2		U(0x10)
#define CTX_FP_Q3		U(0x18)
#define CTX_FP_Q4		U(0x20)
#define CTX_FP_Q5		U(0x28)
#define CTX_FP_Q6		U(0x30)
#define CTX_FP_Q7		U(0x38)
#define CTX_FP_Q8		U(0x40)
#define CTX_FP_Q9		U(0x48)
#define CTX_FP_Q10		U(0x50)
#define CTX_FP_Q11		U(0x58)
#define CTX_FP_Q12		U(0x60)
#define CTX_FP_Q13		U(0x68)
#define CTX_FP_Q14		U(0x70)
#define CTX_FP_Q15		U(0x78)
#define CTX_FP_Q16		U(0x80)
#define CTX_FP_Q17		U(0x88)
#define CTX_FP_Q18		U(0x90)
#define CTX_FP_Q19		U(0x98)
#define CTX_FP_Q20		U(0x100)
#define CTX_FP_Q21		U(0x108)
#define CTX_FP_Q22		U(0x110)
#define CTX_FP_Q23		U(0x118)
#define CTX_FP_Q24		U(0x120)
#define CTX_FP_Q25		U(0x128)
#define CTX_FP_Q26		U(0x130)
#define CTX_FP_Q27		U(0x138)
#define CTX_FP_Q28		U(0x140)
#define CTX_FP_Q29		U(0x148)
#define CTX_FP_Q30		U(0x150)
#define CTX_FP_Q31		U(0x158)
#define CTX_FP_FPSR		U(0x160)
#define CTX_FP_FPCR		U(0x164)
#define CTX_FPREGS_END		U(0x168)
#endif

#define WORD_SHIFT		U(2)

#if CTX_INCLUDE_FPREGS
# define CTX_FPREG_ALL		(CTX_FPREGS_END >> WORD_SHIFT)
#endif

#if CTX_INCLUDE_FPREGS
DEFINE_REG_STRUCT(fp_regs, CTX_FPREG_ALL);
#endif

typedef struct tcpu_context {
	gp_reg_t gpregs_ctx;	
#if CTX_INCLUDE_FPREGS
	fp_regs_t fpregs_ctx;
#endif	

} tcpu_context_t;

/*
 * Compile time assertions related to the 'smc_context' structure to
 * ensure that the assembler and the compiler view of the offsets of
 * the structure members is the same.
 */
CASSERT(CTX_GPREG_R0 == __builtin_offsetof(gp_reg_t, r0), \
	assert_gp_reg_greg_r0_offset_mismatch);
CASSERT(CTX_GPREG_R1 == __builtin_offsetof(gp_reg_t, r1), \
	assert_gp_reg_greg_r1_offset_mismatch);
CASSERT(CTX_GPREG_R2 == __builtin_offsetof(gp_reg_t, r2), \
	assert_gp_reg_greg_r2_offset_mismatch);
CASSERT(CTX_GPREG_R3 == __builtin_offsetof(gp_reg_t, r3), \
	assert_gp_reg_greg_r3_offset_mismatch);
CASSERT(CTX_GPREG_R4 == __builtin_offsetof(gp_reg_t, r4), \
	assert_gp_reg_greg_r4_offset_mismatch);
CASSERT(CTX_SP_USR == __builtin_offsetof(gp_reg_t, sp_usr), \
	assert_gp_reg_sp_usr_offset_mismatch);
CASSERT(CTX_LR_MON == __builtin_offsetof(gp_reg_t, lr_mon), \
	assert_gp_reg_lr_mon_offset_mismatch);
CASSERT(CTX_SPSR_MON == __builtin_offsetof(gp_reg_t, spsr_mon), \
	assert_gp_reg_spsr_mon_offset_mismatch);

CASSERT((sizeof(gp_reg_t) & 0x7U) == 0U, assert_gp_reg_not_aligned);
CASSERT(CTX_SIZE == sizeof(gp_reg_t), assert_gp_reg_size_mismatch);

/* Convenience macros to return from CALL handler */
#define CALL_RET0(_h) {				\
	return (uintptr_t)(_h); 		\
}
#define CALL_RET1(_h, _r0) {			\
	((gp_reg_t *)(_h))->r0 = (_r0);	\
	CALL_RET0(_h);				\
}
#define CALL_RET2(_h, _r0, _r1) {		\
	((gp_reg_t *)(_h))->r1 = (_r1);	\
	CALL_RET1(_h, (_r0));			\
}
#define CALL_RET3(_h, _r0, _r1, _r2) {		\
	((gp_reg_t *)(_h))->r2 = (_r2);	\
	CALL_RET2(_h, (_r0), (_r1));		\
}
#define CALL_RET4(_h, _r0, _r1, _r2, _r3) {	\
	((gp_reg_t *)(_h))->r3 = (_r3);	\
	CALL_RET3(_h, (_r0), (_r1), (_r2));	\
}
#define CALL_RET5(_h, _r0, _r1, _r2, _r3, _r4) {	\
	((gp_reg_t *)(_h))->r4 = (_r4);	\
	CALL_RET4(_h, (_r0), (_r1), (_r2), (_r3));	\
}
#define CALL_RET6(_h, _r0, _r1, _r2, _r3, _r4, _r5) {	\
	((gp_reg_t *)(_h))->r5 = (_r5);	\
	CALL_RET5(_h, (_r0), (_r1), (_r2), (_r3), (_r4));	\
}
#define CALL_RET7(_h, _r0, _r1, _r2, _r3, _r4, _r5, _r6) {	\
	((gp_reg_t *)(_h))->r6 = (_r6);	\
	CALL_RET6(_h, (_r0), (_r1), (_r2), (_r3), (_r4), (_r5)); \
}
#define CALL_RET8(_h, _r0, _r1, _r2, _r3, _r4, _r5, _r6, _r7) {	\
	((gp_reg_t *)(_h))->r7 = (_r7);	\
	CALL_RET7(_h, (_r0), (_r1), (_r2), (_r3), (_r4), (_r5), (_r6));	\
}

/*
 * Helper macro to retrieve the CALL parameters from gp_reg_t.
 */
#define get_call_params_from_ctx(_hdl, _r1, _r2, _r3, _r4) {	\
		_r1 = ((gp_reg_t *)_hdl)->r1;		\
		_r2 = ((gp_reg_t *)_hdl)->r2;		\
		_r3 = ((gp_reg_t *)_hdl)->r3;		\
		_r4 = ((gp_reg_t *)_hdl)->r4;		\
		}


#endif /* __ASSEMBLER__ */


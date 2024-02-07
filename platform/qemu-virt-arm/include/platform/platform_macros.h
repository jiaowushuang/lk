/*
 * Copyright (c) 2016, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once
#include <kern/utils.h>
#include <arch/asm_macros.h>
#include <arch/arch_macro.h>
#include <platform/gic.h>

.section .rodata.gic_reg_name, "aS"
/* Applicable only to GICv2 and GICv3 with SRE disabled (legacy mode) */
gicc_regs:
	.asciz "gicc_hppir", "gicc_ahppir", "gicc_ctlr", ""

/* Applicable only to GICv3 with SRE enabled */
icc_regs:
	.asciz "icc_hppir0_el1", "icc_hppir1_el1", "icc_ctlr_el3", ""

/* Registers common to both GICv2 and GICv3 */
gicd_pend_reg:
	.asciz "gicd_ispendr regs (Offsets 0x200-0x278)\nOffset\t\t\tValue\n"
newline:
	.asciz "\n"
spacer:
	.asciz ":\t\t 0x"
prefix:
	.asciz "0x"

/* ICC_SRE bit definitions */
#define ICC_SRE_EN_BIT		BIT_32(3)
#define ICC_SRE_DIB_BIT		BIT_32(2)
#define ICC_SRE_DFB_BIT		BIT_32(1)
#define ICC_SRE_SRE_BIT		BIT_32(0)

/* gicv2 */
/* Physical CPU Interface registers */
#define GICC_CTLR		U(0x0)
#define GICC_PMR		U(0x4)
#define GICC_BPR		U(0x8)
#define GICC_IAR		U(0xC)
#define GICC_EOIR		U(0x10)
#define GICC_RPR		U(0x14)
#define GICC_HPPIR		U(0x18)
#define GICC_AHPPIR		U(0x28)
#define GICC_IIDR		U(0xFC)
#define GICC_DIR		U(0x1000)
#define GICC_PRIODROP		GICC_EOIR

/*******************************************************************************
 * Common GIC Distributor interface register offsets
 ******************************************************************************/
#define GICD_CTLR		U(0x0)
#define GICD_TYPER		U(0x4)
#define GICD_IIDR		U(0x8)
#define GICD_IGROUPR		U(0x80)
#define GICD_ISENABLER		U(0x100)
#define GICD_ICENABLER		U(0x180)
#define GICD_ISPENDR		U(0x200)
#define GICD_ICPENDR		U(0x280)
#define GICD_ISACTIVER		U(0x300)
#define GICD_ICACTIVER		U(0x380)
#define GICD_IPRIORITYR		U(0x400)
#define GICD_ICFGR		U(0xc00)
#define GICD_NSACR		U(0xe00)

	/* ---------------------------------------------
	 * The below utility macro prints out relevant GIC
	 * registers whenever an unhandled exception is
	 * taken in BL31 on ARM standard platforms.
	 * Expects: GICD base in x16, GICC base in x17
	 * Clobbers: x0 - x10, sp
	 * ---------------------------------------------
	 */
	.macro arm_print_gic_regs
	/* Check for GICv3 system register access */
	mrs	x7, id_aa64pfr0_el1
	ubfx	x7, x7, #ID_AA64PFR0_GIC_SHIFT, #ID_AA64PFR0_GIC_WIDTH
	cmp	x7, #1
	b.ne	print_gicv2

	/* Check for SRE enable */
	mrs	x8, ICC_SRE_EL3
	tst	x8, #ICC_SRE_SRE_BIT
	b.eq	print_gicv2

	/* Load the icc reg list to x6 */
	adr	x6, icc_regs
	/* Load the icc regs to gp regs used by str_in_crash_buf_print */
	mrs	x8, ICC_HPPIR0_EL1
	mrs	x9, ICC_HPPIR1_EL1
	mrs	x10, ICC_CTLR_EL3
	/* Store to the crash buf and print to console */
	bl	str_in_crash_buf_print
	b	print_gic_common

print_gicv2:
	/* Load the gicc reg list to x6 */
	adr	x6, gicc_regs
	/* Load the gicc regs to gp regs used by str_in_crash_buf_print */
	ldr	w8, [x17, #GICC_HPPIR]
	ldr	w9, [x17, #GICC_AHPPIR]
	ldr	w10, [x17, #GICC_CTLR]
	/* Store to the crash buf and print to console */
	bl	str_in_crash_buf_print

print_gic_common:
	/* Print the GICD_ISPENDR regs */
	add	x7, x16, #GICD_ISPENDR
	adr	x4, gicd_pend_reg
	bl	asm_print_str
gicd_ispendr_loop:
	sub	x4, x7, x16
	cmp	x4, #0x280
	b.eq	exit_print_gic_regs

	/* Print "0x" */
	adr	x4, prefix
	bl	asm_print_str

	/* Print offset */
	sub	x4, x7, x16
	mov	x5, #12
	bl	asm_print_hex_bits

	adr	x4, spacer
	bl	asm_print_str

	ldr	x4, [x7], #8
	bl	asm_print_hex

	adr	x4, newline
	bl	asm_print_str
	b	gicd_ispendr_loop
exit_print_gic_regs:
	.endm

/* ---------------------------------------------
 * The below required platform porting macro
 * prints out relevant GIC and CCI registers
 * whenever an unhandled exception is taken.
 * Clobbers: x0 - x10, x16, x17, sp
 * ---------------------------------------------
 */
.macro plat_crash_print_regs
	mov_imm	x17, GICBASE(0) + GICC_OFFSET
	mov_imm	x16, GICBASE(0) + GICD_OFFSET
	arm_print_gic_regs
.endm


/*
 * Copyright (c) 2015-2020, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <kern/asm.h>
#include <arch/asm_macros.h>
#include <arch/arch_macro.h>
#include <arch/cpu_ops.h>
#include <platform/qemu-virt.h>

#define MIDR			U(0x410FD030)

	/* ---------------------------------------------
	 * int plat_crash_console_init(void)
	 * Function to initialize the crash console
	 * without a C Runtime to print crash report.
	 * Clobber list : x0, x1, x2
	 * ---------------------------------------------
	 */
FUNCTION(plat_crash_console_init) 
	mov_imm x0, CRASH_UART_BASE
	mov_imm x1, CRASH_UART_CLK_IN_HZ
	mov_imm x2, CONSOLE_BAUDRATE
	b	console_pl011_core_init

	/* ---------------------------------------------
	 * int plat_crash_console_putc(int c)
	 * Function to print a character on the crash
	 * console without a C Runtime.
	 * Clobber list : x1, x2
	 * ---------------------------------------------
	 */
FUNCTION(plat_crash_console_putc) 
	mov_imm	x1, CRASH_UART_BASE
	b	console_pl011_core_putc

	/* ---------------------------------------------
	 * void plat_crash_console_flush(int c)
	 * Function to force a write of all buffered
	 * data that hasn't been output.
	 * Out : void.
	 * Clobber list : x0, x1
	 * ---------------------------------------------
	 */
FUNCTION(plat_crash_console_flush) 
	mov_imm	x0, CRASH_UART_BASE
	b	console_pl011_core_flush

	/* -----------------------------------------------------
	 * void plat_panic_handler(void) __dead2;
	 * Endless loop by default.
	 * -----------------------------------------------------
	 */
FUNCTION(plat_panic_handler) 
	wfi
	b	plat_panic_handler

	/* ---------------------------------------------
	 * This function provides cortex_a53 specific
	 * register information for crash reporting.
	 * It needs to return with x6 pointing to
	 * a list of register names in ascii and
	 * x8 - x15 having values of registers to be
	 * reported.
	 * ---------------------------------------------
	 */
.section .rodata.cortex_regs, "aS"
cortex_regs:  /* The ascii list of register names to be reported */
	.asciz	"cpuectlr_el1", "cpumerrsr_el1", "l2merrsr_el1", \
		"cpuactlr_el1", ""

FUNCTION_SECTION(cortex_cpu_reg_dump) 
	adr	x6, cortex_regs
	mrs	x8, ECTLR_EL1
	mrs	x9, MERRSR_EL1
	mrs	x10, L2MERRSR_EL1
	mrs	x11, CPUACTLR_EL1
	ret


declare_cpu_ops cortex_a53, MIDR, \
	cortex_cpu_reg_dump

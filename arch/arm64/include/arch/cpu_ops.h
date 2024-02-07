#pragma once

#include <arch/arch_macro.h>
#include <arch/assert_macros.h>


#define CPU_IMPL_PN_MASK	(MIDR_IMPL_MASK << MIDR_IMPL_SHIFT) | \
				(MIDR_PN_MASK << MIDR_PN_SHIFT)

/* Word size for 64-bit CPUs */
#define CPU_WORD_SIZE			8


	.equ	CPU_MIDR_SIZE, CPU_WORD_SIZE
	.equ	CPU_REG_DUMP_SIZE, CPU_WORD_SIZE

#if !CRASH_REPORTING
	.equ	CPU_REG_DUMP_SIZE, 0
#endif

/*
 * Define the offsets to the fields in cpu_ops structure.
 * Every offset is defined based in the offset and size of the previous
 * field.
 */
	.equ	CPU_MIDR, 0
	.equ	CPU_REG_DUMP, CPU_MIDR + CPU_MIDR_SIZE
	.equ	CPU_OPS_SIZE, CPU_REG_DUMP + CPU_REG_DUMP_SIZE

	/*
	 * Declare CPU operations
	 *
	 * _name:
	 *	Name of the CPU for which operations are being specified
	 * _midr:
	 *	Numeric value expected to read from CPU's MIDR
	 */
	.macro declare_cpu_ops _name:req, _midr:req, _reg_dump:req
	.section .rodata.cpu_ops, "a"
	.align 3
	.type cpu_ops_\_name, %object
	.quad \_midr
#if CRASH_REPORTING
	.quad \_reg_dump
#endif
	.endm


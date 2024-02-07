#include <kern/asm.h>
#include <arch/asm_macros.h>
#include <arch/arch_macro.h>
#include <assym.h>
#include <arch/arch_context.h>
#include <platform/platform_macros.h>

#if CRASH_REPORTING

	/* ------------------------------------------------------
	 * The below section deals with dumping the system state
	 * when an unhandled exception is taken in EL3.
	 * The layout and the names of the registers which will
	 * be dumped during a unhandled exception is given below.
	 * ------------------------------------------------------
	 */
.section .rodata.crash_prints, "aS"
print_spacer:
	.asciz	"             = 0x"

gp_regs:
	.asciz	"x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7",\
		"x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15",\
		"x16", "x17", "x18", "x19", "x20", "x21", "x22",\
		"x23", "x24", "x25", "x26", "x27", "x28", "x29", ""
el3_sys_regs:
	.asciz	"scr_el3", "sctlr_el3", "cptr_el3", "tcr_el3",\
		"daif", "mair_el3", "spsr_el3", "elr_el3", "ttbr0_el3",\
		"esr_el3", "far_el3", ""

el01_sys_regs:
	.asciz	"spsr_el1", "elr_el1", "spsr_abt", "spsr_und",\
		"spsr_irq", "spsr_fiq", "sctlr_el1", "actlr_el1", "cpacr_el1",\
		"csselr_el1", "sp_el1", "esr_el1", "ttbr0_el1", "ttbr1_el1",\
		"mair_el1", "amair_el1", "tcr_el1", "tpidr_el1", "tpidr_el0",\
		"tpidrro_el0",  "par_el1", "mpidr_el1", "afsr0_el1", "afsr1_el1",\
		"contextidr_el1", "vbar_el1", "cntp_ctl_el0", "cntp_cval_el0",\
		"cntv_ctl_el0", "cntv_cval_el0", "cntkctl_el1", "sp_el0", "isr_el1", ""

#if CTX_INCLUDE_AARCH32_REGS
aarch32_regs:
	.asciz	"dacr32_el2", "ifsr32_el2", ""
#endif /* CTX_INCLUDE_AARCH32_REGS */

panic_msg:
	.asciz "PANIC in ELx.\nx30"
excpt_msg:
	.asciz "Unhandled Exception in ELx.\nx30"
intr_excpt_msg:
	.ascii "Unhandled Interrupt Exception in ELx.\n"
x30_msg:
	.asciz "x30"
excpt_msg_el:
	.asciz "Unhandled Exception from EL"


	/*
	 * Helper function to print from crash buf.
	 * The print loop is controlled by the buf size and
	 * ascii reg name list which is passed in x6. The
	 * function returns the crash buf address in x0.
	 * Clobbers : x0 - x7, sp
	 */
FUNCTION_SECTION(size_controlled_print)
	/* Save the lr */
	mov	sp, x30
	/* load the crash buf address */
	mrs	x7, TPIDR_ELX
test_size_list:
	/* Calculate x5 always as it will be clobbered by asm_print_hex */
	mrs	x5, TPIDR_ELX
	add	x5, x5, #PCPU_KDATA_CRASH_OFFSET
	/* Test whether we have reached end of crash buf */
	cmp	x7, x5
	b.eq	exit_size_print
	ldrb	w4, [x6]
	/* Test whether we are at end of list */
	cbz	w4, exit_size_print
	mov	x4, x6
	/* asm_print_str updates x4 to point to next entry in list */
	bl	asm_print_str
	/* x0 = number of symbols printed + 1 */
	sub	x0, x4, x6
	/* update x6 with the updated list pointer */
	mov	x6, x4
	bl	print_alignment
	ldr	x4, [x7], #REGSZ
	bl	asm_print_hex
	bl	asm_print_newline
	b	test_size_list
exit_size_print:
	mov	x30, sp
	ret

	/* -----------------------------------------------------
	 * This function calculates and prints required number
	 * of space characters followed by "= 0x", based on the
	 * length of ascii register name.
 	 * x0: length of ascii register name + 1
	 * ------------------------------------------------------
 	 */
FUNCTION_SECTION(print_alignment)
	/* The minimum ascii length is 3, e.g. for "x0" */
	adr	x4, print_spacer - 3
	add	x4, x4, x0
	b	asm_print_str

	/*
	 * Helper function to store x8 - x15 registers to
	 * the crash buf. The system registers values are
	 * copied to x8 to x15 by the caller which are then
	 * copied to the crash buf by this function.
	 * x0 points to the crash buf. It then calls
	 * size_controlled_print to print to console.
	 * Clobbers : x0 - x7, sp
	 */
FUNCTION_SECTION(str_in_crash_buf_print) 
	/* restore the crash buf address in x0 */
	mrs	x0, TPIDR_ELX
	stp	x8, x9, [x0]
	stp	x10, x11, [x0, #REGSZ * 2]
	stp	x12, x13, [x0, #REGSZ * 4]
	stp	x14, x15, [x0, #REGSZ * 6]
	b	size_controlled_print

	/* ------------------------------------------------------
	 * This macro calculates the offset to crash buf from
	 * cpu_data and stores it in TPIDR_ELX. It also saves x0
	 * and x1 in the crash buf by using sp as a temporary
	 * register.
	 * ------------------------------------------------------
	 */
	.macro prepare_crash_buf_save_x0_x1
	/* we can corrupt this reg to free up x0 */
	mov	sp, x0
	/* TPIDR_ELX contains the address to cpu_data structure */
	mrs	x0, TPIDR_ELX
	/* Calculate the Crash buffer offset in cpu_data */
	add	x0, x0, #PCPU_KDATA_CRASH_OFFSET
	/* Store crash buffer address in TPIDR_ELX */
	msr	TPIDR_ELX, x0
	str	x1, [x0, #REGSZ]
	mov	x1, sp
	str	x1, [x0]
	.endm

	/* -----------------------------------------------------
	 * This function allows to report a crash (if crash
	 * reporting is enabled) when an unhandled exception
	 * occurs. It prints the CPU state via the crash console
	 * making use of the crash buf. This function will
	 * not return.
	 * -----------------------------------------------------
	 */
FUNCTION_SECTION(report_unhandled_exception)
	prepare_crash_buf_save_x0_x1
	adr	x0, excpt_msg
	mov	sp, x0
	/* This call will not return */
	b	do_crash_reporting

	/* -----------------------------------------------------
	 * This function allows to report a crash (if crash
	 * reporting is enabled) when an unhandled interrupt
	 * occurs. It prints the CPU state via the crash console
	 * making use of the crash buf. This function will
	 * not return.
	 * -----------------------------------------------------
	 */
FUNCTION_SECTION(report_unhandled_interrupt)
	prepare_crash_buf_save_x0_x1
	adr	x0, intr_excpt_msg
	mov	sp, x0
	/* This call will not return */
	b	do_crash_reporting
	/* -----------------------------------------------------
	 * This function allows to report a crash from the lower
	 * exception level (if crash reporting is enabled) when
	 * panic() is invoked from C Runtime.
	 * It prints the CPU state via the crash console making
	 * use of 'gpregs_ctx' structure where general purpose
	 * registers are saved and the crash buf.
	 * This function will not return.
	 *
 	 * x0: Exception level
	 * -----------------------------------------------------
	 */
/* tcpu_sp == user thread stack frame base, and this is exception stack frame top of the user(we need this!!!) */	 
FUNCTION_SECTION(lower_el_panic) 
	/* get current thread addr */
	mrs 	x6, TPIDR_ELX
	ldr	x6, [x6, #PCPU_KDATA_USER_OFFSET]
	/* get current thread arch or tcpu_sp addr */
	ldr	x6, [x6, #PCPU_KDATA_USER_ARCH_OFFSET]
	/* get tcpu_sp */
	ldr 	x6, [x6]
	/* get exception stack addr -- tcpu_sp + sizeof(struct context_switch_frame) */
	add	x6, x6, #PCPU_KDATA_USER_CALLEE_FP_SIZE

	mov	x8, x0

	/* Print the crash message */
	adr	x4, excpt_msg_el
	bl	asm_print_str

	/* Print exception level */
	add	x0, x8, #'0'
	bl	plat_crash_console_putc
	bl	asm_print_newline

	/* Report x0 - x29 values stored in 'gpregs_ctx' structure */
	sub	x7, x6, #CTX_EXCEPTION_END
	/* Store the ascii list pointer in x6 */
	adr	x6, gp_regs

print_next:
	ldrb	w4, [x6]
	/* Test whether we are at end of list */
	cbz	w4, print_x30
	mov	x4, x6
	/* asm_print_str updates x4 to point to next entry in list */
	bl	asm_print_str
	/* x0 = number of symbols printed + 1 */
	sub	x0, x4, x6
	/* Update x6 with the updated list pointer */
	mov	x6, x4
	bl	print_alignment
	ldr	x4, [x7], #REGSZ
	bl	asm_print_hex
	bl	asm_print_newline
	b	print_next

print_x30:
	adr	x4, x30_msg
	bl	asm_print_str

	/* Print spaces to align "x30" string */
	mov	x0, #4
	bl	print_alignment

	/* Report x30 */
	ldr	x4, [x7]

	/* ----------------------------------------------------------------
	 * Different virtual address space size can be defined for each EL.
	 * Ensure that we use the proper one by reading the corresponding
	 * TCR_ELx register.
	 * ----------------------------------------------------------------
	 */
	cmp	x8, #MODE_EL2
	b.lt	from_el1	/* EL1 */
	mrs	x2, sctlr_el2
	mrs	x1, tcr_el2

	/* ----------------------------------------------------------------
	 * Check if pointer authentication is enabled at the specified EL.
	 * If it isn't, we can then skip stripping a PAC code.
	 * ----------------------------------------------------------------
	 */
test_pauth:
	tst	x2, #(SCTLR_EnIA_BIT | SCTLR_EnIB_BIT)
	b.eq	no_pauth

	/* Demangle address */
	and	x1, x1, #0x3F	/* T0SZ = TCR_ELx[5:0] */
	sub	x1, x1, #64
	neg	x1, x1		/* bottom_pac_bit = 64 - T0SZ */
	mov	x2, #-1
	lsl	x2, x2, x1
	bic	x4, x4, x2

no_pauth:
	bl	asm_print_hex
	bl	asm_print_newline

	/* TPIDR_ELX contains the address to cpu_data structure */
	mrs	x0, TPIDR_ELX
	/* Calculate the Crash buffer offset in cpu_data */
	add	x0, x0, #PCPU_KDATA_CRASH_OFFSET
	/* Store crash buffer address in TPIDR_ELX */
	msr	TPIDR_ELX, x0

	/* Print the rest of crash dump */
	b	print_sys_regs

from_el1:
	cmp	x8, #MODE_EL1
	b.lt	from_el0	/* EL0 */
	mrs	x2, sctlr_el1
	mrs	x1, tcr_el1
	b	test_pauth
from_el0:
	b 	no_pauth

	/* -----------------------------------------------------
	 * This function allows to report a crash (if crash
	 * reporting is enabled) when panic() is invoked from
	 * C Runtime. It prints the CPU state via the crash
	 * console making use of the crash buf. This function
	 * will not return.
	 * -----------------------------------------------------
	 */
FUNCTION_SECTION(curr_el_panic)
	msr	spsel, #MODE_SP_ELX
	prepare_crash_buf_save_x0_x1
	adr	x0, panic_msg
	mov	sp, x0
	/* Fall through to 'do_crash_reporting' */

	/* ------------------------------------------------------------
	 * The common crash reporting functionality. It requires x0
	 * and x1 has already been stored in crash buf, sp points to
	 * crash message and TPIDR_ELX contains the crash buf address.
	 * The function does the following:
	 *   - Retrieve the crash buffer from TPIDR_ELX
	 *   - Store x2 to x6 in the crash buffer
	 *   - Initialise the crash console.
	 *   - Print the crash message by using the address in sp.
	 *   - Print x30 value to the crash console.
	 *   - Print x0 - x7 from the crash buf to the crash console.
	 *   - Print x8 - x29 (in groups of 8 registers) using the
	 *     crash buf to the crash console.
	 *   - Print el3 sys regs (in groups of 8 registers) using the
	 *     crash buf to the crash console.
	 *   - Print non el3 sys regs (in groups of 8 registers) using
	 *     the crash buf to the crash console.
	 * ------------------------------------------------------------
	 */
do_crash_reporting:
	/* Retrieve the crash buf from TPIDR_ELX */
	mrs	x0, TPIDR_ELX
	/* Store x2 - x6, x30 in the crash buffer */
	stp	x2, x3, [x0, #REGSZ * 2]
	stp	x4, x5, [x0, #REGSZ * 4]
	stp	x6, x30, [x0, #REGSZ * 6]
	/* Initialize the crash console */
	bl	plat_crash_console_init
	/* Verify the console is initialized */
	cbz	x0, crash_panic
	/* Print the crash message. sp points to the crash message */
	mov	x4, sp
	bl	asm_print_str
	/* Print spaces to align "x30" string */
	mov	x0, #4
	bl	print_alignment
	/* Load the crash buf address */
	mrs	x0, TPIDR_ELX
	/* Report x30 first from the crash buf */
	ldr	x4, [x0, #REGSZ * 7]

#if ENABLE_PAUTH
	/* Demangle address */
	xpaci	x4
#endif
	bl	asm_print_hex
	bl	asm_print_newline
	/* Load the crash buf address */
	mrs	x0, TPIDR_ELX
	/* Now mov x7 into crash buf */
	str	x7, [x0, #REGSZ * 7]

	/* Report x0 - x29 values stored in crash buf */
	/* Store the ascii list pointer in x6 */
	adr	x6, gp_regs
	/* Print x0 to x7 from the crash buf */
	bl	size_controlled_print
	/* Store x8 - x15 in crash buf and print */
	bl	str_in_crash_buf_print
	/* Load the crash buf address */
	mrs	x0, TPIDR_ELX
	/* Store the rest of gp regs and print */
	stp	x16, x17, [x0]
	stp	x18, x19, [x0, #REGSZ * 2]
	stp	x20, x21, [x0, #REGSZ * 4]
	stp	x22, x23, [x0, #REGSZ * 6]
	bl	size_controlled_print
	/* Load the crash buf address */
	mrs	x0, TPIDR_ELX
	stp	x24, x25, [x0]
	stp	x26, x27, [x0, #REGSZ * 2]
	stp	x28, x29, [x0, #REGSZ * 4]
	bl	size_controlled_print

	/* Print the sys registers */
print_sys_regs:
#ifdef WITH_MONITOR_MODE
	adr	x6, el3_sys_regs
	mrs	x8, scr_el3
	mrs	x9, sctlr_el3
	mrs	x10, cptr_el3
	mrs	x11, tcr_el3
	mrs	x12, daif
	mrs	x13, mair_el3
	mrs	x14, spsr_el3
	mrs	x15, elr_el3
	bl	str_in_crash_buf_print
	mrs	x8, ttbr0_el3
	mrs	x9, esr_el3
	mrs	x10, far_el3
	bl	str_in_crash_buf_print
#elif WITH_SUPER_MODE
	/* Print the non el3 sys registers */
	adr	x6, el01_sys_regs
	mrs	x8, spsr_el1
	mrs	x9, elr_el1
	mrs	x10, spsr_abt
	mrs	x11, spsr_und
	mrs	x12, spsr_irq
	mrs	x13, spsr_fiq
	mrs	x14, sctlr_el1
	mrs	x15, actlr_el1
	bl	str_in_crash_buf_print
	mrs	x8, cpacr_el1
	mrs	x9, csselr_el1
	mrs	x10, sp_el1
	mrs	x11, esr_el1
	mrs	x12, ttbr0_el1
	mrs	x13, ttbr1_el1
	mrs	x14, mair_el1
	mrs	x15, amair_el1
	bl	str_in_crash_buf_print
	mrs	x8, tcr_el1
	mrs	x9, tpidr_el1
	mrs	x10, tpidr_el0
	mrs	x11, tpidrro_el0
	mrs	x12, par_el1
	mrs	x13, mpidr_el1
	mrs	x14, afsr0_el1
	mrs	x15, afsr1_el1
	bl	str_in_crash_buf_print
	mrs	x8, contextidr_el1
	mrs	x9, vbar_el1
	mrs	x10, cntp_ctl_el0
	mrs	x11, cntp_cval_el0
	mrs	x12, cntv_ctl_el0
	mrs	x13, cntv_cval_el0
	mrs	x14, cntkctl_el1
	mrs	x15, sp_el0
	bl	str_in_crash_buf_print
	mrs	x8, isr_el1
	bl	str_in_crash_buf_print
#else
#if CTX_INCLUDE_AARCH32_REGS
	/* Print the AArch32 registers */
	adr	x6, aarch32_regs
	mrs	x8, dacr32_el2
	mrs	x9, ifsr32_el2
	bl	str_in_crash_buf_print
#endif /* CTX_INCLUDE_AARCH32_REGS */
#endif
	/* Get the cpu specific registers to report */
	bl	do_cpu_reg_dump
	bl	str_in_crash_buf_print

	/* Print some platform registers */
	plat_crash_print_regs

	bl	plat_crash_console_flush

	/* Done reporting */
	no_ret	plat_panic_handler

#else	/* CRASH_REPORTING */
FUNCTION_SECTION(report_unhandled_exception)
report_unhandled_interrupt:
	no_ret	plat_panic_handler

#endif	/* CRASH_REPORTING */

FUNCTION_SECTION(crash_panic) 
	no_ret	plat_panic_handler
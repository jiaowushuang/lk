/***************************************************************************
 * Copyright 1996-2019 Synopsys, Inc.
 *
 * This Synopsys software and all associated documentation are proprietary
 * to Synopsys, Inc. and may only be used pursuant to the terms and
 * conditions of a written license agreement with Synopsys, Inc.
 * All other use, reproduction, modification, or distribution of the
 * Synopsys software or the associated documentation is strictly prohibited.
 ***************************************************************************/

.text
.code 32
.align	0

.globl _v8r_boot
_v8r_boot:

        //Put secondary cores into WFI
        MRC p15, 0, r0, c0, c0, 5 		// Read MPIDR
        ANDS r0, r0, #0xF
        BEQ primary_core_boot
        WFI								// Wait For Interrupt if not cpu0

primary_core_boot:
        // Change EL2 exception base address
        LDR r0, .cpu_vector_table
        LDR r0, [r0]
        /* Set VBAR address for this CPU. */
        MCR	p15, 0, r0, c12, c0, 0

        // Init HSCTLR
        LDR r0, =0x30C5180C				// See TRM for decoding
        MCR p15, 4, r0, c1, c0, 0 		// Write r0 to HSCTLR

        // Enable EL1 access to all IMP DEF registers
        LDR r0, =0x7F81
        MCR p15, 4, r0, c1, c0, 1 		// Write r0 to HACTLR

        // Change EL1 exception base address
        LDR r0, .cpu_vector_table
        LDR r0, [r0]
        /* Set VBAR address for this CPU. */
        MCR	p15, 0, r0, c12, c0, 0

        // Go to SVC mode
        MRS r0, cpsr
        MOV r1, #0x13
        BFI r0, r1, #0, #5

        MSR spsr_hyp, r0
        LDR r0, =el1_reset_handler
        MSR elr_hyp, r0
        DSB
        ISB
        ERET

el1_reset_handler:

        LDR r0, .LABTstack
        CPS #0x17
        MOV SP, r0

        LDR r0, .LIRQstack
        CPS #0x12
        MOV SP, r0

        LDR r0, .LFIQstack
        CPS #0x11
        MOV SP, r0

        LDR r0, .LSVCstack
        CPS #0x13
        MOV SP, r0

        B .LinitDataSaveRestore

.cpu_vector_table:
    .word __cpu_vector_table

/* constants relating to the stack initialisation values */
.LFIQstack:
	.word	__fiq_stack
.LABTstack:
	.word	__abt_stack
.LUNDEFstack:
	.word	__undef_stack
.LIRQstack:
	.word	__irq_stack
.LSVCstack:
	.word	__svc_stack
.LUSRstack:
	.word	__usr_stack

/* EOF */

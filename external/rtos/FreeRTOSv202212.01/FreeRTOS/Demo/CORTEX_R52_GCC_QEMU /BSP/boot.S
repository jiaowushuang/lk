/******************************************************************************
 * Synopsys DesignWare Virtual Platform Test Framework
 * Boot.S
 *
 * This file is substantially derived from the file crt0.S in the GCC newlib
 * stdC library source. It varies in that it requires all stack pointers to be
 * defined in the linker file, but otherwise implements the normal crt0 fns.
 *
 ******************************************************************************/

.text
.code 32
.align	0

.globl _boot
.global .LinitDataSaveRestore
_boot:

	/* A little bit of boot code that sets up the stacks, zeroes bss memory
	** and calls the static initialisers, before calling main(). If main
	** ever exits, then the static destructors are called and the system
	** is rebooted. */


	/*	Set up the stack pointer to a fixed value */
	/*	Changes by toralf:
		- Allow linker script to provide stack via __stack symbol - see
		  defintion of .Lstack
		- Provide "hooks" that may be used by the application to add
		  custom init code - see .Lhwinit and .Lswinit
		- Go through all execution modes and set up stack for each of them.
		  Loosely based on init.s from ARM/Motorola example code.
			  Note: Mode switch via CPSR is not allowed once in non-privileged
			mode, so we take care not to enter "User" to set up its sp,
			and also skip most operations if already in that mode. */

	/* Note: This 'mov' is essential when starting in User, and ensures we
		 always get *some* sp value for the initial mode, even if we
		 have somehow missed it below (in which case it gets the same
		 value as FIQ - not ideal, but better than nothing.) */
	ldr		r3, .LFIQstack
	mov		sp, r3

	mrs		r2, CPSR
	tst		r2, #0x0F		/* Test mode bits - in User of all are 0 */
	beq	   .LZeroMem		/* "eq" means r2 AND #0x0F is 0 */

	msr		CPSR_c, #0x11	/* FIRQ mode, interrupts disabled */
	mov		sp, r3

	ldr		sl, .LABTstack
	mov		r3, sl
	msr		CPSR_c, #0x17	/* Abort mode, interrupts disabled */
	mov		sp, r3

	ldr		r3, .LUNDEFstack
	msr		CPSR_c, #0x1B	/* Undefined mode, interrupts disabled */
	mov		sp, r3

	ldr		r3, .LIRQstack
	msr		CPSR_c, #0x12	/* IRQ mode, interrupts disabled */
	mov		sp, r3

	ldr		r3, .LSVCstack
	msr		CPSR_c, #0x13	/* Supervisory mode, interrupts disabled */
	mov		sp, r3

	ldr		r3, .LUSRstack
	str	   r3, [r3, #-4]	/* Move value into user mode sp without */
	ldmdb  r3, {sp}^	   /* changing modes, via '^' form of ldm */

	/* Disable FIQ and IRQ, and return to mode at entry */
	orr	   r2, r2, #0xC0
	msr	   CPSR_c, r2

.LinitDataSaveRestore:
    /* This section of code ensures that the initialised data segment
    ** from the ELF file is restored on subsequent platform resets. The
    ** first time through it will be correct but unsaved. The code saves
    ** it and then continues. Subsequent runs through cause the .init
    ** segment to be restored from its saved version. */
    ldr     r3, .LsavedSigAddr
    ldr     r2, [r3]
    ldr     r1, .LsavedSigConst
    cmp     r2, r1

    /* load memcpy arguments as if we're restoring the init data segment */
    ldr     a1, .LinitDataAddr
    ldr     a3, .LinitDataEnd
    sub     a3, a3, a1
    ldr     a2, .LsafeDataAddr

    /* swap src and dst address if this is the first time through and the operation is a save */
    moveq   r3, a1
    moveq   a1, a2
    moveq   a2, r3

    /* now either save or restore the init section */
    bl      memcpy

    /* clear the flag so that next time the data is restored */
    ldr     r3, .LsavedSigAddr
    ldr     r2, [r3]
    ldr     r1, .LsavedSigConst
    cmp     r2, r1
    moveq   r1, #0
    streq   r1, [r3]        /* clear signature first time around */

    /* and jump around our constants */
    b       .LZeroMem


.LZeroMem:
	/* Zero the memory in the .bss section.	 */
	ldr		a1, .LZmemStart		/* First arg: start of memory block */
	mov		a2, #0			/* Second arg: fill value */
	mov		fp, a2			/* Null frame pointer */
	mov		r7, a2			/* Null frame pointer for Thumb */
	ldr		a3, .LZmemEnd
	sub		a3, a3, a1		/* Third arg: length of block */
	bl		memset

	/* Some arm/elf targets use the .init and .fini sections
	   to create constructors and destructors, and for these
	   targets we need to call the _init function and arrange
	   for _fini to be called at program exit.	*/
	mov		r4, r0
	mov		r5, r1
	ldr		r0, .Lfini
	bl		atexit
    bl      __libc_init_array

	mov		r0, r4
	mov		r1, r5

	bl		main

#if 1
    /* This bit of code uses a semi-hosting command to pause the platform */
    ldr     r0, .LpauseOpcode
    swi     0x123456
#else
    /* OR use the more conventional exit - which will try to close stdin/out/err! */
	bl		exit
#endif
	b		_boot

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

/* constants relating to the init data save/restore code chunk */
.LsavedSigAddr:
    .word   __data_saved_sig
.LsavedSigConst:
    .word   0x12345678
.LinitDataAddr:
    .word   __data_start
.LinitDataEnd:
    .word   __data_stop
.LsafeDataAddr:
    .word   __data_safe_start

/* start and end of memory segment to be zeroed */
.LZmemStart:
	.word	__bss_start__
.LZmemEnd:
	.word	__bss_end__

.Lfini:
	.word	_fini

.LpauseOpcode:
    .word   0x00020026


/* EOF */

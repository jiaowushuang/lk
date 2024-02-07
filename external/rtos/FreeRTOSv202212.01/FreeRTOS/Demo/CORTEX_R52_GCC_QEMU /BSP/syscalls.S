/******************************************************************************
 * Synopsys DesignWare VPTEST PlatformTest code
 * SysCalls.S
 *
 * Copyright (c) 2006 by Synopsys Inc.
 * ALL RIGHTS RESERVED
 *
 * SYNOPSYS CONFIDENTIAL - This is an unpublished, proprietary work of
 * Synopsys, Inc., and is fully protected under copyright and trade secret
 * laws. You may not view, use, disclose, copy, or distribute this file or
 * any information contained herein except pursuant to a valid written
 * license from Synopsys.
 *
 ******************************************************************************/

.text
.code 32
.align	0

/* The platform test code is built with GCC and a configuration of newlib that
** does not include an implementation of the system calls - hence they need to
** be provided for the link. In this case most of the calls are implemented as
** stubs which will stop the platform. Typically the system calls are not used
** with the exception of the sbrk, used for heap allocation/extension. */

.globl abort
abort:
    /* This bit of code uses a semi-hosting command to pause the platform */
    ldr     r0, .LpauseOpcode
    swi     0x123456
    b       .Lspin

.globl _exit
_exit:
    /* This bit of code uses a semi-hosting command to pause the platform */
    ldr     r0, .LpauseOpcode
    swi     0x123456
    mov     pc, lr      /* return from this fn - allows the platform to be run again */

.Lspin:
    b       .Lspin

.LpauseOpcode:
    .word   0x00020026


.globl _sbrk
_sbrk:
    /* This function is called to get some more memory for the heap. Because the
    ** platform tests use C++ and dynamically created objects this means that
    ** new and malloc get used, so this syscall has to work properly. The fn
    ** starts allocating from the beginning of the heap (as defined in the linker
    ** directives file), and keeps going until it reaches then allocated heap end. */

    /* char *_sbrk( int increment ) - r0 used for argument and return, r1-r3 safe to use */

    /*	Read Affinity register (MPIDR) to determine CPU ID and Cluster ID */
	mrc p15, 0, r3, c0, c0, 5
	/* Mask off CPU ID field */
	and r1, r3, #0xF
	/* Mask off Cluster ID field */
	lsr r2, r3, #8
	and r2, r2, #0xF
    /* Look up CPU ID, place in r3 */
	ldr r3, .cpu_id_cluster_table
	ldr r3, [r3, +r2, lsl #2]
    ldr r3, [r3, +r1, lsl #2]

    ldr     r2, .sbrk_top_table
    add     r2, r2, r3, lsl #2
    ldr     r2, [r2]
    cmp     r2, #0
    ldreq   r2, .heap_addr_table    /* first time through start at the bottom of the allocated area */
	ldreq   r2, [r2, +r3, lsl #2]
    add     r1, r2, r0              /* r1 is new top marker */
    ldr     r0, .heap_stop_table
	ldr     r0, [r0, +r3, lsl #2]
    cmp     r1, r0                  /* did we overflow the allocation? */
    ldrls   r0, .sbrk_top_table     /* get the top marker pointer (again, not enough registers!) */
    addls   r0, r0, r3, lsl #2
    strls   r1, [r0]                /* only update the static top marker if not overflowed */
    movls   r0, r2
    movls   pc, lr                  /* slightly quicker return when successful */
    mvn     r0, #0                  /* return -1 if increase failed */
    mov     pc, lr                  /* return */

.cpu_id_cluster_table:
    .word   __cpu_id_cluster_table
.sbrk_top_table:
    .word   __sbrk_top_table
.heap_addr_table:
    .word   __heap_addr_table
.heap_stop_table:
    .word   __heap_stop_table


.bss
__sbrk_top_table:
    .word   0x00000000
    .word   0x00000000
    .word   0x00000000
    .word   0x00000000
    .word   0x00000000
    .word   0x00000000
    .word   0x00000000
    .word   0x00000000

/* EOF */

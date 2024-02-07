/*******************************************************************************
 * Synopsys DesignWare
 *
 * Copyright (c) 10 Nov 2010 by Synopsys Inc.
 * ALL RIGHTS RESERVED
 *
 * SYNOPSYS CONFIDENTIAL - This is an unpublished, proprietary work of
 * Synopsys, Inc., and is fully protected under copyright and trade secret
 * laws. You may not view, use, disclose, copy, or distribute this file or
 * any information contained herein except pursuant to a valid written
 * license from Synopsys.
 *******************************************************************************/

.text
.code 32
.align	0

.globl _boot_smp

/*	In a Symmetric Multi-Processor system all Cores execute the same boot code.
	The different cores are identified by the CPU ID field (bits 1:0) of MPIDR.
	The clusters are identified by CLUSTER ID field (bits 11:8) of MPIDR.
	Now that multiple clusters are possible, the clusters can be asymmetric.
*/

/*  NOTE ON SINGLE-CLUSTER AND MULTI-CLUSTER PLATFORM TESTS

    It is preferred that a cluster executes from one ELF file.
    This ELF file should boot to the Platform Tests menu from
    CPU0, while all other CPUs wait for CPU0 to get to main().
    At this point the heap is configured, so the other CPUs can
    boot directly to a test function. NOTE that they should not
    boot via main() as this path initializes global variables
    etc. - things that we only want to do once. The other CPU
    test functions are recommended to provide various tests
    to be executed at the request of CPU0. These tests should
    be simple presence tests to prove core presence in the platform.

    EFFECT: all CPUs share the same heap and global variables.
            They will have their own stacks however. malloc()
            should already be implemented with locking to be
            multi-core compatible.

    Regarding MULTI-CLUSTER:
    If the clusters all boot from the same location and are
    directly compatible (as in ARM's big.LITTLE where code
    for A15 can run on A7), then you may decide to boot ALL
    cores from all clusters in question from one ELF. This
    is possible, however, beware that all CPUs share the
    same heap. Also note that only one CPU should be selected
    to drive the Platform Test menu, all others should run
    basic test loops to prove presence.
    WARNING: if more that one cluster is handled by an ELF
    file, then beware of resets: a reset to the cluster driving
    the Platform Test menu will cause that CPU to boot to main()
    again and thus reset the heap (which the clusters that have
    not been reset may still be using). With only one ELF,
    all clusters must be reset at the same time!
    The alternative is to compile a Platform Test ELF for
    each cluster, then if one cluster is reset, the other
    cluster's global variables and heap are untouched and
    hence they can continue to run as normal.

    Motivation for single ELF per cluster:
    All CPUs in the cluster execute from the same location
    and are brought out of reset at the same time. Running
    a separate Platform Test ELF for each core in the cluster
    is not necessary, especially since we are likely to want
    to drive the Platform Test menu from one core, whilst
    the other cores just need to prove basic presence.

*/

_boot_smp:

   	/* Check the type of CPU */
	mrc p15, 0, r0, c0, c0, 0
	lsr r1, r0, #4
	ldr r2, =0xFFF
	and r1, r1, r2
	/* ARMv8R cores need special boot code */
	ldr r2, =0xD13 /* Cortex-R52 */
	cmp r1, r2
	beq _v8r_boot
	ldr r2, =0xD16 /* Cortex-R52+ */
	cmp r1, r2
	beq _v8r_boot

	/*	Read Affinity register (MPIDR) to determine CPU ID and Cluster ID */
	mrc p15, 0, r0, c0, c0, 5

	/* Mask off CPU ID field */
	and r1, r0, #0xF
	/* Mask off Cluster ID field */
	lsr r2, r0, #8
	and r2, r2, #0xF

    /* Check CPU CLUSTER is in bounds. */
    ldr r3, .max_clusters
    cmp r2, r3
    bge UnknownCluster

    /* Check CPU ID is within bounds for CPU CLUSTER */
    ldr r3, .max_cpus_table
    ldr r3, [r3, +r2, lsl #2]
    cmp r1, r3
    bge UnknownCore

    /* Look up CPU ID, place in r9 */
	ldr r9, .cpu_id_cluster_table
	ldr r9, [r9, +r2, lsl #2]
    ldr r9, [r9, +r1, lsl #2]


    //-----------------------------------------------------------------------
	// Set the monitor mode vector table base address (MVBAR)
	//-----------------------------------------------------------------------

	// Vector table will be at start of monitor code.
	// NOTE: all cores use the same vector table + code, but different stacks.
#define CPUID_ARM_VIRT_SHIFT		12
#define CPUID_ARM_VIRT_MASK		(0xF << CPUID_ARM_VIRT_SHIFT)
#define CPUID_ARM_SEC_EXT_SHIFT		4
#define CPUID_ARM_SEC_EXT_MASK		(0xF << CPUID_ARM_SEC_EXT_SHIFT)
	mrc	p15, 0, r0, c0, c1, 1			@CPUID_EXT_PFR1
	and	r0, r0, #CPUID_ARM_SEC_EXT_MASK	@ Mask virtualization bits
	cmp	r0, #(1 << CPUID_ARM_SEC_EXT_SHIFT)
	ldr	r0, .mon_mode_vectors
	mcreq	p15, 0, r0, c12, c0, 1

	//-----------------------------------------------------------------------
	// Set the vector table base address (VBAR)
	//-----------------------------------------------------------------------

    /* Look up VBAR address for this CPU. */
	ldr r0, .cpu_vector_table
	ldr r0, [r0, +r9, lsl #2]
	/* Set VBAR address for this CPU. */
	mcreq	p15, 0, r0, c12, c0, 0

	//-----------------------------------------------------------------------
	// Set the monitor mode stack with SMI call.
	//-----------------------------------------------------------------------

	// It would be good if we could set up the monitor mode stack in Boot.S.
	// However, in monitor mode, the SSM requires that instructions are
	// fetched from the monitor code/monitor ramcode ranges.
	// Hence, we'll need to set up the stack by SMI with a special OpCode.
#define ID_ISAR4_SMC_SHIFT		12
#define ID_ISAR4_SMC_MASK		(0xF << ID_ISAR4_SMC_SHIFT)
	mrc	p15, 0, r1, c0, c1, 1			@ID_ISAR4
	and	r1, r1, #ID_ISAR4_SMC_MASK	@ Mask SMC bits
	cmp	r1, #(1 << ID_ISAR4_SMC_SHIFT)
	ldr r0, .cpu_mon_stack_table
	ldr r0, [r0, +r9, lsl #2]
	ldr	r1, .mon_shared_mem
	str	r0, [r1]
	smceq	0xF

    /* Look up CPU boot address. */
	ldr r3, .cpu_boot_addr_table
	ldr r3, [r3, +r9, lsl #2]

    mov pc, r3

UnknownCluster:
    b UnknownCluster

UnknownCore:
    b UnknownCore

.max_clusters:
    .word __max_clusters
.cpu_id_cluster_table:
    .word __cpu_id_cluster_table
.max_cpus_table:
    .word __max_cpus_table
.cpu_boot_addr_table:
    .word __cpu_boot_addr_table

.mon_shared_mem:
	.word	__mon_shared_mem
.mon_mode_vectors:
    .word __mon_mode_vectors
.cpu_mon_stack_table:
    .word __cpu_mon_stack_table
.cpu_vector_table:
    .word __cpu_vector_table

/* EOF */

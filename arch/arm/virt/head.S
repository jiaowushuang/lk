/*
 * Copyright (c) 2008-2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

/*
 * xen/arch/arm/head.S
 *
 * Start-of-day code for an ARMv7-A with virt extensions.
 *
 * Tim Deegan <tim@xen.org>
 * Copyright (c) 2011 Citrix Systems.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <kern/asm.h>
#include <arch/arm/cores.h>
#include <arch/arm/mmu.h>
#include <arch/asm_macros.h>
#include <arch/arch_macro.h>

#if WITH_KERNEL_VM
#include <kernel/vm.h>
#endif

#define __START_FIRST_SLOT      FIRST_TABLE_OFFSET(KERNEL_BASE+KERNEL_LOAD_OFFSET)
#define __START_SECOND_SLOT     SECOND_TABLE_OFFSET(KERNEL_BASE+KERNEL_LOAD_OFFSET)

/*
 * Move an immediate constant into a 32-bit register using movw/movt
 * instructions.
 */
.macro mov_w reg, word
        movw  \reg, #:lower16:\word
        movt  \reg, #:upper16:\word
.endm

/*
 * There are no easy way to have a PC relative address within the range
 * +/- 4GB of the PC.
 *
 * This macro workaround it by asking the user to tell whether the MMU
 * has been turned on or not.
 *
 * When the MMU is turned off, we need to apply the physical offset
 * (r10) in order to find the associated physical address.
 */
.macro adr_l, dst, sym, mmu
        ldr   \dst, =\sym
        .if \mmu == 0
        add   \dst, \dst, r10
        .endif
.endm

/*
 * Common register usage in this file:
 *   r0  - arg0
 *   r1  - arg1
 *   r2  - arg2
 *   r3  - arg3
 *   r4  -
 *   r5  - 
 *   r6  - cpu num
 *   r7  - pgd phys
 *   r8  - pgd phys 
 *   r9  - paddr(start)
 *   r10 - phys offset
 *   r11 - 
 *   r12 - tmp
 *   r13 - SP
 *   r14 - LR
 *   r15 - PC
 */

        .arm
        .arch_extension virt

.section ".text.boot"
.globl _start
_start:
    b   platform_reset
    b   arm_undefined       /* taken from hyp mode : kernel  */
    b   arm_syscall_hyp     /* taken from hyp mode : kernel  */
    b   arm_prefetch_abort  /* taken from hyp mode : kernel  */
    b   arm_data_abort      /* taken from hyp mode : kernel  */
    b   arm_trap            /* taken from non-hyp mode: user - arm_undefined, arm_syscall(svc/hvc), arm_prefetch_abort, arm_data_abort */
    b   arm_irq
    b   arm_fiq
#if WITH_SMP
    b   arm_reset
#endif

.weak platform_reset
platform_reset:
        /* Fall through for the weak symbol */
.globl arm_reset
arm_reset:
        /*
         * This must be the very first address in the loaded image.
         * It should be linked at KERNEL_BASE+KERNEL_LOAD_OFFSET, and loaded at any
         * 4K-aligned address.  All of text+data+bss must fit in 2MB,
         * or the initial pagetable code below will need adjustment.
         */

        /* Disable all interrupts */
        cpsid   aif                    

        /* Find out where we are */
        ldr     r12, =arm_reset
        adr     r9, arm_reset                   /* r9  := paddr (arm_reset) */
        sub     r10, r9, r12                     /* r10 := phys-offset */

        /* Check cpu mode */
        mrc     p15, 0, r12, c0, c1, 1
        and     r12, r12, #0xf000               /* Bits 12-15 define virt extensions */
        teq     r12, #0x1000                    /* Must == 0x1 or may be incompatible */
        bne     .

        mrs     r12, cpsr
        and     r12, r12, #0x1f
        cmp     r12, #0x1a                      /* CPSR_MODE_HYP */
        bne     .

#if WITH_SMP
        /* figure out our cpu number */
        mrc     p15, 0, r12, c0, c0, 5          /* read MPIDR */

        /* mask off the bottom bits to test cluster number:cpu number */
        ubfx    r12, r12, #0, #SMP_CPU_ID_BITS

        /* if we're not cpu 0:0, fall into a trap and wait */
        teq     r12, #0
        movne   r0, r12
        bne     arm_secondary_setup
#endif // WITH_SMP

#if WITH_CPU_EARLY_INIT
        /* call platform/arch/etc specific init code */
        bl      __cpu_early_init
#endif // WITH_CPU_EARLY_INIT

#if WITH_NO_PHYS_RELOCATION
        /* assume that image is properly loaded in physical memory */
#else
        /* see if we need to relocate to our proper location in physical memory */
        adr     r4, _start                           /* this emits sub r4, pc, #constant */
        ldr     r5, =(MEMBASE + KERNEL_LOAD_OFFSET)  /* calculate the binary's physical load address */
        subs    r12, r4, r5                          /* calculate the delta between where we're loaded and the proper spot */
        beq     .Lrelocate_done

        /* we need to relocate ourselves to the proper spot */
        ldr     r6, =__data_end
        ldr     r7, =(KERNEL_BASE - MEMBASE)
        sub     r6, r7
        add     r6, r12

.Lrelocate_loop:
        ldr     r7, [r4], #4
        str     r7, [r5], #4
        cmp     r4, r6
        bne     .Lrelocate_loop

        /* we're relocated, jump to the right address */
        sub     pc, r12
        nop     /* skipped in the add to pc */

        /* recalculate the physical offset */
        sub     r10, r10, r12

.Lrelocate_done:
#endif // !WITH_NO_PHYS_RELOCATION

        bl      cpu_init
#if WITH_KERNEL_VM        
        bl      create_page_tables
        bl      enable_mmu
#endif
#if ARCH_HAS_MPU
        bl      enable_mpu
#endif
#if WITH_KERNEL_VM
        /* We are still in the 1:1 mapping. Jump to the runtime Virtual Address. */
        ldr     r0, =.Lprimary_switched
        mov     pc, r0
.Lprimary_switched:
#endif
#if MMU_WITH_TRAMPOLINE
        /* Switch to main page table - HTTBR_64 */
        mov      r1, #0
        mcrr     p15, 4, r8, r1, c2 // BADDR = r8(32 bits) : r1
        dsb
        isb
        /* Invalidate TLB. The value in r0 is ignored */
        mcr     p15, 4, r0, c8, c7, 0 // TLBIALLH
        dsb     sy
        isb

#endif // MMU_WITH_TRAMPOLINE        

        //bl      setup_fixmap

        /* at this point we're running at our final location in virtual memory (if enabled) */
.Lstack_setup:
#ifndef ARM_ISA_ARMV8
        /* set up the stack for irq, fiq, abort, undefined, system/user, and lastly supervisor/hypervisor mode */
        mov     r12, #0

        cpsid   i,#0x12       /* irq */
        mov     sp, r12

        cpsid   i,#0x11       /* fiq */
        mov     sp, r12

        cpsid   i,#0x17       /* abort */
        mov     sp, r12

        cpsid   i,#0x1b       /* undefined */
        mov     sp, r12

        cpsid   i,#0x1f       /* system */
        mov     sp, r12

        cpsid   i,#0x13       /* supervisor */
        mov     sp, r12
#endif
        cpsid   i,#0x1a       /* hypervisor */
        ldr     r12, =abort_stack
        add     r12, #ARCH_DEFAULT_STACK_SIZE
        mov     sp, r12    

        /* stay in hypervisor mode from now on out */

        /* copy the initialized data segment out of rom if necessary */
        ldr     r4, =__data_start_rom
        ldr     r5, =__data_start
        ldr     r6, =__data_end

        cmp     r4, r5
        beq     .L__do_bss

.L__copy_loop:
        cmp     r5, r6
        ldrlt   r7, [r4], #4
        strlt   r7, [r5], #4
        blt     .L__copy_loop

.L__do_bss:
        /* clear out the bss */
        ldr     r4, =__post_prebss_bss_start
        ldr     r5, =_end
        mov     r6, #0
.L__bss_loop:
        cmp     r4, r5
        strlt   r6, [r4], #4
        blt     .L__bss_loop

        bl      bootstrap_main
        b       .

#if WITH_SMP
        /* secondary cpu entry point */
        /* r0 holds cpu number */
        /* r10 hold phys offset */
FUNCTION(arm_secondary_setup)
        /* all other cpus, trap and wait to be released */
4:
        wfe
        ldr     r12, =arm_boot_cpu_lock
        add     r12, r12, r10
        ldr     r12, [r12]
        cmp     r12, #0
        bne     4b

        and     r1, r0, #0xff
        cmp     r1, #(1 << SMP_CPU_CLUSTER_SHIFT)
        bge     unsupported_cpu_trap
        bic     r0, r0, #0xff
        orr     r0, r1, r0, LSR #(8 - SMP_CPU_CLUSTER_SHIFT)

        cmp     r0, #SMP_MAX_CPUS
        bge     unsupported_cpu_trap
        mov     r6, r0 /* save cpu num */

        /* set up the stack for irq, fiq, abort, undefined, system/user, and lastly supervisor mode */
        mov     r1, #0
        cpsid   i,#0x12       /* irq */
        mov     sp, r1

        cpsid   i,#0x11       /* fiq */
        mov     sp, r1

        cpsid   i,#0x17       /* abort */
        mov     sp, r1

        cpsid   i,#0x1b       /* undefined */
        mov     sp, r1

        cpsid   i,#0x1f       /* system */
        mov     sp, r1

        cpsid   i,#0x13       /* supervisor */
        mov     sp, r1
        
        cpsid   i,#0x1a       /* hypervisor */
        ldr     r1, =abort_stack
        mov     r2, #ARCH_DEFAULT_STACK_SIZE
        add     r0, #1
        mul     r2, r2, r0
        add     r1, r2

        mov     sp, r1

        bl      cpu_init

#if WITH_KERNEL_VM   
        /* load the physical base of the translation table and clear the table */
        ldr     r7, =arm_kernel_translation_table
        add     r7, r7, r10
#if MMU_WITH_TRAMPOLINE
        /* move arm_kernel_translation_table address to r8 and
        * set cacheable attributes on translation walk
        */
        mov     r8, r7

        /* Prepare tt_trampoline page table */
        /* Calculate pagetable physical addresses */
        ldr     r7, =tt_trampoline  /* r7 = tt_trampoline vaddr */
        add     r7, r7, r10     /* r7 = tt_trampoline paddr */
#endif // MMU_WITH_TRAMPOLINE      
        bl      enable_mmu

        /* We are still in the 1:1 mapping. Jump to the runtime Virtual Address. */
        ldr     r0, =.Lsecondary_switched
        mov     pc, r0

.Lsecondary_switched:
#if MMU_WITH_TRAMPOLINE
        /* Switch to main page table - HTTBR_64 */
        mov      r1, #0
        mcrr     p15, 4, r8, r1, c2 // BADDR = r8(32 bits) : r1
        dsb
        isb
        /* Invalidate TLB. The value in r0 is ignored */
        mcr     p15, 4, r0, c8, c7, 0 // TLBIALLH
        dsb     sy
        isb

#endif // MMU_WITH_TRAMPOLINE    
 
        mcr     p15, 4, r0, c8, c7, 0  /* Flush hypervisor TLB */
        mcr     p15, 0, r0, c7, c5, 0  /* Flush I-cache */
        mcr     p15, 0, r0, c7, c5, 6       /* Flush branch predictor */
        dsb                            /* Ensure completion of TLB+BP flush */
        isb
#endif

#if ARCH_HAS_MPU
        bl      enable_mpu
#endif
        /* stay in hypervisor and call into arm arch code to continue setup */
        mov     r0, r6
        bl      arm_secondary_entry

        /* cpus above the number we claim to support get trapped here */
unsupported_cpu_trap:
        wfe
        b       unsupported_cpu_trap

END_FUNCTION(arm_secondary_setup)
#endif // WITH_SMP

/* Clobbers - r5, r0, r12 */
LOCAL_FUNCTION(cpu_init)
        mov     r5, lr                    /* r5 := return address */
#if WITH_KERNEL_VM        
        /* Initialize Memory Attribute Indirection Register */
        ldr     r0, =MMU_MAIR0_VAL
        mcr     p15, 4, r0, c10, c2, 0  // HMAIR0  
        ldr     r1, =MMU_MAIR1_VAL
        mcr     p15, 4, r1, c10, c2, 1  // HMAIR1

        /*
         * Set up the HTCR:
         * PT walks use Inner-Shareable accesses,
         * PT walks are write-back, write-allocate in both cache levels,
         * Full 32-bit address space goes through this table.
         */

        ldr     r0, =MMU_TCR_FLAGS_BASE
        mcr     p15, 4, r0, c2, c0, 2  

        ldr     r12, =arm_mmu_tcr_flags
        add     r12, r12, r10
        str     r0, [r12]
#endif // WITH_KERNEL_VM
        ldr     r0, =HSCTLR_SET
#if WITH_KERNEL_VM
        /* enable caches so atomics and spinlocks work */
        orr     r0, r0, #(1<<12)
        orr     r0, r0, #(1<<2)
#endif // WITH_KERNEL_VM
        mcr     p15, 0, r0, c1, c0, 0
        isb

        mov   pc, r5                    /* Return address is in r5 */
END_FUNCTION(cpu_init)

/*
 * Macro to create a page table entry in \ptbl to \tbl
 *
 * ptbl:    table symbol where the entry will be created
 * tbl:     table symbol to point to
 * virt:    virtual address
 * shift:   #imm page table shift
 * mmu:     Is the MMU turned on/off. If not specified it will be off
 *
 * Preserves \virt
 * Clobbers r1 - r4
 *
 * Also use r10 for the phys offset.
 *
 * Note that \virt should be in a register other than r1 - r4
 */
.macro create_table_entry, ptbl, tbl, virt, shift, offset, mmu=0
        lsr   r1, \virt, #\shift
        mov_w r2, LPAE_ENTRY_MASK
        and   r1, r1, r2             /* r1 := slot in \tlb */
        lsl   r1, r1, #3             /* r1 := slot offset in \tlb */

        ldr   r4, =\tbl
        add   r4, r4, \offset
        add   r4, r4, r10            /* r4 := paddr(\tlb) */

        movw  r2, #MMU_INITIAL_MAP_TABLE   /* r2:r3 := right for linear PT */
        orr   r2, r2, r4             /*           + \tlb paddr */
        mov   r3, #(1<<31)

        adr_l r4, \ptbl, \mmu

        strd  r2, r3, [r4, r1]
.endm

/*
 * Macro to create a mapping entry in \tbl to \paddr. Only mapping in 3rd
 * level table (i.e page granularity) is supported.
 *
 * ptbl:     table symbol where the entry will be created
 * virt:    virtual address
 * phys:    physical address
 * type:    mapping type. If not specified it will be normal memory (L3/L2)
 * mmu:     Is the MMU turned on/off. If not specified it will be off
 *
 * Preserves \virt, \phys
 * Clobbers r1 - r4
 *
 * * Also use r10 for the phys offset.
 *
 * Note that \virt and \paddr should be in other registers than r1 - r4
 * and be distinct.
 */
.macro create_mapping_entry, ptbl, virt, phys, shift, type, mmu=0
        mov_w r2, LPAE_ENTRY_MASK
        lsr   r1, \virt, #\shift
        and   r1, r1, r2             /* r1 := slot in \tlb */
        lsl   r1, r1, #3             /* r1 := slot offset in \tlb */

        lsr   r4, \phys, #\shift
        lsl   r4, r4, #\shift   /* r4 := PAGE_ALIGNED(phys) */

        movw  r2, #\type             /* r2:r3 := right for section PT */
        orr   r2, r2, r4             /*          + PAGE_ALIGNED(phys) */
        mov   r3, #0

        adr_l r4, \ptbl, \mmu

        strd  r2, r3, [r4, r1]
.endm

/*
 * Rebuild the boot pagetable's first-level entries. The structure
 * is described in mm.c.
 *
 * After the CPU enables paging it will add the fixmap mapping
 * to these page tables, however this may clash with the 1:1
 * mapping. So each CPU must rebuild the page tables here with
 * the 1:1 in place.
 *
 * Inputs:
 *   r9 : paddr(start)
 *   r10: phys offset
 *
 * Clobbers r0 - r4
 *
 * Register usage within this function:
 *   r6 : Identity map in place
 */
#if WITH_KERNEL_VM  
LOCAL_FUNCTION(create_page_tables)
        /* Prepare the page-tables for mapping  */
        // TODO: zero of page table
        /* load the address of the mmu_initial_mappings table and start processing */
        ldr     r5, =mmu_initial_mappings
        add     r5, r5, r10
        /* r5 = physical address of mmu initial mapping table */ 
        mov     r0, #0
        /* page table offset */
.Linitial_mapping_loop:            
        ldmia   r5!, { r6-r8, r11-r12 }
        /* r6 = phys, r7 = virt, r8 = size, r11 = flags */
        /* if size == 0, end of list */
        cmp     r8, #0
        beq     .Linitial_mapping_done

        /* set up the flags */
        ldr     r12, =MMU_INITIAL_MAP_NORMAL_BLOCK
        teq     r11, #MMU_INITIAL_MAPPING_FLAG_UNCACHED
        ldreq   r12, =MMU_INITIAL_MAP_STRONGLY_ORDERED_BLOCK
        beq     0f
        teq     r11, #MMU_INITIAL_MAPPING_FLAG_DEVICE
        ldreq   r12, =MMU_INITIAL_MAP_DEVICE_BLOCK
        /* r12 = mmu entry flags */

0:
        create_table_entry arm_kernel_translation_table, arm_kernel_translation_table_l2, r7, FIRST_SHIFT, r0

        /* Setup arm_kernel_translation_table_l2: */
        adr_l r4, arm_kernel_translation_table_l2, mmu=0
        add   r4, r4, r0
        mov   r2, r6

        lsr   r2, r2, #SECOND_SHIFT  /* Base address for 2MB mapping */
        lsl   r2, r2, #SECOND_SHIFT
        orr   r2, r2, r12

        mov   r3, #0x0

        /* ... map of vaddr(start) in arm_kernel_translation_table_l2 */
        mov   r1, #0
1:      strd  r2, r3, [r4, r1]       /* Map vaddr(start) */
        add   r2, r2, #SECOND_SIZE     /* Next block */
        add   r1, r1, #8             /* Next slot */
        cmp   r1, #(LPAE_ENTRIES<<3) /* 512*8-byte entries per page */
        blo   1b 
        mov   r0, #PAGE_SIZE

        b       .Linitial_mapping_loop

.Linitial_mapping_done:

        ldr   r7, =arm_kernel_translation_table
        add   r7, r7, r10

#if MMU_WITH_TRAMPOLINE
        /*
         * Setup the 1:1 mapping so we can turn the MMU on. Note that
         * only the first page of  will be part of the 1:1 mapping.
         */
        mov   r8, r7
        mov   r11, #0

        ldr   r0, =KERNEL_BASE //+KERNEL_LOAD_OFFSET
        create_table_entry tt_trampoline, tt_trampoline_l2, r0, FIRST_SHIFT, r11

        /* Setup tt_trampoline_l2: */
        adr_l r4, tt_trampoline_l2, mmu=0

        lsr   r2, r9, #SECOND_SHIFT  /* Base address for 2MB mapping */
        lsl   r2, r2, #SECOND_SHIFT
        orr   r2, r2, #(MMU_INITIAL_MAP_NORMAL_BLOCK & 0xf00) /* r2:r3 := map */
        orr   r2, r2, #(MMU_INITIAL_MAP_NORMAL_BLOCK & 0x0ff)
        mov   r3, #0x0

        /* ... map of vaddr(start) in tt_trampoline_l2 */
        mov   r1, #0
1:      strd  r2, r3, [r4, r1]       /* Map vaddr(start) */
        add   r2, r2, #SECOND_SIZE     /* Next block */
        add   r1, r1, #8             /* Next slot */
        cmp   r1, #(LPAE_ENTRIES<<3) /* 512*8-byte entries per page */
        blo   1b

        /*
         * Find the first slot used. If the slot is not __START_FIRST_SLOT,
         * then the 1:1 mapping will use its own set of page-tables from
         * the second level.
         */
        lsr   r1, r9, #FIRST_SHIFT
        mov_w r0, LPAE_ENTRY_MASK
        and   r1, r1, r0              /* r1 := first slot */
        cmp   r1, #__START_FIRST_SLOT
        beq   1f
        create_table_entry tt_trampoline, tt_trampoline_l2, r9, FIRST_SHIFT, r11
        b     link_from_second_id

1:
        /*
         * Find the second slot used. If the slot is __START_SECOND_SLOT, then the
         * 1:1 mapping will use its own set of page-tables from the
         * third level. For slot __START_SECOND_SLOT,  is not yet able to handle
         * it.
         */
        lsr   r1, r9, #SECOND_SHIFT
        mov_w r0, LPAE_ENTRY_MASK
        and   r1, r1, r0             /* r1 := second slot */
        cmp   r1, #__START_SECOND_SLOT
        beq   fail
        create_table_entry tt_trampoline_l2, tt_trampoline_l3, r9, SECOND_SHIFT, r11
        b     link_from_third_id

link_from_second_id:
        create_table_entry tt_trampoline_l2, tt_trampoline_l3, r9, SECOND_SHIFT, r11
link_from_third_id:
        create_mapping_entry tt_trampoline_l3, r9, r9, THIRD_SHIFT, MMU_INITIAL_MAP_NORMAL_PAGE

        ldr  r7, =tt_trampoline
        add  r7, r7, r10
#endif

        mov   pc, lr    
END_FUNCTION(create_page_tables)

/*
 * Turn on the Data Cache and the MMU. The function will return on the 1:1
 * mapping. In other word, the caller is responsible to switch to the runtime
 * mapping.
 *
 * Clobbers r0, r1, r7
 */
LOCAL_FUNCTION(enable_mmu)
        /* per cpu mmu setup, shared between primary and secondary cpus
           args:
           r7 == translation table physical
           r8 == final translation table physical (if using trampoline)
        */

        /*
         * The state of the TLBs is unknown before turning on the MMU.
         * Flush them to avoid stale one.
         */
        mcr     p15, 4, r0, c8, c7, 0    /* Flush hypervisor TLBs */
        dsb     nsh

        /* Write 's PT's paddr into the HTTBR */
        mov     r1, #0                 /* r7:r1 is paddr (boot_pagetable) */
        mcrr    p15, 4, r7, r1, c2 
        isb

        /* Read HSCTLR into r0 */
        mrc     p15, 4, r0, c1, c0, 0
        /* Enable MMU and D-cache */
        orr     r0, r0, #(1<<0 |1<<2)

        dsb                          /* Flush PTE writes and finish reads */

        /* Write back HSCTLR */
        mcr     p15, 4, r0, c1, c0, 0
        isb                          /* Now, flush the icache */
        mov     pc, lr
END_FUNCTION(enable_mmu)
#endif

#if ARCH_HAS_MPU
LOCAL_FUNCTION(enable_mpu)
        /* Read HSCTLR into r0 */
        mrc     p15, 4, r0, c1, c0, 0
        /* Enable MPU and D-cache */
        orr     r0, r0, #(1<<0 |1<<2)

	/* Enable MPU background region - BR=0(Background Region disable) */
	orr	r0, r0, #(1<<17)      

        dsb                          /* Flush PTE writes and finish reads */

        /* Write back HSCTLR */
        mcr     p15, 4, r0, c1, c0, 0
        isb                          /* Now, flush the icache */
        mov     pc, lr
END_FUNCTION(enable_mpu)
#endif
/*
 * Map the UART in the fixmap (when earlyprintk is used) and hook the
 * fixmap table in the page tables.
 *
 * The fixmap cannot be mapped in create_page_tables because it may
 * clash with the 1:1 mapping.
 *
 * Inputs:
 *   r10: Physical offset
 *
 * Clobbers r0 - r4
 */
LOCAL_FUNCTION(setup_fixmap)
        mov     pc, lr
END_FUNCTION(setup_fixmap)

/* Fail-stop */
fail:   
1:      wfe
        b       1b

.ltorg
#if WITH_KERNEL_VM && MMU_WITH_TRAMPOLINE
.section ".bss.prebss.translation_table"
.align 14
DATA(tt_trampoline)
    .skip 4096
DATA(tt_trampoline_l2)
    .skip 4096
DATA(tt_trampoline_l3)
    .skip 4096        
#endif // WITH_KERNEL_VM && MMU_WITH_TRAMPOLINE
.data
.align 2

#if WITH_KERNEL_VM
/*
 * Switch TTBR
 * r1:r0       ttbr
 *
 * TODO: This code does not comply with break-before-make.
 */
FUNCTION(switch_ttbr)
        dsb                            /* Ensure the flushes happen before
                                        * continuing */
        isb                            /* Ensure synchronization with previous
                                        * changes to text */
        mcr     p15, 4, r0, c8, c7, 0  /* Flush hypervisor TLB */
        mcr     p15, 0, r0, c7, c5, 0  /* Flush I-cache */
        mcr     p15,0,r0,c7,c5,6       /* Flush branch predictor */
        dsb                            /* Ensure completion of TLB+BP flush */
        isb

        mcrr    p15, 4, r0, r1, c2 

        dsb                            /* ensure memory accesses do not cross
                                        * over the TTBR0 write */
        isb                            /* Ensure synchronization with previous
                                        * changes to text */
        mcr     p15, 4, r0, c8, c7, 0  /* Flush hypervisor TLB */
        mcr     p15, 0, r0, c7, c5, 0  /* Flush I-cache */
        mcr     p15,0,r0,c7,c5,6       /* Flush branch predictor */
        dsb                            /* Ensure completion of TLB+BP flush */
        isb

        mov pc, lr
END_FUNCTION(switch_ttbr)
#endif



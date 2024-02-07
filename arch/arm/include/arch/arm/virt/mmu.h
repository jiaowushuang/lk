/*
 * Copyright (c) 2014 Google Inc. All rights reserved
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#pragma once

#include <arch/defines.h>

#define SECTION_SIZE      (2*MB)

#ifndef MMU_KERNEL_SIZE_SHIFT
#define MMU_KERNEL_SIZE_SHIFT (32) /* in current design, kernel aspace must be max 32 bits */
#endif

#ifndef MMU_USER_SIZE_SHIFT
#define MMU_USER_SIZE_SHIFT             (32) /* in current design, user aspace can be max 32 bits(40 TODO:) */
#endif

#define MMU_KERNEL_PAGE_SIZE_SHIFT      (PAGE_SIZE_SHIFT)
#define MMU_USER_PAGE_SIZE_SHIFT        (PAGE_SIZE_SHIFT)

#define MMU_LX_X(page_shift, level) ((4 - (level)) * ((page_shift) - 3) + 3)

#if MMU_USER_SIZE_SHIFT > MMU_LX_X(MMU_USER_PAGE_SIZE_SHIFT, 0)
#define MMU_USER_TOP_SHIFT MMU_LX_X(MMU_USER_PAGE_SIZE_SHIFT, 0)
#elif MMU_USER_SIZE_SHIFT > MMU_LX_X(MMU_USER_PAGE_SIZE_SHIFT, 1)
#define MMU_USER_TOP_SHIFT MMU_LX_X(MMU_USER_PAGE_SIZE_SHIFT, 1)
#elif MMU_USER_SIZE_SHIFT > MMU_LX_X(MMU_USER_PAGE_SIZE_SHIFT, 2)
#define MMU_USER_TOP_SHIFT MMU_LX_X(MMU_USER_PAGE_SIZE_SHIFT, 2)
#elif MMU_USER_SIZE_SHIFT > MMU_LX_X(MMU_USER_PAGE_SIZE_SHIFT, 3)
#define MMU_USER_TOP_SHIFT MMU_LX_X(MMU_USER_PAGE_SIZE_SHIFT, 3)
#else
#error User address space size must be larger than page size
#endif
#define MMU_USER_PAGE_TABLE_ENTRIES_TOP (0x1 << (MMU_USER_SIZE_SHIFT - MMU_USER_TOP_SHIFT))

#if MMU_KERNEL_SIZE_SHIFT > MMU_LX_X(MMU_KERNEL_PAGE_SIZE_SHIFT, 0)
#define MMU_KERNEL_TOP_SHIFT MMU_LX_X(MMU_KERNEL_PAGE_SIZE_SHIFT, 0)
#elif MMU_KERNEL_SIZE_SHIFT > MMU_LX_X(MMU_KERNEL_PAGE_SIZE_SHIFT, 1)
#define MMU_KERNEL_TOP_SHIFT MMU_LX_X(MMU_KERNEL_PAGE_SIZE_SHIFT, 1)
#elif MMU_KERNEL_SIZE_SHIFT > MMU_LX_X(MMU_KERNEL_PAGE_SIZE_SHIFT, 2)
#define MMU_KERNEL_TOP_SHIFT MMU_LX_X(MMU_KERNEL_PAGE_SIZE_SHIFT, 2)
#elif MMU_KERNEL_SIZE_SHIFT > MMU_LX_X(MMU_KERNEL_PAGE_SIZE_SHIFT, 3)
#define MMU_KERNEL_TOP_SHIFT MMU_LX_X(MMU_KERNEL_PAGE_SIZE_SHIFT, 3)
#else
#error Kernel address space size must be larger than page size
#endif
#define MMU_KERNEL_PAGE_TABLE_ENTRIES_TOP (0x1 << (MMU_KERNEL_SIZE_SHIFT - MMU_KERNEL_TOP_SHIFT))

#define MMU_KERNEL_PAGE_TABLE_ENTRIES           (512)
#define MMU_PTE_DESCRIPTOR_BLOCK_MAX_SHIFT      (30)

#ifndef ASSEMBLY
#define BM(base, count, val) (((val) & ((1ULL << (count)) - 1)) << (base))
#else
#define BM(base, count, val) (((val) & ((0x1 << (count)) - 1)) << (base))
#endif

#define LPAE_SHIFT      9
#define LPAE_ENTRIES    (1U << LPAE_SHIFT)
#define LPAE_ENTRY_MASK (LPAE_ENTRIES - 1)

#define THIRD_SHIFT    (PAGE_SIZE_SHIFT)
#define THIRD_ORDER    (THIRD_SHIFT - PAGE_SIZE_SHIFT)
#define THIRD_SIZE     (1U << THIRD_SHIFT)
#define THIRD_MASK     (~(THIRD_SIZE - 1))

#define SECOND_SHIFT   (THIRD_SHIFT + LPAE_SHIFT)
#define SECOND_ORDER   (SECOND_SHIFT - PAGE_SIZE_SHIFT)
#define SECOND_SIZE    (1U << SECOND_SHIFT)
#define SECOND_MASK    (~(SECOND_SIZE - 1))

#define FIRST_SHIFT    (SECOND_SHIFT + LPAE_SHIFT)
#define FIRST_ORDER    (FIRST_SHIFT - PAGE_SIZE_SHIFT)
#define FIRST_SIZE     (1U << FIRST_SHIFT)
#define FIRST_MASK     (~(FIRST_SIZE - 1))

#define FIRST_LINEAR_OFFSET(va) ((va) >> FIRST_SHIFT)
#define SECOND_LINEAR_OFFSET(va) ((va) >> SECOND_SHIFT)
#define THIRD_LINEAR_OFFSET(va) ((va) >> THIRD_SHIFT)

#define TABLE_OFFSET(offs) (offs & LPAE_ENTRY_MASK)
#define FIRST_TABLE_OFFSET(va)  TABLE_OFFSET(FIRST_LINEAR_OFFSET(va))
#define SECOND_TABLE_OFFSET(va) TABLE_OFFSET(SECOND_LINEAR_OFFSET(va))
#define THIRD_TABLE_OFFSET(va)  TABLE_OFFSET(THIRD_LINEAR_OFFSET(va))

/* TTBCR/VTCR/HTCR bits descriptions */
/* Only use TTBR0 whether kern/user(Non-secure PL1&0 VA->IPA), 
 * hyp(Non-secure PL2|Non-secure PL1&0 IPA->PA), mon(Secure L1&0)
 */
// TTBCR/HTCR
#ifdef WITH_HYPER_MODE

#define MMU_T0SZ                                (0)
#if MMU_T0SZ > 1
#define MMU_KERNEL_ALIGN_SIZE_SHIFT                (14 - MMU_T0SZ) // first pd align_size
#else
#define MMU_KERNEL_ALIGN_SIZE_SHIFT                (5 - MMU_T0SZ)
#endif /* MMU_T0SZ */

// VTCR only
/* Refer Table B3-5 Input address range constraints on programming VTCR */
#define MMU_VTG0_SIZE_SHIFT                     (4)
#if MMU_USER_SIZE_SHIFT <= MMU_PTE_DESCRIPTOR_BLOCK_MAX_SHIFT

#define MMU_VT0SZ                               (2)
#define MMU_VT0SZ_S                             (MMU_VT0SZ & (1UL << (MMU_VTG0_SIZE_SHIFT-1))) >> (MMU_VTG0_SIZE_SHIFT-1)
#define MMU_USER_ALIGN_SIZE_SHIFT                (14 - MMU_VT0SZ)
#define MMU_VTCR_VSL0                                BM(6, 2, 0) /* second level */
#define MMU_VTCR_VS                                  BM(4, 1, MMU_VT0SZ_S)

#else

#if MMU_USER_SIZE_SHIFT <= 34
#define MMU_VT0SZ                               (0)
#else
#define MMU_VT0SZ                               (-7)
#endif /* MMU_USER_SIZE_SHIFT */

#define MMU_VT0SZ_S                             (MMU_VT0SZ & (1UL << (MMU_VTG0_SIZE_SHIFT-1))) >> (MMU_VTG0_SIZE_SHIFT-1)
#define MMU_USER_ALIGN_SIZE_SHIFT                (5 - MMU_VT0SZ) 
#define MMU_VTCR_VSL0                                BM(6, 2, 1) /* first level */
#define MMU_VTCR_VS                                  BM(4, 1, MMU_VT0SZ_S)   

#endif /* MMU_USER_SIZE_SHIFT */

#else /* WITH_HYPER_MODE */

#define MMU_T0SZ                                (0)
#if MMU_T0SZ > 1
#define MMU_USER_ALIGN_SIZE_SHIFT                (14 - MMU_T0SZ) // first pd align_size
#else
#define MMU_USER_ALIGN_SIZE_SHIFT                (5 - MMU_T0SZ)
#endif /* MMU_T0SZ */

// TTBCR only
#define MMU_T1SZ                                (0)
#if MMU_T1SZ > 1
#define MMU_KERNEL_ALIGN_SIZE_SHIFT                (14 - MMU_T1SZ)
#else
#define MMU_KERNEL_ALIGN_SIZE_SHIFT                (5 - MMU_T1SZ)
#endif /* MMU_T1SZ */

#endif /* WITH_HYPER_MODE */

// TTBCR only
#define MMU_TCR_EAE                             BM(31, 1, 1)
#define MMU_TCR_SH1(shareability_flags)         BM(28, 2, (shareability_flags))
#define MMU_TCR_ORGN1(cache_flags)              BM(26, 2, (cache_flags))
#define MMU_TCR_IRGN1(cache_flags)              BM(24, 2, (cache_flags))
#define MMU_TCR_EPD1                            BM(23, 1, 1)
#define MMU_TCR_A1                              BM(22, 1, 1)
#define MMU_TCR_T1SZ(size)                      BM(16, 6, (size))

  
#define MMU_SH_NON_SHAREABLE                    (0)
#define MMU_SH_OUTER_SHAREABLE                  (2)
#define MMU_SH_INNER_SHAREABLE                  (3)

#define MMU_RGN_NON_CACHEABLE                   (0)
#define MMU_RGN_WRITE_BACK_ALLOCATE             (1)
#define MMU_RGN_WRITE_THROUGH_NO_ALLOCATE       (2)
#define MMU_RGN_WRITE_BACK_NO_ALLOCATE          (3)

// TTBCR/HTCR/VTCR
#define MMU_TCR_SH0(shareability_flags)         BM(12, 2, (shareability_flags))
#define MMU_TCR_ORGN0(cache_flags)              BM(10, 2, (cache_flags))
#define MMU_TCR_IRGN0(cache_flags)              BM( 8, 2, (cache_flags))
#define MMU_TCR_EPD0                            BM( 7, 1, 1)
/* TTBCR/HTCR(0-2) 
 * VTCR(0-3) + S(4) + SL0(6-7)
 */
#define MMU_TCR_T0SZ(size)                      BM( 0, 6, (size))

/* AttrIndx[2], from the translation table descriptor, selects the appropriate HMAIR:
 * Setting AttrIndx[2] to 0 selects HMAIR0.
 * Setting AttrIndx[2] to 1 selects HMAIR1.
 */
/* AttrIndx[2] indicates the value of n in MAIRn:
 * AttrIndx[2] == 0 Use MAIR0.
 * AttrIndx[2] == 1 Use MAIR1.
 * AttrIndx[2:0] indicates the required Attr field, Attrn, where n = AttrIndx[2:0].
 */
/* HMAIR/MAIR, only stage 1 */
#define MMU_MAIR_ATTR(index, attr)              BM(index * 8, 8, (attr)) // index = AttrIndx[1:0](0-3)

/* L1/L2/L3 descriptor types */
#define MMU_PTE_DESCRIPTOR_INVALID              BM(0, 2, 0)
#define MMU_PTE_DESCRIPTOR_MASK                 BM(0, 2, 3)

/* L1/L2 descriptor types */
#define MMU_PTE_L12_DESCRIPTOR_BLOCK           BM(0, 2, 1)
#define MMU_PTE_L12_DESCRIPTOR_TABLE           BM(0, 2, 3)

/* L3 descriptor types */
#define MMU_PTE_L3_DESCRIPTOR_PAGE              BM(0, 2, 3)

/* Output address mask */
#define MMU_PTE_OUTPUT_ADDR_MASK                BM(12, 20, 0xfffffffff) /* max 32 bits */

/* Table attrs only stage 1 */
#define MMU_PTE_ATTR_NS_TABLE                   BM(63, 1, 1)
#define MMU_PTE_ATTR_AP_TABLE_NO_WRITE          BM(62, 1, 1)
#define MMU_PTE_ATTR_AP_TABLE_NO_EL0            BM(61, 1, 1)
#define MMU_PTE_ATTR_UXN_TABLE                  BM(60, 1, 1)
#define MMU_PTE_ATTR_PXN_TABLE                  BM(59, 1, 1)

/* Block/Page attrs stage 1 all, stage 2 part */
#define MMU_PTE_ATTR_RES_SOFTWARE               BM(55, 4, 0xf)
#define MMU_PTE_ATTR_UXN                        BM(54, 1, 1)
#define MMU_PTE_ATTR_PXN                        BM(53, 1, 1) // stage 2 ignore 
#define MMU_PTE_ATTR_CONTIGUOUS                 BM(52, 1, 1)

#define MMU_PTE_ATTR_NON_GLOBAL                 BM(11, 1, 1) // stage 2 ignore 
#define MMU_PTE_ATTR_AF                         BM(10, 1, 1)

#define MMU_PTE_ATTR_SH_NON_SHAREABLE           BM(8, 2, 0)
#define MMU_PTE_ATTR_SH_OUTER_SHAREABLE         BM(8, 2, 2)
#define MMU_PTE_ATTR_SH_INNER_SHAREABLE         BM(8, 2, 3)

// stage 1
#define MMU_PTE_ATTR_AP_P_RW_U_NA               BM(6, 2, 0)
#define MMU_PTE_ATTR_AP_P_RW_U_RW               BM(6, 2, 1)
#define MMU_PTE_ATTR_AP_P_RO_U_NA               BM(6, 2, 2)
#define MMU_PTE_ATTR_AP_P_RO_U_RO               BM(6, 2, 3)
// stage 2
#define MMU_PTE_ATTR_HAP_NA                     BM(6, 2, 0)
#define MMU_PTE_ATTR_HAP_RO                     BM(6, 2, 1)
#define MMU_PTE_ATTR_HAP_WO                     BM(6, 2, 2) /* in current design, no use */
#define MMU_PTE_ATTR_HAP_RW                     BM(6, 2, 3)

#define MMU_PTE_ATTR_AP_MASK                    BM(6, 2, 3)

#define MMU_PTE_ATTR_NON_SECURE                 BM(5, 1, 1) // stage 2 ignore 

// stage 1
#define MMU_PTE_ATTR_ATTR_INDEX(attrindex)      BM(2, 3, attrindex)
#define MMU_PTE_ATTR_ATTR_INDEX_MASK            MMU_PTE_ATTR_ATTR_INDEX(7)
/* Default configuration for main kernel page table: only stage 1
 *    - do cached translation walks
 */

/* Device-nGnRE memory */
#define MMU_MAIR_ATTR4                  MMU_MAIR_ATTR(0, 0x04)
#define MMU_PTE_ATTR_DEVICE             MMU_PTE_ATTR_ATTR_INDEX(4)

/* Device-nGnRnE memory */
#define MMU_MAIR_ATTR5                  MMU_MAIR_ATTR(1, 0x00)
#define MMU_PTE_ATTR_STRONGLY_ORDERED   MMU_PTE_ATTR_ATTR_INDEX(5)

#define MMU_MAIR_ATTR6                  0

/* Normal Memory, Outer Write-back non-transient Read/Write allocate,
 * Inner Write-back non-transient Read/Write allocate
 */
#define MMU_MAIR_ATTR7                  MMU_MAIR_ATTR(3, 0xff)
#define MMU_PTE_ATTR_NORMAL_MEMORY      MMU_PTE_ATTR_ATTR_INDEX(7)


#define MMU_MAIR_ATTR0                  (0) // index = 0
#define MMU_MAIR_ATTR1                  (0) // index = 1
#define MMU_MAIR_ATTR2                  (0) // index = 2
#define MMU_MAIR_ATTR3                  (0) // index = 3

#define MMU_MAIR0_VAL                    (MMU_MAIR_ATTR0 | MMU_MAIR_ATTR1 | \
                                         MMU_MAIR_ATTR2 | MMU_MAIR_ATTR3)
#define MMU_MAIR1_VAL                    (MMU_MAIR_ATTR4 | MMU_MAIR_ATTR5 | \
                                         MMU_MAIR_ATTR6 | MMU_MAIR_ATTR7 )

// stage 2 
#define MMU_PTE_ATTR_MEM_ATTR(memattr)          BM(2, 4, memattr)
#define MMU_PTE_ATTR_MEM_ATTR_MASK              MMU_PTE_ATTR_MEM_ATTR(0xf)

/* Device-nGnRnE memory */
#define MMU_MEM_ATTR_STRONGLY_ORDERED           MMU_PTE_ATTR_MEM_ATTR(0) 
/* Device-nGnRE memory */
#define MMU_MEM_ATTR_DEVICE                     MMU_PTE_ATTR_MEM_ATTR(1)
/* Normal Memory, Outer Write-back non-transient Read/Write allocate,
 * Inner Write-back non-transient Read/Write allocate
 */
#define MMU_MEM_ATTR_NORMAL_MEMORY              MMU_PTE_ATTR_MEM_ATTR(0xf)


/* Enable cached page table walks:
 * inner/outer (IRGN/ORGN): write-back + write-allocate
 */
/* TTBCR/HTCR */
#define MMU_TCR_FLAGS1 (MMU_TCR_SH1(MMU_SH_INNER_SHAREABLE) | \
                        MMU_TCR_ORGN1(MMU_RGN_WRITE_BACK_ALLOCATE) | \
                        MMU_TCR_IRGN1(MMU_RGN_WRITE_BACK_ALLOCATE) | \
                        MMU_TCR_T1SZ(MMU_T1SZ))
#define MMU_TCR_FLAGS0 (MMU_TCR_SH0(MMU_SH_INNER_SHAREABLE) | \
                        MMU_TCR_ORGN0(MMU_RGN_WRITE_BACK_ALLOCATE) | \
                        MMU_TCR_IRGN0(MMU_RGN_WRITE_BACK_ALLOCATE) | \
                        MMU_TCR_T0SZ(MMU_T0SZ))
#define MMU_TCR_FLAGS_BASE (MMU_TCR_EAE | MMU_TCR_FLAGS0) // MMU_TCR_FLAGS1 ignore in current design
/* TTBCR only */
#define MMU_TCR_FLAGS_KERNEL (MMU_TCR_EPD0)
#define MMU_TCR_FLAGS_USER (0)
/* VTCR only */ 
#define MMU_VTCR_FLAGS0 (MMU_TCR_SH0(MMU_SH_INNER_SHAREABLE) | \
                        MMU_TCR_ORGN0(MMU_RGN_WRITE_BACK_ALLOCATE) | \
                        MMU_TCR_IRGN0(MMU_RGN_WRITE_BACK_ALLOCATE) | \
                        MMU_TCR_T0SZ(MMU_VT0SZ))
#define MMU_VTCR_FLAGS_BASE (MMU_VTCR_FLAGS0 | MMU_VTCR_VSL0 | MMU_VTCR_VS)

// stage 1 only, aarch32

/* Legacy low 32 bits */
#define MMU_INITIAL_MAP_TABLE (MMU_PTE_L12_DESCRIPTOR_TABLE)

#define MMU_INITIAL_MAP_NORMAL_TABLE \
    (MMU_PTE_ATTR_NON_GLOBAL| \
     MMU_PTE_ATTR_AF | \
     MMU_PTE_ATTR_SH_INNER_SHAREABLE | \
     MMU_PTE_ATTR_NON_SECURE | \
     MMU_PTE_ATTR_NORMAL_MEMORY | \
     MMU_PTE_ATTR_AP_P_RW_U_RW | \
     MMU_PTE_L12_DESCRIPTOR_TABLE)

#define MMU_INITIAL_MAP_DEVICE_TABLE \
    (MMU_PTE_ATTR_NON_GLOBAL| \
     MMU_PTE_ATTR_AF | \
     MMU_PTE_ATTR_SH_OUTER_SHAREABLE | \
     MMU_PTE_ATTR_NON_SECURE | \
     MMU_PTE_ATTR_DEVICE | \
     MMU_PTE_ATTR_AP_P_RW_U_RW | \
     MMU_PTE_L12_DESCRIPTOR_TABLE)

#define MMU_INITIAL_MAP_NORMAL_BLOCK \
    (MMU_PTE_ATTR_NON_GLOBAL| \
     MMU_PTE_ATTR_AF | \
     MMU_PTE_ATTR_SH_INNER_SHAREABLE | \
     MMU_PTE_ATTR_NON_SECURE | \
     MMU_PTE_ATTR_NORMAL_MEMORY | \
     MMU_PTE_ATTR_AP_P_RW_U_RW | \
     MMU_PTE_L12_DESCRIPTOR_BLOCK)

#define MMU_INITIAL_MAP_STRONGLY_ORDERED_BLOCK \
    (MMU_PTE_ATTR_NON_GLOBAL| \
     MMU_PTE_ATTR_AF | \
     MMU_PTE_ATTR_SH_OUTER_SHAREABLE | \
     MMU_PTE_ATTR_STRONGLY_ORDERED | \
     MMU_PTE_ATTR_NON_SECURE | \
     MMU_PTE_ATTR_AP_P_RW_U_RW | \
     MMU_PTE_L12_DESCRIPTOR_BLOCK)


#define MMU_INITIAL_MAP_DEVICE_BLOCK \
    (MMU_PTE_ATTR_NON_GLOBAL| \
     MMU_PTE_ATTR_AF | \
     MMU_PTE_ATTR_SH_OUTER_SHAREABLE | \
     MMU_PTE_ATTR_DEVICE | \
     MMU_PTE_ATTR_NON_SECURE | \
     MMU_PTE_ATTR_AP_P_RW_U_RW | \
     MMU_PTE_L12_DESCRIPTOR_BLOCK)

#define MMU_INITIAL_MAP_NORMAL_PAGE \
    (MMU_PTE_ATTR_NON_GLOBAL| \
     MMU_PTE_ATTR_AF | \
     MMU_PTE_ATTR_SH_INNER_SHAREABLE | \
     MMU_PTE_ATTR_NON_SECURE | \
     MMU_PTE_ATTR_NORMAL_MEMORY | \
     MMU_PTE_ATTR_AP_P_RW_U_RW | \
     MMU_PTE_L3_DESCRIPTOR_PAGE)

#define MMU_INITIAL_MAP_STRONGLY_ORDERED_PAGE \
    (MMU_PTE_ATTR_NON_GLOBAL| \
     MMU_PTE_ATTR_AF | \
     MMU_PTE_ATTR_SH_OUTER_SHAREABLE | \
     MMU_PTE_ATTR_STRONGLY_ORDERED | \
     MMU_PTE_ATTR_NON_SECURE | \
     MMU_PTE_ATTR_AP_P_RW_U_RW | \
     MMU_PTE_L3_DESCRIPTOR_PAGE)


#define MMU_INITIAL_MAP_DEVICE_PAGE \
    (MMU_PTE_ATTR_NON_GLOBAL| \
     MMU_PTE_ATTR_AF | \
     MMU_PTE_ATTR_SH_OUTER_SHAREABLE | \
     MMU_PTE_ATTR_DEVICE | \
     MMU_PTE_ATTR_NON_SECURE | \
     MMU_PTE_ATTR_AP_P_RW_U_RW | \
     MMU_PTE_L3_DESCRIPTOR_PAGE)

#ifndef ASSEMBLY

#include <sys/types.h>
#include <assert.h>
#include <kern/compiler.h>
#include <arch/arm.h>
#include <arch/arch_helpers.h>

typedef uint64_t pte_t;

__BEGIN_CDECLS

void arm_mmu_early_init(void);
void arm_mmu_init(void);
status_t arm_vtop(addr_t va, addr_t *pa);

/* tlb routines */

#ifdef WITH_HYPER_MODE
static inline void arm_after_invalidate_tlb_barrier(void) {
#if WITH_SMP
    arm_write_bpiallis(0);
#else
    arm_write_bpiall(0);
#endif
    DSB;
    ISB;
}

static inline void arm_invalidate_tlb_global_no_barrier(void) {
#if WITH_SMP
    tlbiallhis();
#else
    tlbiallh();
#endif
}

static inline void arm_invalidate_tlb_global(void) {
    DSB;
    arm_invalidate_tlb_global_no_barrier();
    arm_after_invalidate_tlb_barrier();
}

static inline void arm_invalidate_tlb2_global_no_barrier(void) {
#if WITH_SMP
    arm_write_tlbiallis(0);
#else
    arm_write_tlbiall(0);
#endif
}

static inline void arm_invalidate_tlb2_global(void) {
    DSB;
    arm_invalidate_tlb2_global_no_barrier();
    arm_after_invalidate_tlb_barrier();
}

static inline void arm_invalidate_tlb_mva_no_barrier(vaddr_t va) {
	(void)va;
#if WITH_SMP
	tlbiallnsnhis();
#else
    tlbiallnsnh();
#endif
}

static inline void arm_invalidate_tlb_mva(vaddr_t va) {
    DSB;
    arm_invalidate_tlb_mva_no_barrier(va);
    arm_after_invalidate_tlb_barrier();
}

static inline void arm_invalidate_tlb_asid_no_barrier(uint8_t asid) {
	(void)asid;
#if WITH_SMP
	tlbiallnsnhis();
#else
    tlbiallnsnh();
#endif
}

static inline void arm_invalidate_tlb_asid(uint8_t asid) {
    DSB;
    arm_invalidate_tlb_asid_no_barrier(asid);
    arm_after_invalidate_tlb_barrier();
}

static inline void arm_invalidate_tlb_mva_asid_no_barrier(vaddr_t va, uint8_t asid) {
#if WITH_SMP
    tlbimvahis((va & 0xfffff000) | asid);
#else
    tlbimvah((va & 0xfffff000) | asid);
#endif
}

static inline void arm_invalidate_tlb_mva_asid(vaddr_t va, uint8_t asid) {
    DSB;
    arm_invalidate_tlb_mva_asid_no_barrier(va, asid);
    arm_after_invalidate_tlb_barrier();
}
#else
static inline void arm_after_invalidate_tlb_barrier(void) {
#if WITH_SMP
    arm_write_bpiallis(0);
#else
    arm_write_bpiall(0);
#endif
    DSB;
    ISB;
}

static inline void arm_invalidate_tlb_global_no_barrier(void) {
#if WITH_SMP
    arm_write_tlbiallis(0);
#else
    arm_write_tlbiall(0);
#endif
}

static inline void arm_invalidate_tlb_global(void) {
    DSB;
    arm_invalidate_tlb_global_no_barrier();
    arm_after_invalidate_tlb_barrier();
}

static inline void arm_invalidate_tlb_mva_no_barrier(vaddr_t va) {
#if WITH_SMP
    arm_write_tlbimvaais(va & 0xfffff000);
#else
    arm_write_tlbimvaa(va & 0xfffff000);
#endif
}

static inline void arm_invalidate_tlb_mva(vaddr_t va) {
    DSB;
    arm_invalidate_tlb_mva_no_barrier(va);
    arm_after_invalidate_tlb_barrier();
}


static inline void arm_invalidate_tlb_asid_no_barrier(uint8_t asid) {
#if WITH_SMP
    arm_write_tlbiasidis(asid);
#else
    arm_write_tlbiasid(asid);
#endif
}

static inline void arm_invalidate_tlb_asid(uint8_t asid) {
    DSB;
    arm_invalidate_tlb_asid_no_barrier(asid);
    arm_after_invalidate_tlb_barrier();
}

static inline void arm_invalidate_tlb_mva_asid_no_barrier(vaddr_t va, uint8_t asid) {
#if WITH_SMP
    arm_write_tlbimvais((va & 0xfffff000) | asid);
#else
    arm_write_tlbimva((va & 0xfffff000) | asid);
#endif
}

static inline void arm_invalidate_tlb_mva_asid(vaddr_t va, uint8_t asid) {
    DSB;
    arm_invalidate_tlb_mva_asid_no_barrier(va, asid);
    arm_after_invalidate_tlb_barrier();
}
#endif /* WITH_HYPER_MODE */

#define MMU_ARM_GLOBAL_ASID (~0U)
#define MMU_ARM_GLOBAL_VMID ((~0U) - 1)
#define MMU_ARM_USER_VMID (1U)
#define MMU_ARM_USER_ASID (0U)

int arm_mmu_map(vaddr_t vaddr, paddr_t paddr, size_t size, pte_t attrs,
                  vaddr_t vaddr_base, uint top_size_shift,
                  uint top_index_shift, uint page_size_shift,
                  pte_t *top_page_table, uint asid);
int arm_mmu_unmap(vaddr_t vaddr, size_t size,
                    vaddr_t vaddr_base, uint top_size_shift,
                    uint top_index_shift, uint page_size_shift,
                    pte_t *top_page_table, uint asid);
status_t arch_mmu_at_kernel(vaddr_t vaddr, paddr_t *paddr, uint flags);
status_t arch_mmu_at_user(vaddr_t vaddr, paddr_t *paddr, uint flags);
__END_CDECLS

#endif /* ASSEMBLY */

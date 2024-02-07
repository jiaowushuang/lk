/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <stdbool.h>
#include <kern/bitops.h>
#include <arch/arch_helpers.h>
#if WITH_DEV_CACHE_PL310
#include <dev/cache/pl310.h>
#endif

#define wordBits 64
#define MRS(reg, v)  asm volatile("mrs %x0," reg : "=r"(v))
#define MSR(reg, v)                                \
    do {                                           \
        word_t _v = v;                             \
        asm volatile("msr " reg ",%x0" :: "r" (_v));\
    }while(0)

static inline void cleanByWSL(uint64_t wsl)
{
    asm volatile("dc csw, %0" : : "r"(wsl));
}

static inline void cleanInvalidateByWSL(uint64_t wsl)
{
    asm volatile("dc cisw, %0" : : "r"(wsl));
}

static inline uint64_t readCLID(void)
{
    uint64_t CLID;
    MRS("clidr_el1", CLID);
    return CLID;
}

#define LOUU(x)    (((x) >> 27)        & MASK(3))
#define LOC(x)     (((x) >> 24)        & MASK(3))
#define LOUIS(x)   (((x) >> 21)        & MASK(3))
#define CTYPE(x,n) (((x) >> (n*3))     & MASK(3))

enum arm_cache_type {
    ARMCacheI =    1,
    ARMCacheD =    2,
    ARMCacheID =   3,
};

static inline uint64_t readCacheSize(int level, bool instruction)
{
    uint64_t size, csselr_old;
    /* Save CSSELR */
    MRS("csselr_el1", csselr_old);
    /* Select cache level */
    MSR("csselr_el1", ((level << 1) | instruction));
    /* Read 'size' */
    MRS("ccsidr_el1", size);
    /* Restore CSSELR */
    MSR("csselr_el1", csselr_old);
    return size;
}

#define LINEBITS(s)     (((s) & MASK(3)) + 4)
#define ASSOC(s)        ((((s) >> 3) & MASK(10)) + 1)
#define NSETS(s)        ((((s) >> 13) & MASK(15)) + 1)

void clean_D_PoU(void)
{
    int clid = readCLID();
    int lou = LOUU(clid);

    for (int l = 0; l < lou; l++) {
        if (CTYPE(clid, l) > ARMCacheI) {
            uint64_t lsize = readCacheSize(l, 0);
            int lbits = LINEBITS(lsize);
            int assoc = ASSOC(lsize);
            int assoc_bits = wordBits - clzl(assoc - 1);
            int nsets = NSETS(lsize);
            for (int w = 0; w < assoc; w++) {
                for (int s = 0; s < nsets; s++) {
                    cleanByWSL((w << (32 - assoc_bits)) |
                               (s << lbits) | (l << 1));
                }
            }
        }
    }
}

static inline void cleanInvalidate_D_by_level(int l)
{
    uint64_t lsize = readCacheSize(l, 0);
    int lbits = LINEBITS(lsize);
    int assoc = ASSOC(lsize);
    int assoc_bits = wordBits - clzl(assoc - 1);
    int nsets = NSETS(lsize);

    for (int w = 0; w < assoc; w++) {
        for (int s = 0; s < nsets; s++) {
            cleanInvalidateByWSL((w << (32 - assoc_bits)) |
                                 (s << lbits) | (l << 1));
        }
    }
}

void cleanInvalidate_D_PoC(void)
{
    int clid = readCLID();
    int loc = LOC(clid);

    for (int l = 0; l < loc; l++) {
        if (CTYPE(clid, l) > ARMCacheI) {
            cleanInvalidate_D_by_level(l);
        }
    }
}

void cleanInvalidate_L1D(void)
{
    cleanInvalidate_D_by_level(0);
}


void invalidate_I_PoU(void)
{
#if WITH_SMP > 1
    asm volatile("ic ialluis");
#else
    asm volatile("ic iallu");
#endif
    isb();
}

void cleanCaches_PoU(void)
{
    dsb();
    clean_D_PoU();
    dsb();
    invalidate_I_PoU();
    dsb();
}

void cleanInvalidateL1Caches(void)
{
    dsb();
    cleanInvalidate_D_PoC();
    dsb();
    invalidate_I_PoU();
    dsb();
}

void arch_clean_invalidate_caches(void)
{
    cleanCaches_PoU();
#if WITH_DEV_CACHE_PL310
    pl310_flush_invalidate();
#endif
    cleanInvalidateL1Caches();
    isb();
}

void arch_clean_invalidate_L1_caches(unsigned int type)
{
    dsb();
    if (type & BIT(1)) {
        cleanInvalidate_L1D();
        dsb();
    }
    if (type & BIT(0)) {
        invalidate_I_PoU();
        dsb();
        isb();
    }
}
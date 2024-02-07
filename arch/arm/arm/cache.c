/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */


#include <stdbool.h>
#include <kern/bitops.h>
#include <arch/arch_helpers.h>
#if WITH_DEV_CACHE_PL310
#include <dev/cache/pl310.h>
#endif

#define wordBits 32

static inline void cleanByWSL(uint32_t wsl)
{
    asm volatile("mcr p15, 0, %0, c7, c10, 2" : : "r"(wsl));
}

static inline void cleanInvalidateByWSL(uint32_t wsl)
{
    asm volatile("mcr p15, 0, %0, c7, c14, 2" : : "r"(wsl));
}


static inline uint32_t readCLID(void)
{
    uint32_t CLID;
    asm volatile("mrc p15, 1, %0, c0, c0, 1" : "=r"(CLID));
    return CLID;
}

#define LOUU(x)    (((x) >> 27)        & MASK(3))
#define LOC(x)     (((x) >> 24)        & MASK(3))
#define LOUIS(x)   (((x) >> 21)        & MASK(3))
#define CTYPE(x,n) (((x) >> (n*3))     & MASK(3))

enum arm_cache_type {
    ARMCacheNone = 0,
    ARMCacheI =    1,
    ARMCacheD =    2,
    ARMCacheID =   3,
    ARMCacheU =    4,
};


static inline uint32_t readCacheSize(int level, bool instruction)
{
    uint32_t size_unique_name, csselr_old;
    /* Save CSSELR */
    asm volatile("mrc p15, 2, %0, c0, c0, 0" : "=r"(csselr_old));
    /* Select cache level */
    asm volatile("mcr p15, 2, %0, c0, c0, 0" : : "r"((level << 1) | instruction));
    /* Read 'size' */
    asm volatile("mrc p15, 1, %0, c0, c0, 0" : "=r"(size_unique_name));
    /* Restore CSSELR */
    asm volatile("mcr p15, 2, %0, c0, c0, 0" : : "r"(csselr_old));
    return size_unique_name;
}

/* Number of bits to index within a cache line.  The field is log2(nwords) - 2
 * , and thus by adding 4 we get log2(nbytes). */
#define LINEBITS(s) (( (s)        & MASK(3))  + 4)
/* Associativity, field is assoc - 1. */
#define ASSOC(s)    ((((s) >> 3)  & MASK(10)) + 1)
/* Number of sets, field is nsets - 1. */
#define NSETS(s)    ((((s) >> 13) & MASK(15)) + 1)


void clean_D_PoU(void)
{
    int clid = readCLID();
    int lou = LOUU(clid);
    int l;

    for (l = 0; l < lou; l++) {
        if (CTYPE(clid, l) > ARMCacheI) {
            uint32_t s = readCacheSize(l, 0);
            int lbits = LINEBITS(s);
            int assoc = ASSOC(s);
            int assoc_bits = wordBits - clzl(assoc - 1);
            int nsets = NSETS(s);
            int w;

            for (w = 0; w < assoc; w++) {
                int v;

                for (v = 0; v < nsets; v++) {
                    cleanByWSL((w << (32 - assoc_bits)) |
                               (v << lbits) | (l << 1));
                }
            }
        }
    }
}

static inline void cleanInvalidate_D_by_level(int l)
{
    uint32_t s = readCacheSize(l, 0);
    int lbits = LINEBITS(s);
    int assoc = ASSOC(s);
    int assoc_bits = wordBits - clzl(assoc - 1);
    int nsets = NSETS(s);
    int w;

    for (w = 0; w < assoc; w++) {
        int v;

        for (v = 0; v < nsets; v++) {
            cleanInvalidateByWSL((w << (32 - assoc_bits)) |
                                 (v << lbits) | (l << 1));
        }
    }
}

void cleanInvalidate_D_PoC(void)
{
    int clid = readCLID();
    int loc = LOC(clid);
    int l;

    for (l = 0; l < loc; l++) {
        if (CTYPE(clid, l) > ARMCacheI) {
            cleanInvalidate_D_by_level(l);
        }
    }
}

void cleanInvalidate_L1D(void)
{
    cleanInvalidate_D_by_level(0);
}

/* I-Cache invalidate all to PoU (L2 cache) (v6/v7 common) */
void invalidate_I_PoU(void)
{
#ifdef ARM_CPU_CORTEX_A8
    /* Erratum 586324 -- perform a dummy cached load before flushing. */
    asm volatile("ldr r0, [sp]" : : : "r0");
#endif
    asm volatile("mcr p15, 0, %0, c7, c5, 0" : : "r"(0));
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
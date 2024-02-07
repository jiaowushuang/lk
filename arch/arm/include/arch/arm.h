/*
 * Copyright (c) 2008-2013 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#ifndef __ARCH_ARM_H
#define __ARCH_ARM_H

#include <stdbool.h>
#include <sys/types.h>
#include <arch/arm/cores.h>
#include <kern/compiler.h>

/* due to the cp15 accessors below, you're gonna have a bad time if you try
 * to compile in thumb mode. Either compile in ARM only or get a thumb2 capable cpu.

#if defined(__thumb__) && !defined(__thumb2__)
#error this file unsupported in thumb1 mode
#endif
*/
__BEGIN_CDECLS

#if ARM_ISA_ARMV7 || ARM_ISA_ARMV8
#define DSB __asm__ volatile("dsb" ::: "memory")
#define DMB __asm__ volatile("dmb" ::: "memory")
#define ISB __asm__ volatile("isb" ::: "memory")
#elif ARM_ISA_ARMV6 || ARM_ISA_ARMV6M
#define DSB __asm__ volatile("mcr p15, 0, %0, c7, c10, 4" :: "r" (0) : "memory")
#define ISB __asm__ volatile("mcr p15, 0, %0, c7, c5, 4" :: "r" (0) : "memory")
#define DMB __asm__ volatile("nop")
#else
#error unhandled arm isa
#endif
#define NOP __asm__ volatile("nop");

void arm_context_switch(vaddr_t *old_sp, vaddr_t new_sp);

void arm_chain_load(paddr_t entry, ulong arg0, ulong arg1, ulong arg2, ulong arg3) __NO_RETURN;

#if 0
static inline uint32_t read_cpsr(void) {
    uint32_t cpsr;

    __asm__ volatile("mrs   %0, cpsr" : "=r" (cpsr));
    return cpsr;
}
#endif

#define CPSR_THUMB       (1<<5)        /* Thumb Mode enable */
#define CPSR_FIQ_MASK    (1<<6)        /* Fast Interrupt mask */
#define CPSR_IRQ_MASK    (1<<7)        /* Interrupt mask */
#define CPSR_ABT_MASK    (1<<8)        /* Asynchronous Abort mask */
#define CPSR_BIG_ENDIAN  (1<<9)        /* arm32: Big Endian Mode */
#define CPSR_DBG_MASK    (1<<9)        /* arm64: Debug Exception mask */
#define CPSR_IT_MASK     (0x0600fc00)  /* Thumb If-Then Mask */
#define CPSR_JAZELLE     (1<<24)       /* Jazelle Mode */

/* Table B1-1 ARM Processor modes */
#define CPSR_MODE_MASK 0x1f
#define CPSR_MODE_USR 0x10
#define CPSR_MODE_FIQ 0x11
#define CPSR_MODE_IRQ 0x12
#define CPSR_MODE_SVC 0x13
#define CPSR_MODE_MON 0x16
#define CPSR_MODE_ABT 0x17
#define CPSR_MODE_HYP 0x1a
#define CPSR_MODE_UND 0x1b
#define CPSR_MODE_SYS 0x1f
#define CPSR_THUMB    (1<<5)
#define CPSR_FIQ_MASK (1<<6)
#define CPSR_IRQ_MASK (1<<7)
#define CPSR_ABORT    (1<<8)
#define CPSR_ENDIAN   (1<<9)
#define CPSR_AIF_MASK (CPSR_FIQ_MASK|CPSR_IRQ_MASK|CPSR_ABORT)

#ifdef IS_64BIT
/* 64 bit modes */
#define CPSR_MODE_BIT  0x10 /* Set iff AArch32 */
#define CPSR_MODE_EL3h 0x0d
#define CPSR_MODE_EL3t 0x0c
#define CPSR_MODE_EL2h 0x09
#define CPSR_MODE_EL2t 0x08
#define CPSR_MODE_EL1h 0x05
#define CPSR_MODE_EL1t 0x04
#define CPSR_MODE_EL0t 0x00
#endif

struct arm_iframe {
#if ARM_WITH_VFP
    uint32_t fpexc;
#endif
    uint32_t usp;
    uint32_t ulr; // 9
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t spsr;
};

struct arm_fault_frame {
    uint32_t hsr;
#if ARM_WITH_VFP
    uint32_t fpexc;
#endif
    uint32_t usp;
    uint32_t ulr; // 17
    uint32_t r[13];
    uint32_t lr;
    uint32_t pc;
    uint32_t spsr;
};

struct arm_mode_regs {
    uint32_t usr_r13, usr_r14;
    uint32_t fiq_r13, fiq_r14;
    uint32_t irq_r13, irq_r14;
    uint32_t svc_r13, svc_r14;
    uint32_t abt_r13, abt_r14;
    uint32_t und_r13, und_r14;
    uint32_t sys_r13, sys_r14;
#ifdef WITH_HYPER_MODE    
    uint32_t hyp_r13, hyp_r14;
#endif    
};

#define psr_mode(psr,m) (((psr) & CPSR_MODE_MASK) == m)
static inline bool psr_mode_is_32bit(const struct arm_fault_frame *regs)
{
#ifndef IS_64BIT
    return true;
#else
    return !!(regs->spsr & CPSR_MODE_BIT);
#endif
}

#define usr_mode(r)     psr_mode((r)->spsr,CPSR_MODE_USR)
#define fiq_mode(r)     psr_mode((r)->spsr,CPSR_MODE_FIQ)
#define irq_mode(r)     psr_mode((r)->spsr,CPSR_MODE_IRQ)
#define svc_mode(r)     psr_mode((r)->spsr,CPSR_MODE_SVC)
#define mon_mode(r)     psr_mode((r)->spsr,CPSR_MODE_MON)
#define abt_mode(r)     psr_mode((r)->spsr,CPSR_MODE_ABT)
#define und_mode(r)     psr_mode((r)->spsr,CPSR_MODE_UND)
#define sys_mode(r)     psr_mode((r)->spsr,CPSR_MODE_SYS)

#ifndef IS_64BIT
#define hyp_mode(r)     psr_mode((r)->spsr,CPSR_MODE_HYP)
#define psr_mode_is_user(r) usr_mode(r)
#else
#define hyp_mode(r)     (psr_mode((r)->spsr,CPSR_MODE_EL2h) || \
                         psr_mode((r)->spsr,CPSR_MODE_EL2t))

/*
 * Trap may have been taken from EL0, which might be in AArch32 usr
 * mode, or in AArch64 mode (CPSR_MODE_EL0t).
 */
#define psr_mode_is_user(r) \
    (psr_mode((r)->spsr,CPSR_MODE_EL0t) || usr_mode(r))
#endif

static inline bool guest_mode(const struct arm_fault_frame *r)
{
	return true;
}


void arm_save_mode_regs(struct arm_mode_regs *regs);

#define GEN_CP_REG_FUNCS(cp, reg, op1, c1, c2, op2) \
static inline __ALWAYS_INLINE uint32_t arm_read_##reg(void) { \
    uint32_t val; \
    __asm__ volatile("mrc " #cp ", " #op1 ", %0, " #c1 ","  #c2 "," #op2 : "=r" (val)); \
    return val; \
} \
\
static inline __ALWAYS_INLINE uint32_t arm_read_##reg##_relaxed(void) { \
    uint32_t val; \
    __asm__("mrc " #cp ", " #op1 ", %0, " #c1 ","  #c2 "," #op2 : "=r" (val)); \
    return val; \
} \
\
static inline __ALWAYS_INLINE void arm_write_##reg(uint32_t val) { \
    __asm__ volatile("mcr " #cp ", " #op1 ", %0, " #c1 ","  #c2 "," #op2 :: "r" (val)); \
    ISB; \
} \
\
static inline __ALWAYS_INLINE void arm_write_##reg##_relaxed(uint32_t val) { \
    __asm__ volatile("mcr " #cp ", " #op1 ", %0, " #c1 ","  #c2 "," #op2 :: "r" (val)); \
}

#define GEN_CP15_REG_FUNCS(reg, op1, c1, c2, op2) \
    GEN_CP_REG_FUNCS(p15, reg, op1, c1, c2, op2)

#define GEN_CP14_REG_FUNCS(reg, op1, c1, c2, op2) \
    GEN_CP_REG_FUNCS(p14, reg, op1, c1, c2, op2)

/* armv6+ control regs */
GEN_CP15_REG_FUNCS(sctlr, 0, c1, c0, 0);
GEN_CP15_REG_FUNCS(actlr, 0, c1, c0, 1);
GEN_CP15_REG_FUNCS(cpacr, 0, c1, c0, 2);

GEN_CP15_REG_FUNCS(ttbr, 0, c2, c0, 0);
GEN_CP15_REG_FUNCS(ttbr0, 0, c2, c0, 0);
GEN_CP15_REG_FUNCS(ttbr1, 0, c2, c0, 1);
GEN_CP15_REG_FUNCS(ttbcr, 0, c2, c0, 2);
GEN_CP15_REG_FUNCS(dacr, 0, c3, c0, 0);
GEN_CP15_REG_FUNCS(dfsr, 0, c5, c0, 0);
GEN_CP15_REG_FUNCS(ifsr, 0, c5, c0, 1);
GEN_CP15_REG_FUNCS(dfar, 0, c6, c0, 0);
GEN_CP15_REG_FUNCS(wfar, 0, c6, c0, 1);
GEN_CP15_REG_FUNCS(ifar, 0, c6, c0, 2);

GEN_CP15_REG_FUNCS(fcseidr, 0, c13, c0, 0);
GEN_CP15_REG_FUNCS(contextidr, 0, c13, c0, 1);
GEN_CP15_REG_FUNCS(tpidrurw, 0, c13, c0, 2);
GEN_CP15_REG_FUNCS(tpidruro, 0, c13, c0, 3);
GEN_CP15_REG_FUNCS(tpidrprw, 0, c13, c0, 4);

/* armv7+ */
GEN_CP15_REG_FUNCS(midr, 0, c0, c0, 0);
GEN_CP15_REG_FUNCS(mpidr, 0, c0, c0, 5);
GEN_CP15_REG_FUNCS(vbar, 0, c12, c0, 0);
GEN_CP15_REG_FUNCS(cbar, 4, c15, c0, 0);

GEN_CP15_REG_FUNCS(ats1cpr, 0, c7, c8, 0);
GEN_CP15_REG_FUNCS(ats1cpw, 0, c7, c8, 1);
GEN_CP15_REG_FUNCS(ats1cur, 0, c7, c8, 2);
GEN_CP15_REG_FUNCS(ats1cuw, 0, c7, c8, 3);
GEN_CP15_REG_FUNCS(ats12nsopr, 0, c7, c8, 4);
GEN_CP15_REG_FUNCS(ats12nsopw, 0, c7, c8, 5);
GEN_CP15_REG_FUNCS(ats12nsour, 0, c7, c8, 6);
GEN_CP15_REG_FUNCS(ats12nsouw, 0, c7, c8, 7);
GEN_CP15_REG_FUNCS(par, 0, c7, c4, 0);

/* Branch predictor invalidate */
GEN_CP15_REG_FUNCS(bpiall, 0, c7, c5, 6);
GEN_CP15_REG_FUNCS(bpimva, 0, c7, c5, 7);
GEN_CP15_REG_FUNCS(bpiallis, 0, c7, c1, 6);

/* tlb registers */
GEN_CP15_REG_FUNCS(tlbiallis, 0, c8, c3, 0);
GEN_CP15_REG_FUNCS(tlbimvais, 0, c8, c3, 1);
GEN_CP15_REG_FUNCS(tlbiasidis, 0, c8, c3, 2);
GEN_CP15_REG_FUNCS(tlbimvaais, 0, c8, c3, 3);
GEN_CP15_REG_FUNCS(itlbiall, 0, c8, c5, 0);
GEN_CP15_REG_FUNCS(itlbimva, 0, c8, c5, 1);
GEN_CP15_REG_FUNCS(itlbiasid, 0, c8, c5, 2);
GEN_CP15_REG_FUNCS(dtlbiall, 0, c8, c6, 0);
GEN_CP15_REG_FUNCS(dtlbimva, 0, c8, c6, 1);
GEN_CP15_REG_FUNCS(dtlbiasid, 0, c8, c6, 2);
GEN_CP15_REG_FUNCS(tlbiall, 0, c8, c7, 0);
GEN_CP15_REG_FUNCS(tlbimva, 0, c8, c7, 1);
GEN_CP15_REG_FUNCS(tlbiasid, 0, c8, c7, 2);
GEN_CP15_REG_FUNCS(tlbimvaa, 0, c8, c7, 3);

GEN_CP15_REG_FUNCS(l2ctlr, 1, c9, c0, 2);
GEN_CP15_REG_FUNCS(l2ectlr, 1, c9, c0, 3);

/* debug registers */
GEN_CP14_REG_FUNCS(dbddidr, 0, c0, c0, 0);
GEN_CP14_REG_FUNCS(dbgdrar, 0, c1, c0, 0);
GEN_CP14_REG_FUNCS(dbgdsar, 0, c2, c0, 0);
GEN_CP14_REG_FUNCS(dbgdscr, 0, c0, c1, 0);
GEN_CP14_REG_FUNCS(dbgdtrtxint, 0, c0, c5, 0);
GEN_CP14_REG_FUNCS(dbgdtrrxint, 0, c0, c5, 0); /* alias to previous */
GEN_CP14_REG_FUNCS(dbgwfar, 0, c0, c6, 0);
GEN_CP14_REG_FUNCS(dbgvcr, 0, c0, c7, 0);
GEN_CP14_REG_FUNCS(dbgecr, 0, c0, c9, 0);
GEN_CP14_REG_FUNCS(dbgdsccr, 0, c0, c10, 0);
GEN_CP14_REG_FUNCS(dbgdsmcr, 0, c0, c11, 0);
GEN_CP14_REG_FUNCS(dbgdtrrxext, 0, c0, c0, 2);
GEN_CP14_REG_FUNCS(dbgdscrext, 0, c0, c2, 2);
GEN_CP14_REG_FUNCS(dbgdtrtxext, 0, c0, c3, 2);
GEN_CP14_REG_FUNCS(dbgdrcr, 0, c0, c4, 2);
GEN_CP14_REG_FUNCS(dbgvr0, 0, c0, c0, 4);
GEN_CP14_REG_FUNCS(dbgvr1, 0, c0, c1, 4);
GEN_CP14_REG_FUNCS(dbgvr2, 0, c0, c2, 4);
GEN_CP14_REG_FUNCS(dbgbcr0, 0, c0, c0, 5);
GEN_CP14_REG_FUNCS(dbgbcr1, 0, c0, c1, 5);
GEN_CP14_REG_FUNCS(dbgbcr2, 0, c0, c2, 5);
GEN_CP14_REG_FUNCS(dbgwvr0, 0, c0, c0, 6);
GEN_CP14_REG_FUNCS(dbgwvr1, 0, c0, c1, 6);
GEN_CP14_REG_FUNCS(dbgwcr0, 0, c0, c0, 7);
GEN_CP14_REG_FUNCS(dbgwcr1, 0, c0, c1, 7);
GEN_CP14_REG_FUNCS(dbgoslar, 0, c1, c0, 4);
GEN_CP14_REG_FUNCS(dbgoslsr, 0, c1, c1, 4);
GEN_CP14_REG_FUNCS(dbgossrr, 0, c1, c2, 4);
GEN_CP14_REG_FUNCS(dbgprcr, 0, c1, c4, 4);
GEN_CP14_REG_FUNCS(dbgprsr, 0, c1, c5, 4);
GEN_CP14_REG_FUNCS(dbgclaimset, 0, c7, c8, 6);
GEN_CP14_REG_FUNCS(dbgclaimclr, 0, c7, c9, 6);
GEN_CP14_REG_FUNCS(dbgauthstatus, 0, c7, c14, 6);
GEN_CP14_REG_FUNCS(dbgdevid, 0, c7, c2, 7);

/* fpu */
void arm_fpu_set_enable(bool enable);
#if ARM_WITH_VFP
void arm_fpu_undefined_instruction(struct arm_iframe *frame);
struct thread;
void arm_fpu_thread_initialize(struct thread *t);
void arm_fpu_thread_swap(struct thread *oldthread, struct thread *newthread);
#endif

__END_CDECLS

#endif

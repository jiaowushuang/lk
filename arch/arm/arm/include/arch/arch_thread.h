/*
 * Copyright (c) 2008-2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#ifndef __ARM_ARCH_THREAD_H
#define __ARM_ARCH_THREAD_H


#include <sys/types.h>
#include <kern/compiler.h>
#include <arch/thread.h>

#ifdef WITH_HYPER_MODE
#include <dev/interrupt/arm_vgic.h>
#endif

__BEGIN_CDECLS

/* Derived Class of 'pcb_ctx_t' - switch needs the temporary context, the user includes 'world' */
/* user stack for c runitme of sp_el0 
 * uintptr_t stack;
 */
/* page table memory entry, such as kernel:
 * vspace == kernel_kvspace
 * uintptr_t vspace;
 */
#ifdef WITH_HYPER_MODE
struct pcb_vgic_ctx {
    uint32_t hcr;
    uint32_t vmcr;
    uint32_t apr;
    virq_t lr[GIC_VCPU_MAX_NUM_LR];
};
#ifdef CONFIG_VTIMER_UPDATE_VOFFSET
struct pcb_vtimer_ctx {
    uint64_t last_pcount;
};
#endif
#endif

struct pcb_user_ctx {
	/* temporary cpu context - for switch context */
	uintptr_t tcpu_sp;
#if ARM_WITH_VFP
	/* has this thread ever used the floating point state? */
	bool fpused;

	uint32_t fpscr;
	uint32_t fpexc;
	double   fpregs[32];
#endif
#ifdef WITH_HYPER_MODE
	struct pcpu_context pcpu;	/* vcpu */
    // vgic
	struct pcb_vgic_ctx vgic;

	// vtimer
	bool vppi_masked; // vtimer irq
#ifdef CONFIG_VTIMER_UPDATE_VOFFSET
	struct pcb_vtimer_ctx vtimer;
#endif
#endif
	struct pcb_ctx pcb;
} __aligned(CACHE_LINE);

typedef struct pcb_user_ctx pcb_user_ctx_t;

struct context_switch_frame {
    vaddr_t r4;
    vaddr_t r5;
    vaddr_t r6;
    vaddr_t r7;
    vaddr_t r8;
    vaddr_t r9;
    vaddr_t r10;
    vaddr_t r11;
    vaddr_t lr;
};

__END_CDECLS

#endif


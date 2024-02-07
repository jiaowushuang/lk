/*
 * Copyright (c) 2008-2012 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#ifndef __ARM_M_ARCH_THREAD_H
#define __ARM_M_ARCH_THREAD_H

#include <stdbool.h>
#include <sys/types.h>
#include <kern/compiler.h>
#include <arch/thread.h>

__BEGIN_CDECLS

/* Derived Class of 'pcb_ctx_t' - switch needs the temporary context, the user includes 'world' */
/* user stack for c runitme of sp_el0 
 * uintptr_t stack;
 */
/* page table memory entry, such as kernel:
 * vspace == kernel_kvspace
 * uintptr_t vspace;
 */
struct pcb_user_ctx {
    vaddr_t tcpu_sp;
    bool was_preempted;

#if ARM_WITH_VFP
    /* has this thread ever used the floating point state? */
    bool fpused;

    /* s16-s31 saved here. s0-s15, fpscr saved on exception frame */
    float fpregs[16];
#endif
	struct pcpu_context pcpu;	
	struct pcb_ctx pcb;
} __aligned(CACHE_LINE);

typedef struct pcb_user_ctx pcb_user_ctx_t;

struct context_switch_frame {
#if  (__CORTEX_M >= 0x03)
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;
    uint32_t lr;
#else
    /* frame format is slightly different due to ordering of push/pops */
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t lr;
#endif
};

__END_CDECLS

#endif


/*
 * Copyright (c) 2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

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

struct fpstate {
    uint64_t    regs[64];
    uint32_t    fpcr;
    uint32_t    fpsr;
    uint        current_cpu;
};

struct pcb_user_ctx {
	/* temporary cpu context - for switch context */
	uintptr_t tcpu_sp;
    struct fpstate fpstate;
	struct pcpu_context pcpu;
#ifdef WITH_HYPER_MODE    
    // vgic
    // vtimer
#endif
	struct pcb_ctx pcb;
} __aligned(CACHE_LINE);

typedef struct pcb_user_ctx pcb_user_ctx_t;

struct context_switch_frame {
    vaddr_t r[14];
};

__END_CDECLS

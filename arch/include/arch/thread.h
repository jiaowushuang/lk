/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

// give the arch code a chance to declare the arch_thread struct

#include <kern/compiler.h>
#include <arch/arch_context.h>
#include <arch/defines.h>

__BEGIN_CDECLS

/* Execution Residency Type 
 * 1. PCPU
 * 2. TCPU
 */
/* Execution Residency Data  
 * 1. global - sysreg .etc
 * 2. temporary - gpreg .etc
 */

/* Execution entity type 
 * 1. kernel - monitor(3),    hypervisor(2), superviser(1),    administrator(0)
 * 2. user -   hypervisor(2), superviser(1), administrator(0), administrator(0)
 * 3. monitor(3) - Transfer stations in different worlds
 */
enum {
	SECURE, NON_SECURE,
	MONITOR_K, HYPERVISOR_K, SUPERVISER_K, ADMINISTRATOR_K, 
	HYPERVISOR_U, SUPERVISER_U, ADMINISTRATOR_U
};

/* Execution handle type */
enum {
	/* KERNEL */
	EHK_IMAGE = 0,
	EHK_EXCEPTION = 2,
	EHK_IRQ = 4,
	EHK_WORKQUEUE = 6,
	EHK_LARGED = 8,
	EHK_CB = 10,
	EHK_HOOK = 12,
	EHK_DISTRIBUTOR = 14,
	/* USER */
	EHU_IMAGE = 1,
	EHU_SIGNAL = 3,
	EHU_THREAD = 5,
	EHU_THREADX = 7,
	EHU_VM = 9,
	EHU_WORKQUEUE = 11,
	EHU_LARGED = 13,
	EHU_CB = 15,
	EHU_HOOK = 17,
	EHU_DISTRIBUTOR = 19,
};

/* 
 * 1. Initial setup environment 
 * (if switching between different worlds and switching between el2/el1/el0 for each world)
 * 2. Save/restore environment for switch .
 */
/* Ops */
struct pcb_ss_ops {
	/* probe ops */
	// void (*setup)(unsigned int, struct pcpu_context *, struct pcb_ctx *);
};
typedef struct pcb_ss_ops pcb_ss_ops_t;
struct pcb_ctx_ops {
	/* switch ops */
	// void (*switch)(struct pcb_ctx *, struct pcb_ctx *);
};
typedef struct pcb_ctx_ops pcb_ctx_ops_t;

/* Base Class */
struct pcb_ctx {
	/* The same class can reuse context and stack and page table
	 * class(8), subclass(8), concurrency&affinity(16+[32]), max concurrency == core number 
	 */
	/* The executing entity is PROCESS, PER PROCESS, or NEURO, PERCPU 
	 * So, context == &kernel_percpu_data[] OR context == &process_pcpu 
	 * (Assuming a process can only run on one core, So,
	 * If there are multiple PROCESS on a core, a fixed average workload
	 * can be set in advance for each PROCESS, for high-volume task, 
	 * "distribution-induction" algorithm can be used)
	 * stack == &kernel_stacks[] OR stack == &process_pcpu_stack
	 */
	/* class         		context[0]                      context[1]
	 * kernel			pcpu data			NULL	
	 * user|kernel-thread		user-data			vcpu(hyp)|NULL
	 */ 	 
	uintptr_t type;

	/* record the process configuration 
	 * 0: secure world(k)
	 * 1: non-secure world(k)
	 * 2: monitor(k) x 
	 * 3: hypervisor(k) x
	 * 4: superviser(k) x
	 * 5: administrator(k) x
	 * 6: hypervisor(u)
	 * 7: superviser(u)
	 * 8: administrator(u)
	 */
	unsigned int flags;	
	/* entrypoint/function/image PC, such as:
	 * 0. _start[] - EET class (kernel/user), subclass image
	 * 1. exception/irq handler - EET class kernel entrypoint, subclass exception/irq
	 * 2. signal handler - EET class user entrypoint, subclass signal
	 * 3. thread/threadX handler - EET class (kernel/user), subclass thread/threadX
	 * 4. vm handler - EET class user entrypoint, subclass vm
	 * 5. workqueue handler - EET class kernel entrypoint, subclass workqueue
	 * 6. larged handler - EET class kernel entrypoint, subclass larged
	 * 7. cb/hook/Distributor - EET class kernel entrypoint, subclass cb/hook/Distributor
	 */
	uintptr_t handler;
	/* When performing entity switching, the "passive principle" is adopted for saving and
	 * restoring the context, which is based on the context of the person being switched
	 */
	pcb_ctx_ops_t *ops;	
} __aligned(CACHE_LINE);
typedef struct pcb_ctx pcb_ctx_t;

struct thread;

struct pcb_ctx;
#include <arch/arch_thread.h>

void arch_thread_initialize(struct thread *);
void arch_context_switch(struct thread *oldthread, struct thread *newthread);
void arch_context_switch_pls(struct thread *fathread, void *oldthx, void *newthx);

__END_CDECLS

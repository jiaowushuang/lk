/*
 * Copyright (c) 2014-2021, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#define PCPU_KDATA_WORLD_NUM		2
#define PCPU_KDATA_CRASH_SIZE		64 /* need enough space in crash buffer to save 8 registers */

#ifndef __ASSEMBLER__

#include <assert.h>
#include <cassert.h>
#include <arch/defines.h>
#include <kern/compiler.h>


__BEGIN_CDECLS

/*******************************************************************************
 * Function & variable prototypes
 ******************************************************************************/

/*******************************************************************************
 * Cache of frequently used per-cpu data:
 *   Pointers to non-secure, realm, and secure security state contexts
 *   Address of the crash stack
 * It is aligned to the cache line boundary to allow efficient concurrent
 * manipulation of these pointers on different cpus
 *
 * The data structure and the _cpu_data accessors should not be used directly
 * by components that have per-cpu members. The member access macros should be
 * used for this.
 ******************************************************************************/
struct pcpu_kdata;
typedef struct pcpu_kdata pcpu_kdata_t;

/* MPcore - pcpu snapshot - kernel data is THIS!!! */
struct pcpu_kdata {
	struct world *world[PCPU_KDATA_WORLD_NUM]; /* <current core> world - pcpu context - for setup context */
	uintptr_t stack; /* <current core> stack */
	uintptr_t user; /* <current core> user process==current_thread */
	uintptr_t ops; 	/* cpu special opeator */
#if CRASH_REPORTING
	u_register_t crash[PCPU_KDATA_CRASH_SIZE >> 3]; /* cpu carsh */
#endif
#if PLAT_CPU_DATA_SIZE
	uint8_t plat_data[PLAT_CPU_DATA_SIZE]; /* platform defined data */
#endif	
	// TODO /* exception handler data */
} __aligned(CACHE_LINE); 

/* switch context 
 * 1. user+k-sync(user)->user+k-sync(user), kernel, user-k - both context and stack
 * 2. user-k->user-k, user+k-sync(user), kernel - both context and stack
 * 
 * 1. user-k - both context and stack
 * 2. kernel - top - no context, only stack
 * 3. user - no context, only stack; k-sync(user) - both context and stack
 * 4. world ~ world, two worlds can be in same core OR differnet core
 * 
 */

/* SP_EL0 - process func C stack <-> handler(user+k-sync(user), kernel, user-k)
 * SP_ELX - stack frame ptr <-> process(user-k, k-sync(user))
 */

/* OR WE!!!!
 * SP_ELX: stack frame ptr + process func C stack(kernel<嵌套式>, user-k<组合式>, k-sync(user)<组合式>)
 * SP_EL0: stack frame ptr + process func C stack(user)
 */

/* WE!!!
 * kernel<嵌套式>
 * 内核栈即是percpu设置的栈（可以设计为IDLE进程的栈），包含了引导函数栈（在函数末尾会调度，所以这部分栈不会被释放，除非手动改动sp覆盖释放）
 * 内核栈还会用来保存（current ELX）异常/中断所打断的某个线程的 ”所有通用寄存器+异常寄存器“ 以及异常句柄，在异常/中断句柄函数返回时释放
 * 如果在异常句柄中包含resched的话，这部分内存会持续到下次调度到该线程，才会继续执行异常/中断句柄函数，并在返回时释放这块内存
 * 在resched中会需要将callee-saved栈帧保存在内核tcb stack frame，一直持续到下次调度到该线程
 * 所以异常函数在允许执行resched的话，需要考虑最大线程数目来设置内核栈最大容量，同时一般不允许嵌套调用
 * 【IDLE不用新的栈，复用percpu栈，可以考虑销毁引导函数栈，即设置percpu stack 为idle stack】
 * 
 * user-k<组合式>
 * 内核任务栈是内核tcb所持有的栈，每一个内核任务都会有内核tcb
 * 这部分栈分为两个部分，是c func runtime(异常句柄)和stack frame(用来保存异常/中断所打断的某个线程的 ”所有通用寄存器+异常寄存器“)
 * 在（current ELX）异常/中断句柄函数返回时释放; 如果在异常句柄中包含resched的话，
 * 这部分内存会持续到下次调度到该线程，才会继续执行异常/中断句柄函数，并在返回时释放这块内存
 * 在resched中会需要将callee-saved栈帧保存在内核tcb stack frame，一直持续到下次调度到该线程
 * 【SEL4将异常和callee-saved栈帧合并在一起，这样线程切换的函数被省略，取而代之的是进入异常时，
 * 当前线程的寄存器文件保存在callee-saved栈帧，当然会大于callee-saved，
 * 因为还需要保存x0-x17以及elr_elx,spsr_elx,sp_el0，然后在异常返回的时候切换到新的线程；
 * 这样的坏处是无法在执行内核异常中间被打断，执行切换到新的线程】
 * 
 * k-sync(user)<组合式>
 * 内核栈（用户）是内核tcb所持有的栈，每一个用户任务都会有内核tcb
 * 这部分栈分为两个部分，是c func runtime(异常句柄)和stack frame(用来保存异常/中断所打断的某个线程的 ”所有通用寄存器+异常寄存器“)
 * 在（lower ELX）异常/中断句柄函数返回时释放; 如果在异常句柄中包含resched的话，
 * 这部分内存会持续到下次调度到该线程，才会继续执行异常/中断句柄函数，并在返回时释放这块内存
 * 在resched中会需要将callee-saved栈帧保存在内核tcb stack frame，一直持续到下次调度到该线程
 * 关键如果在执行内核函数的过程中，发生（current ELX）异常/中断，这部分内存也会算在该栈上
 * 【设置sp为stack frame地址，而不是stack_top，除非不希望在异常执行时切换到新的线程】
 * 因为在切换上下文时，采用的是save/restore 固定sp，所以需要预先分配tcpu_context_t
 * 
 * user
 * 用户栈一般情况下仅包含c func runtime
 * 
 */

/* Here, it is necessary to design a memory management function specifically for 
 * kernel objects to ensure that kernel objects of the same type are grouped into
 * one or consecutive regions as much as possible
 */
extern pcpu_kdata_t percpu_data[SMP_MAX_CPUS];

#if TODO
/* MMU */
extern pgde_t kernel_pgd[];
extern pude_t kernel_pgu[];
extern pde_t kernel_pd[];
extern pte_t kernel_pt[];
extern vspace_t kernel_kvspace[];
extern asid_pool_t kernel_kasid[]; /* superviser */
#ifdef WITH_HYPER_MODE
extern asid_pool_t kernel_khwasid[]; /* hypervisor */
#endif
/* SMMU */
extern spte_t kernel_spt[];
extern asid_pool_t kernel_sasid[];
#endif

#ifdef __KERNEL_64__
#include <arch/arch_helpers.h>

void init_cpu_data_ptr(void);
/* Return the cpu_data structure for the special CPU. */
struct pcpu_kdata *_cpu_data_by_index(uint32_t cpu_index);

/* Return the cpu_data structure for the current CPU. */
static inline struct pcpu_kdata *_cpu_data(void)
{
#ifdef WITH_MONITOR_MODE
	return (struct pcpu_kdata *)read_tpidr_el3();
#elif WITH_HYPER_MODE
	return (struct pcpu_kdata *)read_tpidr_el2();
#elif WITH_SUPER_MODE
	return (struct pcpu_kdata *)read_tpidr_el1();
#else
	#error("no support kernel type");
#endif
}
#else
struct pcpu_kdata *_cpu_data_by_index(uint32_t cpu_index);
struct pcpu_kdata *_cpu_data(void);
#endif

/**************************************************************************
 * APIs for initialising and accessing per-cpu data
 *************************************************************************/
#define get_cpu_data(_m)		   _cpu_data()->_m
#define set_cpu_data(_m, _v)		   _cpu_data()->_m = (_v)
#define get_cpu_data_by_index(_ix, _m)	   _cpu_data_by_index(_ix)->_m
#define set_cpu_data_by_index(_ix, _m, _v) _cpu_data_by_index(_ix)->_m = (_v)
/* ((cpu_data_t *)0)->_m is a dummy to get the sizeof the struct member _m */

#define flush_cpu_data(_m)	   arch_clean_invalidate_cache_range((uintptr_t)	  	\
						&(_cpu_data()->_m), 				\
						sizeof(((pcpu_kdata_t *)0)->_m))
#define inv_cpu_data(_m)	   arch_invalidate_cache_range((uintptr_t)	  	  	\
						&(_cpu_data()->_m), 				\
						sizeof(((pcpu_kdata_t *)0)->_m))
#define flush_cpu_data_by_index(_ix, _m)							\
				   arch_clean_invalidate_cache_range((uintptr_t)	  	\
					 &(_cpu_data_by_index(_ix)->_m), 			\
						sizeof(((pcpu_kdata_t *)0)->_m))

__END_CDECLS

#endif /* __ASSEMBLER__ */


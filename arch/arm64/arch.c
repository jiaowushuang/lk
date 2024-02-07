/*
 * Copyright (c) 2014-2016 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <kern/debug.h>
#include <stdlib.h>
#include <arch.h>
#include <arch/atomic.h>
#include <arch/ops.h>
#include <arch/arm64.h>
#include <arch/arm64/mmu.h>
#include <arch/mp.h>
#include <kernel/thread.h>
#include <kern/init.h>
#include <kern/main.h>
#include <platform.h>
#include <kern/trace.h>

#define LOCAL_TRACE 0

#if WITH_SMP
/* smp boot lock */
static spin_lock_t arm_boot_cpu_lock = 1;
static volatile int secondaries_to_init = 0;
#endif

static void arm64_cpu_early_init(void) {
    /* set the vector base */
    ARM64_WRITE_SYSREG(VBAR_EL1, (uint64_t)&arm64_exception_base);

    /* switch to EL1 */
    unsigned int current_el = ARM64_READ_SYSREG(CURRENTEL) >> 2;
    if (current_el > 1) {
        arm64_el3_to_el1();
    }

    arch_enable_fiqs();
}

#ifdef ARCH_HAS_MPU
void arch_mpu_init(void) {
    z_arm64_mm_init(true);
}
#endif

void arch_early_init(void) {
    arm64_cpu_early_init();
    platform_init_mmu_mappings();
#ifdef ARCH_HAS_MPU
    arch_mpu_init();
#endif      
}

void arch_stacktrace(uint64_t fp, uint64_t pc)
{
    struct arm64_stackframe frame;

    if (!fp) {
        frame.fp = (uint64_t)__builtin_frame_address(0);
        frame.pc = (uint64_t)arch_stacktrace;
    } else {
        frame.fp = fp;
        frame.pc = pc;
    }

    printf("stack trace:\n");
    while (frame.fp) {
        printf("0x%llx\n", frame.pc);

        /* Stack frame pointer should be 16 bytes aligned */
        if (frame.fp & 0xF)
            break;

        frame.pc = *((uint64_t *)(frame.fp + 8));
        frame.fp = *((uint64_t *)frame.fp);
    }
}

void arch_init(void) {  
#if WITH_SMP
    arch_mp_init_percpu();

    LTRACEF("midr_el1 0x%llx\n", ARM64_READ_SYSREG(midr_el1));

    secondaries_to_init = SMP_MAX_CPUS - 1; /* TODO: get count from somewhere else, or add cpus as they boot */

    init_secondary_cpus(secondaries_to_init);

    LTRACEF("releasing %d secondary cpus\n", secondaries_to_init);

    /* release the secondary cpus */
    spin_unlock(&arm_boot_cpu_lock);

    /* flush the release of the lock, since the secondary cpus are running without cache on */
    arch_clean_cache_range((addr_t)&arm_boot_cpu_lock, sizeof(arm_boot_cpu_lock));
#endif
}

void arch_quiesce(void) {
}

void arch_idle(void) {
    __asm__ volatile("wfi");
}

void arch_chain_load(void *entry, ulong arg0, ulong arg1, ulong arg2, ulong arg3) {
    PANIC_UNIMPLEMENTED;
}

#if 0
/* switch to user mode, set the user stack pointer to user_stack_top, put the svc stack pointer to the top of the kernel stack */
void arch_enter_uspace(vaddr_t entry_point, vaddr_t user_stack_top) {
    DEBUG_ASSERT(IS_ALIGNED(user_stack_top, 16));

    thread_t *ct = get_current_thread();

    vaddr_t kernel_stack_top = (uintptr_t)ct->stack + ct->stack_size;
    kernel_stack_top = ROUNDDOWN(kernel_stack_top, 16);
    /* 如果使用kernel_stack_top直接作为sp-kstack的话，是不考虑在sv时执行线程切换的情况下 */
    vaddr_t kernel_stack_frame = kernel_stack_top - sizeof(tcpu_context_t);

    /* set up a default spsr to get into 64bit user space:
     * zeroed NZCV
     * no SS, no IL, no D
     * all interrupts enabled
     * mode 0: EL0t
     */
    uint32_t spsr = 0;

    arch_disable_ints();

    asm volatile(
        "mov    sp, %[kstack];"
        "msr    sp_el0, %[ustack];"
        "msr    elr_el1, %[entry];"
        "msr    spsr_el1, %[spsr];"
        "eret;"
        :
        : [ustack]"r"(user_stack_top),
        [kstack]"r"(kernel_stack_frame),
        [entry]"r"(entry_point),
        [spsr]"r"(spsr)
        : "memory");
    __UNREACHABLE;
}
#endif

void arch_enter_uspace(vaddr_t entry_point, vaddr_t user_stack_top, vaddr_t kernel_stack_top, vaddr_t arg0) {
#ifdef USER_32BIT
	bool is_32bit_uspace = true;
    DEBUG_ASSERT(IS_ALIGNED(user_stack_top, 8));
#else
	bool is_32bit_uspace = false;
    DEBUG_ASSERT(IS_ALIGNED(user_stack_top, 16));
#endif

    /* set up a default spsr to get into 64bit user space:
     * zeroed NZCV
     * no SS, no IL, no D
     * all interrupts enabled
     * mode 0: EL0t
     */
	uint32_t spsr = is_32bit_uspace ? 0x10 : 0;

	arch_disable_ints();

	__asm__ volatile(
		"mov	x0, %[arg0]\n"	
		"mov	x13, %[stack]\n" /* AArch32 SP_usr */
		"msr    spsel, #1\n"     /* Switch to EL1h before setting a user-space sp */
		"mov	sp, %[kstack]\n" /* Exp sp */
		"msr    sp_el0, %[stack]\n" /* AArch64 SP_usr */
		"msr	spsr_el1, %[spsr]\n" /* Mode = AArch32/AArch64 User */
		"msr	elr_el1, %[entry]\n"
		"mov    x1, #0\n"
		"mov    x2, #0\n"
		"mov    x3, #0\n"
		"mov    x4, #0\n"
		"mov    x5, #0\n"
		"mov    x6, #0\n"
		"mov    x7, #0\n"
		"mov    x8, #0\n"
		"mov    x9, #0\n"
		"mov    x10, #0\n"
		"mov    x11, #0\n"
		"mov    x12, #0\n"
		"mov    x14, #0\n"     /* AArch32 LR_usr */
		"mov    x15, #0\n"
		"mov    x16, #0\n"
		"mov    x17, #0\n"
		"mov    x19, #0\n"
		"mov    x20, #0\n"
		"mov    x21, #0\n"
		"mov    x22, #0\n"
		"mov    x23, #0\n"
		"mov    x24, #0\n"
		"mov    x25, #0\n"
		"mov    x26, #0\n"
		"mov    x27, #0\n"
		"mov    x28, #0\n"
		"mov    x29, #0\n"
		"mov    x30, #0\n"     /* AArch64 LR_usr */			
		"eret\n"
		:
		: [arg0]"r"(arg0), [stack]"r" (user_stack_top), [entry]"r" (entry_point), [spsr]"r" (spsr), [kstack]"r" (kernel_stack_top)
		: "x0", "x1", "memory"
	);
	__UNREACHABLE;

}


#if WITH_SMP
/* called from assembly */
void arm64_secondary_entry(ulong);
void arm64_secondary_entry(ulong asm_cpu_num) {
    uint cpu = arch_curr_cpu_num();
    if (cpu != asm_cpu_num)
        return;

#ifdef ARCH_HAS_MPU
    z_arm64_mm_init(false);
#endif

    arm64_cpu_early_init();

    spin_lock(&arm_boot_cpu_lock);
    spin_unlock(&arm_boot_cpu_lock);

    /* run early secondary cpu init routines up to the threading level */
    bootstrap_init_level(INIT_FLAG_SECONDARY_CPUS, INIT_LEVEL_EARLIEST, INIT_LEVEL_THREADING - 1);

    arch_mp_init_percpu();

    LTRACEF("cpu num %d\n", cpu);

    /* we're done, tell the main cpu we're up */
    atomic_add(&secondaries_to_init, -1);
    __asm__ volatile("sev");

    secondary_cpu_entry();
}
#endif

void arch_set_user_tls(vaddr_t tls_ptr)
{
    /*
     * Note arm32 user space uses the ro TLS register and arm64 uses rw.
     * This matches existing ABIs.
     */
#ifdef USER_32BIT
    /* Lower bits of tpidrro_el0 aliased with arm32 tpidruro. */
    __asm__ volatile("msr tpidrro_el0, %0" :: "r" (tls_ptr));
#else
    /* Can also set from user space. Implemented here for uniformity. */
    __asm__ volatile("msr tpidr_el0, %0" :: "r" (tls_ptr));
#endif
}
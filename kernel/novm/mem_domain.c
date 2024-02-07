/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(ARCH_HAS_MPU)


#include <assert.h>
#include <kernel/spinlock.h>
#include <kern/err.h>
#include <kern/init.h>
#include <kern/trace.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <kernel/thread.h>
#include <kernel/mem_domain.h>

#define LOCAL_TRACE 0

spin_lock_t z_mem_domain_lock;
static uint8_t max_partitions;

#ifdef WITH_HYPER_MODE
static uint8_t thread_vmids; /* 0: invalid ; 1-255: valid */
#endif

struct k_mem_domain k_mem_domain_default;

struct k_mem_partition k_mem_partition_default[KERNEL_MAX_REGIONS];




static bool check_add_partition(struct k_mem_domain *domain,
				struct k_mem_partition *part)
{

	int i;
	uintptr_t pstart, pend, dstart, dend;

	if (part == NULL) {
		LTRACEF("NULL k_mem_partition provided");
		return false;
	}

#ifdef CONFIG_EXECUTE_XOR_WRITE
	/* Arches where execution cannot be disabled should always return
	 * false to this check
	 */
	if (K_MEM_PARTITION_IS_EXECUTABLE(part->attr) &&
	    K_MEM_PARTITION_IS_WRITABLE(part->attr)) {
		LTRACEF("partition is writable and executable <start %lx>",
			part->start);
		return false;
	}
#endif

	if (part->size == 0U) {
		LTRACEF("zero sized partition at %p with base 0x%lx",
			part, part->start);
		return false;
	}

	pstart = part->start;
	pend = part->start + part->size;

	if (pend <= pstart) {
		LTRACEF("invalid partition %p, wraparound detected. base 0x%lx size %zu",
			part, part->start, part->size);
		return false;
	}

	/* Check that this partition doesn't overlap any existing ones already
	 * in the domain
	 */
	for (i = 0; i < domain->num_partitions; i++) {
		struct k_mem_partition *dpart = &domain->partitions[i];

		if (dpart->size == 0U) {
			/* Unused slot */
			continue;
		}

		dstart = dpart->start;
		dend = dstart + dpart->size;

		if (pend > dstart && dend > pstart) {
			LTRACEF("partition %p base %lx (size %zu) overlaps existing base %lx (size %zu)",
				part, part->start, part->size,
				dpart->start, dpart->size);
			return false;
		}
	}

	return true;
}

int k_mem_domain_init(struct k_mem_domain *domain, uint8_t num_parts,
		      struct k_mem_partition *parts[])
{
	int ret = 0;

	if (domain == NULL) {
		ret = -EINVAL;
		goto out;
	}

	if (!(num_parts == 0U || parts != NULL)) {
		LTRACEF("parts array is NULL and num_parts is nonzero");
		ret = -EINVAL;
		goto out;
	}

	if (!(num_parts <= max_partitions)) {
		LTRACEF("num_parts of %d exceeds maximum allowable partitions (%d)",
			num_parts, max_partitions);
		ret = -EINVAL;
		goto out;
	}

    	spin_lock_saved_state_t state;
    	spin_lock_irqsave(&z_mem_domain_lock, state);

	domain->num_partitions = 0U;
	(void)memset(domain->partitions, 0, sizeof(domain->partitions));
	list_initialize(&domain->mem_domain_q);

#ifdef CONFIG_ARCH_MEM_DOMAIN_DATA
	ret = arch_mem_domain_init(domain);

	if (ret != 0) {
		LTRACEF("architecture-specific initialization failed for domain %p with %d",
			domain, ret);
		ret = -ENOMEM;
		goto unlock_out;
	}
#endif
	if (num_parts != 0U) {
		uint32_t i;

		for (i = 0U; i < num_parts; i++) {
			if (!check_add_partition(domain, parts[i])) {
				LTRACEF("invalid partition index %d (%p)",
					i, parts[i]);
				ret = -EINVAL;
				goto unlock_out;
			}

			domain->partitions[i] = *parts[i];
			domain->num_partitions++;
#ifdef CONFIG_ARCH_MEM_DOMAIN_SYNCHRONOUS_API
			int ret2 = arch_mem_domain_partition_add(domain, i);

			ARG_UNUSED(ret2);
			if (ret2 != 0) {
				ret = ret2;
			}
#endif
		}
	}

unlock_out:
	spin_unlock_irqrestore(&z_mem_domain_lock, state);
out:
	return ret;
}

int k_mem_domain_add_partition(struct k_mem_domain *domain,
			       struct k_mem_partition *part)
{
	int p_idx;
	int ret = 0;

	if (domain == NULL) {
		ret = -EINVAL;
		goto out;
	}

	if (!check_add_partition(domain, part)) {
		LTRACEF("invalid partition %p", part);
		ret = -EINVAL;
		goto out;
	}
    	spin_lock_saved_state_t state;
    	spin_lock_irqsave(&z_mem_domain_lock, state);

	for (p_idx = 0; p_idx < max_partitions; p_idx++) {
		/* A zero-sized partition denotes it's a free partition */
		if (domain->partitions[p_idx].size == 0U) {
			break;
		}
	}

	if (!(p_idx < max_partitions)) {
		LTRACEF("no free partition slots available");
		ret = -ENOSPC;
		goto unlock_out;
	}

	LTRACEF("add partition base %lx size %zu to domain %p\n",
		part->start, part->size, domain);

	domain->partitions[p_idx].start = part->start;
	domain->partitions[p_idx].size = part->size;
	domain->partitions[p_idx].attr = part->attr;

	domain->num_partitions++;

#ifdef CONFIG_ARCH_MEM_DOMAIN_SYNCHRONOUS_API
	ret = arch_mem_domain_partition_add(domain, p_idx);
#endif

unlock_out:
	spin_unlock_irqrestore(&z_mem_domain_lock, state);

out:
	return ret;
}

int k_mem_domain_remove_partition(struct k_mem_domain *domain,
				  struct k_mem_partition *part)
{
	int p_idx;
	int ret = 0;

	if ((domain == NULL) || (part == NULL)) {
		ret = -EINVAL;
		goto out;
	}

    	spin_lock_saved_state_t state;
    	spin_lock_irqsave(&z_mem_domain_lock, state);

	/* find a partition that matches the given start and size */
	for (p_idx = 0; p_idx < max_partitions; p_idx++) {
		if (domain->partitions[p_idx].start == part->start &&
		    domain->partitions[p_idx].size == part->size) {
			break;
		}
	}

	if (!(p_idx < max_partitions)) {
		LTRACEF("no matching partition found");
		ret = -ENOENT;
		goto unlock_out;
	}

	LTRACEF("remove partition base %lx size %zu from domain %p\n",
		part->start, part->size, domain);

#ifdef CONFIG_ARCH_MEM_DOMAIN_SYNCHRONOUS_API
	ret = arch_mem_domain_partition_remove(domain, p_idx);
#endif

	/* A zero-sized partition denotes it's a free partition */
	domain->partitions[p_idx].size = 0U;

	domain->num_partitions--;

unlock_out:
	spin_unlock_irqrestore(&z_mem_domain_lock, state);

out:
	return ret;
}

static int add_thread_locked(struct k_mem_domain *domain,
			     struct thread* thread)
{
	int ret = 0;

	DEBUG_ASSERT(domain != NULL);
	DEBUG_ASSERT(thread != NULL);

	LTRACEF("add thread %p to domain %p\n", thread, domain);
	list_add_tail(&domain->mem_domain_q, &thread->mem_domain_info.mem_domain_q_node);
	thread->mem_domain_info.mem_domain = domain;

#ifdef WITH_HYPER_MODE
	thread->mem_domain_info.vmid = (thread_vmids != 255) ? ++thread_vmids : 1;
#endif

#ifdef CONFIG_ARCH_MEM_DOMAIN_SYNCHRONOUS_API
	ret = arch_mem_domain_thread_add(thread);
#endif

	return ret;
}

static int remove_thread_locked(struct thread *thread)
{
	int ret = 0;

	DEBUG_ASSERT(thread != NULL);
	LTRACEF("remove thread %p from memory domain %p\n",
		thread, thread->mem_domain_info.mem_domain);
	list_delete(&thread->mem_domain_info.mem_domain_q_node);

#ifdef WITH_HYPER_MODE
	thread->mem_domain_info.vmid = 0;
#endif
#ifdef CONFIG_ARCH_MEM_DOMAIN_SYNCHRONOUS_API
	ret = arch_mem_domain_thread_remove(thread);
#endif

	return ret;
}

/* Called from thread object initialization */
void z_mem_domain_init_thread(struct thread *thread)
{
	int ret;
    	spin_lock_saved_state_t state;
	struct thread *pthread;

    	spin_lock_irqsave(&z_mem_domain_lock, state);

	pthread = get_current_thread();

	/* New threads inherit memory domain configuration from parent */
	ret = add_thread_locked(pthread->mem_domain_info.mem_domain, thread);
	DEBUG_ASSERT(ret == 0);
	ARG_UNUSED(ret);

	spin_unlock_irqrestore(&z_mem_domain_lock, state);
}

/* Called when thread aborts during teardown tasks. sched_spinlock is held */
void z_mem_domain_exit_thread(struct thread *thread)
{
	int ret;

    	spin_lock_saved_state_t state;
    	spin_lock_irqsave(&z_mem_domain_lock, state);

	ret = remove_thread_locked(thread);
	DEBUG_ASSERT(ret == 0);
	ARG_UNUSED(ret);

	spin_unlock_irqrestore(&z_mem_domain_lock, state);
}

int k_mem_domain_add_thread(struct k_mem_domain *domain, struct thread* thread)
{
	int ret = 0;
    	spin_lock_saved_state_t state;

    	spin_lock_irqsave(&z_mem_domain_lock, state);

	if (thread->mem_domain_info.mem_domain != domain) {
		ret = remove_thread_locked(thread);

		if (ret == 0) {
			ret = add_thread_locked(domain, thread);
		}
	}
	spin_unlock_irqrestore(&z_mem_domain_lock, state);

	return ret;
}

static void init_kern_mem_domain(void)
{
	uint32_t pid;
	int ret;
	for (pid = 0; pid < mpu_config.num_regions; pid++) {
		k_mem_partition_default[pid].start = mpu_config.mpu_regions[pid].base;		
		k_mem_partition_default[pid].size = mpu_config.mpu_regions[pid].size;
		REGION_ATTR_TO_PARTITION_ATTR(&k_mem_partition_default[pid].attr,
						&mpu_config.mpu_regions[pid].attr);
		ret = k_mem_domain_add_partition(&k_mem_domain_default,
					 &k_mem_partition_default[pid]);
		DEBUG_ASSERT_MSG(ret == 0, "failed to add default kern mem partition");
		printf("pid %d, num %d\n", pid, mpu_config.num_regions);
	}
}

static void init_mem_domain_module(uint level)
{
	int ret;

	ARG_UNUSED(ret);
	ARG_UNUSED(level);

	max_partitions = arch_mem_domain_max_partitions_get();
	/*
	 * max_partitions must be less than or equal to
	 * CONFIG_MAX_DOMAIN_PARTITIONS, or would encounter array index
	 * out of bounds error.
	 */
	DEBUG_ASSERT_MSG(max_partitions <= CONFIG_MAX_DOMAIN_PARTITIONS, "out of bounds");

	ret = k_mem_domain_init(&k_mem_domain_default, 0, NULL);
	DEBUG_ASSERT_MSG(ret == 0, "failed to init default mem domain");

#ifdef Z_LIBC_PARTITION_EXISTS
	ret = k_mem_domain_add_partition(&k_mem_domain_default,
					 &z_libc_partition);
	DEBUG_ASSERT_MSG(ret == 0, "failed to add default libc mem partition");

#endif /* Z_LIBC_PARTITION_EXISTS */
	init_kern_mem_domain();
}

INIT_HOOK(init_mem_domain_module, init_mem_domain_module, INIT_LEVEL_VM - 1);

#endif /* ARCH_HAS_MPU */
#include <kernel/thread.h>
#include <kernel/mem_domain.h>

extern void z_arm64_mem_cfg_ipi(void);
extern int configure_dynamic_mpu_regions(struct thread *thread);

int arch_mem_domain_partition_add(struct k_mem_domain *domain, uint32_t partition_id)
{
	ARG_UNUSED(domain);
	ARG_UNUSED(partition_id);

	return 0;
}

int arch_mem_domain_partition_remove(struct k_mem_domain *domain, uint32_t partition_id)
{
	ARG_UNUSED(domain);
	ARG_UNUSED(partition_id);

	return 0;
}

int arch_mem_domain_thread_add(struct thread *thread)
{
	int ret = 0;

	if (thread == get_current_thread()) {
		ret = configure_dynamic_mpu_regions(thread);
	}
#ifdef WITH_SMP
	else {
		/* the thread could be running on another CPU right now */
		z_arm64_mem_cfg_ipi();
	}
#endif

	return ret;
}

int arch_mem_domain_thread_remove(struct thread *thread)
{
	int ret = 0;

	if (thread == get_current_thread()) {
		ret = configure_dynamic_mpu_regions(thread);
	}
#ifdef WITH_SMP
	else {
		/* the thread could be running on another CPU right now */
		z_arm64_mem_cfg_ipi();
	}
#endif

	return ret;
}

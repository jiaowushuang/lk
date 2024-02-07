#include <app.h>
#include <kernel/thread.h>
#include <arch/vcpu.h>
#include <arch.h>
#include <assert.h>
#include <string.h>
#include <kern/trace.h>

#ifdef WITH_KERNEL_VM
#include <kernel/vm.h>
#include <arch/arm/mmu.h>
#endif

#define LOCAL_TRACE 0

extern unsigned int _my_image_start;
extern unsigned int _my_image_end;
extern unsigned int _my_section_start;

#if defined(CONFIG_MULTIPARTITIONING)
// MPU

#define PARTITION_MAX_REGIONS 2 // <= MPU_SUPPORT_MAX_REGIONS

static struct k_mem_domain k_mem_domain_partition[CONFIG_MAX_DOMAIN_PARTITIONS - 1];
static uint32_t domain_partition_nums;

/* XML/DTS CONFIG for kernRTOS */
static struct k_mem_partition k_mem_partitions[PARTITION_MAX_REGIONS];

static status_t guest_mpu_config(struct thread *t)
{
	status_t ret;
	unsigned int *start = &_my_image_start;
	unsigned int *end = &_my_image_end;
	unsigned int *section_start = &_my_section_start;
	char region[32];

	ret = k_mem_domain_init(&k_mem_domain_partition[domain_partition_nums], 
                                        0, NULL);
        if (ret) {
	        LTRACEF("failed to init partition mem domain");
		goto done;
	}

	k_mem_partitions[0].start = KERNEL_BASE; // POINTER_TO_UINT(section_start); fail
						 // KERNEL_BASE for reaching 'guest_entry'
	k_mem_partitions[0].size = GUEST_MEMSIZE;
	k_mem_partitions[0].attr = K_MEM_PARTITION_P_RWX_U_RWX;
	k_mem_partitions[1].start = GUEST_DEVBASE;
	k_mem_partitions[1].size = GUEST_DEVSIZE;
	k_mem_partitions[1].attr = K_MEM_PARTITION_P_RW_U_RW;

	strcpy(region, "img");
	LTRACEF("%s - start at %p, end at %p, size %x, paddr %lx\n", 
			region, start, end, GUEST_MEMSIZE, POINTER_TO_UINT(section_start));

	for (uint32_t pid = 0; pid < PARTITION_MAX_REGIONS; pid++) {
	        ret = k_mem_domain_add_partition(&k_mem_domain_partition[domain_partition_nums],
					 &k_mem_partitions[pid]);
                if (ret) {
	                LTRACEF("failed to add partition mem partition");  
		        goto done;
                }
	}

        k_mem_domain_add_thread(&k_mem_domain_partition[domain_partition_nums], t);
        domain_partition_nums++;
done:
	return ret;
}

#else
// MMU
static status_t guest_mmu_config(struct thread *t)
{
	status_t ret;
	char region[32];
	unsigned int *start = &_my_image_start;
	unsigned int *end = &_my_image_end;
	unsigned int *section_start = &_my_section_start;
	unsigned int size = GUEST_MEMSIZE;
	unsigned int paddr = vaddr_to_paddr(section_start);
	unsigned int vaddr = GUEST_MEMBASE; 
	u_int flags = ARCH_MMU_FLAG_CACHED | ARCH_MMU_FLAG_PERM_USER 
		| ARCH_MMU_FLAG_NS;

	strcpy(region, "img");

	LTRACEF("%s - start at %p, end at %p, size %x, paddr %x, vaddr %x\n", 
		region, start, end, size, paddr, vaddr);

	ret = vmm_alloc_physical(t->aspace, region, size, &vaddr, 0, 
		paddr, VMM_FLAG_VALLOC_SPECIFIC, flags);
        if (ret) {
		LTRACEF("guest config mmu fail %d\n", ret);
		return ret;
	}

    	// idmap for PL1 mode
	vaddr = GUEST_VMEMBASE; // guest kernel virtual start addr

	strcpy(region, "img-idmap");
	LTRACEF("%s - start at %p, end at %p, size %x, paddr %x, vaddr %x\n",
		 region, start, end, size, paddr, vaddr);

	ret = vmm_alloc_physical(t->aspace, region, size, &vaddr, 0, 
		paddr, VMM_FLAG_VALLOC_SPECIFIC, flags);
    	if (ret) {
		LTRACEF("guest config mmu fail %d\n", ret);
		return ret;
	}

	paddr = GUEST_DEVBASE;
	vaddr = GUEST_DEVBASE;
	size = GUEST_DEVSIZE;
	flags = ARCH_MMU_FLAG_UNCACHED_DEVICE | ARCH_MMU_FLAG_PERM_USER | 
		ARCH_MMU_FLAG_NS|ARCH_MMU_FLAG_PERM_NO_EXECUTE;
	
	strcpy(region, "dev");
	LTRACEF("%s - start at %p, end at %p, size %x, paddr %x, vaddr %x\n", 
		region, start, end, size, paddr, vaddr);

	ret = vmm_alloc_physical(t->aspace, region, size, &vaddr, 0, paddr,
		 VMM_FLAG_VALLOC_SPECIFIC, flags);
	if (ret) {
		LTRACEF("guest config mmu fail %d\n", ret);
		return ret;
	}

    	// idmap for PL1 mode
    	vaddr = GUEST_VDEVBASE;
    	size -= PAGE_SIZE;

	strcpy(region, "dev-dimap");
	LTRACEF("%s - start at %p, end at %p, size %x, paddr %x, vaddr %x\n", 
		region, start, end, size, paddr, vaddr);

    	ret = vmm_alloc_physical(t->aspace, region, size, &vaddr, 0, paddr, 
		VMM_FLAG_VALLOC_SPECIFIC, flags);
    	if (ret) {
	    LTRACEF("guest config mmu fail %d\n", ret);
	    return ret;
	}
	return 0;
}
#endif

status_t app_config_thread_entry(struct thread *t) {
	status_t ret;
#if defined(CONFIG_MULTIPARTITIONING)
	ret = guest_mpu_config(t);
#else
	ret = vmm_create_aspace(&t->aspace, t->name, 0);
	if (ret)
		goto done;
	ret = guest_mmu_config(t);
#endif
	if (ret)
		goto done;

	LTRACEF("app config over\n");
	t->flags |= THREAD_FLAG_VM;
	vcpu_init(&t->arch);
	LTRACEF("vcpu init over\n");
done:
	return ret;
}

static void guest_entry(const struct app_descriptor *app, void *args) {
	unsigned int entry_point = GUEST_MEMBASE + GUEST_LOAD_OFFSET;
	LTRACEF("guest enter at %x\n", entry_point);
	/* Invalidate D/I Cache. */
	arch_clean_invalidate_caches();

#ifdef WITH_KERNEL_VM
	/* Invalidate TLB Cache. */
	/* TLB PL2 stage1, stage2, PL1/0 stage1 */
	arm_invalidate_tlb_global();
	arm_invalidate_tlb_asid(0);
	arm_invalidate_tlb2_global();
#endif	
	arch_enter_uspace(entry_point, 0, 0, 0);
}

APP_START(guest_bootloader)
.entry = guest_entry,
APP_END

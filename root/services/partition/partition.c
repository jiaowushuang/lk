#include <app.h>
#include <kernel/thread.h>
#include <assert.h>
#include <string.h>
#include <kern/trace.h>

#define LOCAL_TRACE 0
#define PARTITIOM_DEBUG

#define PARTITION_MAX_REGIONS 2 // <= MPU_SUPPORT_MAX_REGIONS

static struct k_mem_domain k_mem_domain_partition[CONFIG_MAX_DOMAIN_PARTITIONS - 1];
static uint32_t domain_partition_nums;

/* parent thread : bootstrap2
 * bootstrap2 -> minimal { code, sram, device }
 *       Inherited from bootstrap2:
 *       1. kernel server threads(A)
 *       2. partition threads(Bs)
 * 
 * So the partition RTOS is shared with the kernel (service), 
 * and this mechanism can be used to implement the sharing 
 * mechanism of RTOS; However, the disadvantage of this approach
 * is that RTOS can freely access the memory area of the kernel, 
 * which is very dangerous, so it is not recommended to use the 
 * MPU partition inheritance mechanism (similar to the memory 
 * inheritance relationship between parent-child tasks)
 * 
 * WE NEED:
 *      A != B0 !=...Bn (TODO)
*/
    
/* XML/DTS CONFIG for freeRTOS */
static struct k_mem_partition k_mem_partitions[PARTITION_MAX_REGIONS] = {
        [0] = {
                .start = 0,
                .size =  0x40000,
                .attr = K_MEM_PARTITION_P_RX_U_RX,  
        },
        [1] = {
                .start = 0x20000000,
                .size =  0x10000,
                .attr = K_MEM_PARTITION_P_RWX_U_RWX,
        },
};

static status_t partition_mpu_config(struct thread *t)
{
	status_t ret;

	ret = k_mem_domain_init(&k_mem_domain_partition[domain_partition_nums], 
                                        0, NULL);
        if (ret) {
	        LTRACEF("failed to init partition mem domain");
		goto done;
	}

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

extern unsigned int _my_image_start;
extern unsigned int _my_image_data_start, _my_image_data_end;
unsigned int *app_info_header;

status_t app_config_thread_entry(struct thread *t) {
	status_t ret;

	ret = partition_mpu_config(t);
	t->flags |= THREAD_FLAG_PARTITION;
	return ret;
}

static void partition_entry(const struct app_descriptor *app, void *args) {
	unsigned int *src = &_my_image_start;
        unsigned int *dest = &_my_image_data_start;
	size_t size = &_my_image_data_end - &_my_image_data_start;

	LTRACEF("partition image start %p, end %p\n", 
                &_my_image_data_start, &_my_image_data_end);

	memset(dest, 0, size);
	memcpy(dest, src, size);
#ifdef PARTITIOM_DEBUG        
	for (int i = 0; i < 32; i++)
		LTRACEF("%d: %lx\n", i, dest[i]);
#endif
	app_info_header = dest;
	__asm__ volatile("bx %0;" ::"r"(app_info_header[0])); // 0x1, 否则直接bx func会出现异常
}

APP_START(partition_minor)
.entry = partition_entry,
APP_END

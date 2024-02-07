#pragma once

#ifdef ARCH_HAS_MPU
#ifdef ARCH_ARM_HAS_MPU
#include <mach/mpu/arm_mpu.h>
#endif

#ifdef ARCH_NXP_HAS_MPU
#include <mach/mpu/nxp_mpu.h>
#endif

#include <kern/compiler.h>

struct thread;

__BEGIN_CDECLS

struct z_arm_mpu_partition {
	uintptr_t start;
	size_t size;
	k_mem_partition_attr_t attr;
};

#ifdef CONFIG_MULTIPARTITIONING
/**
 * @def K_MEM_PARTITION_DEFINE
 *
 * @brief Statically declare a memory partition
 */
#ifdef _ARCH_MEM_PARTITION_ALIGN_CHECK
#define K_MEM_PARTITION_DEFINE(name, start, size, attr) \
	_ARCH_MEM_PARTITION_ALIGN_CHECK(start, size); \
	struct k_mem_partition name =\
		{ (uintptr_t)start, size, attr}
#else
#define K_MEM_PARTITION_DEFINE(name, start, size, attr) \
	struct k_mem_partition name =\
		{ (uintptr_t)start, size, attr}
#endif /* _ARCH_MEM_PARTITION_ALIGN_CHECK */

/**
 * @brief Memory Partition
 *
 * A memory partition is a region of memory in the linear address space
 * with a specific access policy.
 *
 * The alignment of the starting address, and the alignment of the size
 * value may have varying requirements based on the capabilities of the
 * underlying memory management hardware; arbitrary values are unlikely
 * to work.
 */
struct k_mem_partition {
	/** start address of memory partition */
	uintptr_t start;
	/** size of memory partition */
	size_t size;
	/** attribute of memory partition */
	k_mem_partition_attr_t attr;
};

/**
 * @brief Memory Domain
 *
 * A memory domain is a collection of memory partitions, used to represent
 * a user thread's access policy for the linear address space. A thread
 * may be a member of only one memory domain, but any memory domain may
 * have multiple threads that are members.
 *
 * Supervisor threads may also be a member of a memory domain; this has
 * no implications on their memory access but can be useful as any child
 * threads inherit the memory domain membership of the parent.
 *
 * A user thread belonging to a memory domain with no active partitions
 * will have guaranteed access to its own stack buffer, program text,
 * and read-only data.
 */
struct k_mem_domain {
#ifdef CONFIG_ARCH_MEM_DOMAIN_DATA
	struct arch_mem_domain arch;
#endif /* CONFIG_ARCH_MEM_DOMAIN_DATA */
	/** partitions in the domain */
	struct k_mem_partition partitions[CONFIG_MAX_DOMAIN_PARTITIONS];
	/** Doubly linked list of member threads */
	struct list_node mem_domain_q;
	/** number of active partitions in the domain */
	uint8_t num_partitions;
};

/**
 * Default memory domain
 *
 * All threads are a member of some memory domain, even if running in
 * supervisor mode. Threads belong to this default memory domain if they
 * haven't been added to or inherited membership from some other domain.
 *
 * This memory domain has the z_libc_partition partition for the C library
 * added to it if exists.
 */
extern struct k_mem_domain k_mem_domain_default;
extern struct k_mem_partition k_mem_partition_default[KERNEL_MAX_REGIONS];

#else
/* To support use of IS_ENABLED for the APIs below */
struct k_mem_domain;
struct k_mem_partition;
#endif

/**
 * @brief Initialize a memory domain.
 *
 * Initialize a memory domain with given name and memory partitions.
 *
 * See documentation for k_mem_domain_add_partition() for details about
 * partition constraints.
 *
 * Do not call k_mem_domain_init() on the same memory domain more than once,
 * doing so is undefined behavior.
 *
 * @param domain The memory domain to be initialized.
 * @param num_parts The number of array items of "parts" parameter.
 * @param parts An array of pointers to the memory partitions. Can be NULL
 *              if num_parts is zero.
 *
 * @retval 0 if successful
 * @retval -EINVAL if invalid parameters supplied
 * @retval -ENOMEM if insufficient memory
 */
extern int k_mem_domain_init(struct k_mem_domain *domain, uint8_t num_parts,
			     struct k_mem_partition *parts[]);

/**
 * @brief Add a memory partition into a memory domain.
 *
 * Add a memory partition into a memory domain. Partitions must conform to
 * the following constraints:
 *
 * - Partitions in the same memory domain may not overlap each other.
 * - Partitions must not be defined which expose private kernel
 *   data structures or kernel objects.
 * - The starting address alignment, and the partition size must conform to
 *   the constraints of the underlying memory management hardware, which
 *   varies per architecture.
 * - Memory domain partitions are only intended to control access to memory
 *   from user mode threads.
 * - If CONFIG_EXECUTE_XOR_WRITE is enabled, the partition must not allow
 *   both writes and execution.
 *
 * Violating these constraints may lead to CPU exceptions or undefined
 * behavior.
 *
 * @param domain The memory domain to be added a memory partition.
 * @param part The memory partition to be added
 *
 * @retval 0 if successful
 * @retval -EINVAL if invalid parameters supplied
 * @retval -ENOSPC if no free partition slots available
 */
extern int k_mem_domain_add_partition(struct k_mem_domain *domain,
				      struct k_mem_partition *part);

/**
 * @brief Remove a memory partition from a memory domain.
 *
 * Remove a memory partition from a memory domain.
 *
 * @param domain The memory domain to be removed a memory partition.
 * @param part The memory partition to be removed
 *
 * @retval 0 if successful
 * @retval -EINVAL if invalid parameters supplied
 * @retval -ENOENT if no matching partition found
 */
extern int k_mem_domain_remove_partition(struct k_mem_domain *domain,
					 struct k_mem_partition *part);

/**
 * @brief Add a thread into a memory domain.
 *
 * Add a thread into a memory domain. It will be removed from whatever
 * memory domain it previously belonged to.
 *
 * @param domain The memory domain that the thread is going to be added into.
 * @param thread ID of thread going to be added into the memory domain.
 *
 * @return 0 if successful, fails otherwise.
 */
extern int k_mem_domain_add_thread(struct k_mem_domain *domain,
				   struct thread* thread);

extern void z_mem_domain_init_thread(struct thread *thread);
extern void z_mem_domain_exit_thread(struct thread *thread);


/* z/arm interface */
extern void z_arm_configure_static_mpu_regions(void);
extern void z_arm_configure_dynamic_mpu_regions(struct thread *thread);
extern int z_arm_mpu_init(void);

/* z/arm64 interface */
extern void z_arm64_thread_mem_domains_init(struct thread *thread);
extern void z_arm64_swap_mem_domains(struct thread *thread);
extern void z_arm64_mm_init(bool is_primary_core);
#ifdef CONFIG_ARCH_MEM_DOMAIN_DATA
/**
 *
 * @brief Architecture-specific hook for memory domain initialization
 *
 * Perform any tasks needed to initialize architecture-specific data within
 * the memory domain, such as reserving memory for page tables. All members
 * of the provided memory domain aside from `arch` will be initialized when
 * this is called, but no threads will be a assigned yet.
 *
 * This function may fail if initializing the memory domain requires allocation,
 * such as for page tables.
 *
 * The associated function k_mem_domain_init() documents that making
 * multiple init calls to the same memory domain is undefined behavior,
 * but has no assertions in place to check this. If this matters, it may be
 * desirable to add checks for this in the implementation of this function.
 *
 * @param domain The memory domain to initialize
 * @retval 0 Success
 * @retval -ENOMEM Insufficient memory
 */
extern int arch_mem_domain_init(struct k_mem_domain *domain);
#endif /* CONFIG_ARCH_MEM_DOMAIN_DATA */

#ifdef CONFIG_ARCH_MEM_DOMAIN_SYNCHRONOUS_API
/**
 * @brief Add a thread to a memory domain (arch-specific)
 *
 * Architecture-specific hook to manage internal data structures or hardware
 * state when the provided thread has been added to a memory domain.
 *
 * The thread->mem_domain_info.mem_domain pointer will be set to the domain to
 * be added to before this is called. Implementations may assume that the
 * thread is not already a member of this domain.
 *
 * @param thread Thread which needs to be configured.
 *
 * @retval 0 if successful
 * @retval -EINVAL if invalid parameters supplied
 * @retval -ENOSPC if running out of space in internal structures
 *                    (e.g. translation tables)
 */
extern int arch_mem_domain_thread_add(struct thread *thread);

/**
 * @brief Remove a thread from a memory domain (arch-specific)
 *
 * Architecture-specific hook to manage internal data structures or hardware
 * state when the provided thread has been removed from a memory domain.
 *
 * The thread's memory domain pointer will be the domain that the thread
 * is being removed from.
 *
 * @param thread Thread being removed from its memory domain
 *
 * @retval 0 if successful
 * @retval -EINVAL if invalid parameters supplied
 */
extern int arch_mem_domain_thread_remove(struct thread *thread);

/**
 * @brief Remove a partition from the memory domain (arch-specific)
 *
 * Architecture-specific hook to manage internal data structures or hardware
 * state when a memory domain has had a partition removed.
 *
 * The partition index data, and the number of partitions configured, are not
 * respectively cleared and decremented in the domain until after this function
 * runs.
 *
 * @param domain The memory domain structure
 * @param partition_id The partition index that needs to be deleted
 *
 * @retval 0 if successful
 * @retval -EINVAL if invalid parameters supplied
 * @retval -ENOENT if no matching partition found
 */
extern int arch_mem_domain_partition_remove(struct k_mem_domain *domain,
				     uint32_t partition_id);

/**
 * @brief Add a partition to the memory domain
 *
 * Architecture-specific hook to manage internal data structures or hardware
 * state when a memory domain has a partition added.
 *
 * @param domain The memory domain structure
 * @param partition_id The partition that needs to be added
 *
 * @retval 0 if successful
 * @retval -EINVAL if invalid parameters supplied
 */
extern int arch_mem_domain_partition_add(struct k_mem_domain *domain,
				  uint32_t partition_id);
#endif /* CONFIG_ARCH_MEM_DOMAIN_SYNCHRONOUS_API */

__END_CDECLS

#endif
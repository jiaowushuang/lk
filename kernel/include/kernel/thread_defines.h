#pragma once

#include <arch/thread.h>
#include <kernel/wait.h>
#include <kern/debug.h>
#include <kern/list.h>
#include <sys/types.h>

#if WITH_KERNEL_VM
/* forward declaration */
typedef struct vmm_aspace vmm_aspace_t;
#endif

#if defined(CONFIG_MULTIPARTITIONING) && defined(ARCH_HAS_MPU)
#include <kernel/mem_domain.h>
struct _mem_domain_info {
	/** memory domain queue node */
	struct list_node mem_domain_q_node;
	/** memory domain of the thread */
	struct k_mem_domain *mem_domain;
#ifdef WITH_HYPER_MODE
    uint8_t vmid;
#endif
};
#endif

__BEGIN_CDECLS

/* debug-enable runtime checks */
#if SYS_DEBUGLEVEL > 1
#define THREAD_STATS 1
#define THREAD_STACK_HIGHWATER 1
#define THREAD_STACK_BOUNDS_CHECK 1
#ifndef THREAD_STACK_PADDING_SIZE
#define THREAD_STACK_PADDING_SIZE 256
#endif
#endif

enum thread_state {
    THREAD_SUSPENDED = 0,
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_BLOCKED,
    THREAD_SLEEPING,
    THREAD_DEATH,
};

typedef int (*thread_start_routine)(void *arg);

/* thread local storage */
enum thread_tls_list {
#ifdef WITH_LIB_CONSOLE
    TLS_ENTRY_CONSOLE, // current console
#endif
#ifdef WITH_LIB_UTHREAD
    TLS_ENTRY_UTHREAD,
#endif
#ifdef WITH_LIB_LKUSER
    TLS_ENTRY_LKUSER,
#endif
    MAX_TLS_ENTRY
};

#define MAX_PLS_ENTRY                         32

#define THREAD_FLAG_DETACHED                  (1<<0)
#define THREAD_FLAG_FREE_STACK                (1<<1)
#define THREAD_FLAG_FREE_STRUCT               (1<<2)
#define THREAD_FLAG_REAL_TIME                 (1<<3)
#define THREAD_FLAG_IDLE                      (1<<4)
#define THREAD_FLAG_DEBUG_STACK_BOUNDS_CHECK  (1<<5)
#define THREAD_FLAG_PARTITION                 (1<<6)
#define THREAD_FLAG_VM                        (1<<7)

#define THREAD_MAGIC (0x74687264) // 'thrd'

#if THREAD_STATS
struct thread_specific_stats {
    sys_bigtime_t total_run_time;
    sys_bigtime_t last_run_timestamp;
    ulong schedules; // times this thread is scheduled to run.
};
#endif

typedef struct thread {
    struct list_node thread_list_node;
    /* architecture stuff */
    struct pcb_user_ctx arch;
    int magic;
    /* active bits */
    struct list_node queue_node;
    int priority;
    enum thread_state state;
    int remaining_quantum;
    unsigned int flags;
#if WITH_SMP
    int curr_cpu;
    int pinned_cpu; /* only run on pinned_cpu if >= 0 */
#endif
#if WITH_KERNEL_VM
    vmm_aspace_t *aspace;
#endif
#if defined(CONFIG_MULTIPARTITIONING) && defined(ARCH_HAS_MPU)
	/** memory domain info of the thread */
	struct _mem_domain_info mem_domain_info;
#endif
    /* if blocked, a pointer to the wait queue */
    struct wait_queue *blocking_wait_queue;
    status_t wait_queue_block_ret;

    /* stack stuff */
    void *stack;
    size_t stack_size;

    /* entry point */
    thread_start_routine entry;
    void *arg;

    /* return code */
    int retcode;
    struct wait_queue retcode_wait_queue;

    /* thread local storage */
    uintptr_t tls[MAX_TLS_ENTRY];

    char name[32];

    /* partition local storage for fp */
    uintptr_t pls[MAX_PLS_ENTRY];

#if THREAD_STATS
    struct thread_specific_stats stats;
#endif
} thread_t;


__END_CDECLS
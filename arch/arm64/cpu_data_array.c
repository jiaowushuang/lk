#include <arch/cpu_data.h>
#include <kernel/thread.h>

pcpu_kdata_t percpu_data[SMP_MAX_CPUS];
world_t world_server[SMP_MAX_CPUS][CPU_CONTEXT_NUM];



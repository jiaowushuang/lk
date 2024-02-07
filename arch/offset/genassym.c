#include <stdlib.h>
#include <arch/assym.h>
#include <arch/cpu_data.h>
#include <arch/thread.h>
#include <kernel/thread.h>

ASSYM(PCPU_KDATA_STACK_OFFSET, offsetof(struct pcpu_kdata, stack));
ASSYM(PCPU_KDATA_USER_OFFSET, offsetof(struct pcpu_kdata, user));
ASSYM(PCPU_KDATA_OPS_OFFSET, offsetof(struct pcpu_kdata, ops));
#if CRASH_REPORTING
ASSYM(PCPU_KDATA_CRASH_OFFSET, offsetof(struct pcpu_kdata, crash));
#endif
#if PLAT_CPU_DATA_SIZE
ASSYM(PCPU_KDATA_PLAT_OFFSET, offsetof(struct pcpu_kdata, plat_data))
#endif
ASSYM(PCPU_KDATA_TOTAL_SIZE, ROUNDUP(sizeof(struct pcpu_kdata), CACHE_LINE));
ASSYM(PCPU_KDATA_USER_ARCH_OFFSET, offsetof(struct thread, arch));
ASSYM(PCPU_KDATA_USER_CALLEE_FP_SIZE, sizeof(struct context_switch_frame));

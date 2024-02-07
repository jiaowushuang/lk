/* cheezy allocator that chews up space just after the end of the kernel mapping
 */
/* track how much memory we've used */
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>

/* Created by linker magic */
extern char __per_cpu_start[], __per_cpu_end[];

uintptr_t percpu_alloc_start = (uintptr_t)__per_cpu_start;
uintptr_t percpu_alloc_end = (uintptr_t)__per_cpu_end;

/* alloc the memory during the bootstarp */
void *percpu_alloc_mem(size_t len, size_t align)
{
	uintptr_t ptr;

	ptr = ROUNDUP(percpu_alloc_end, align);
	percpu_alloc_end = (ptr + ROUNDUP(len, align));

	return (void *)ptr;
}

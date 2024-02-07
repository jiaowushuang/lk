/*
 * Copyright (c) 2013, Google Inc. All rights reserved
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __UTHREAD_H
#define __UTHREAD_H

#include <sys/types.h>
#include <kern/list.h>
#include <kern/compiler.h>
#include <kern/err.h>
#include <kernel/thread.h>
#include <kernel/mutex.h>
#include <kernel/usercopy.h>

#if WITH_KERNEL_VM
#include <kernel/vm.h>
#endif

/*
 * user thread virtual memory layout:
 *
 * +-----------------------+ 0xFFFFFFFF
 * |                       |
 * |        kernel         |
 * |                       |
 * +-----------------------+ MEMBASE
 * |                       |
 * |                       |
 * |    other mappings     |
 * |                       |
 * |                       |
 * +-----------------------+ start_stack
 * |        stack          |
 * |     (grows down)      |
 * +-----------------------+
 * |        heap           |
 * +-----------------------+ start_brk
 * |        .bss           |
 * +-----------------------+
 * |        .data          |
 * +-----------------------+ start_data
 * |                       |
 * |     .text, .rodata    |
 * |                       |
 * +-----------------------+ start_code
 * |                       |
 * |    unmapped region    |
 * +-----------------------+ 0x00000000
 *
 * The actual boundaries between various segments are
 * determined by the binary image loader.
 *
 */
typedef struct uthread
{
	vaddr_t start_stack;
	vaddr_t entry;
	size_t stack_size;

	vaddr_t ns_va_bottom;

	/* uthread ID, unique per thread */
	uint32_t id;

	thread_t *thread;
	struct list_node uthread_list_node;
	void *private_data;
	void *app;
} uthread_t;


/* Create a new user thread */
uthread_t *uthread_create(const char *name, vaddr_t entry, int priority,
		vaddr_t stack_top, size_t stack_size, void *private_data);

/* Start the user thread */
status_t uthread_start(uthread_t *ut);

/* Exit current uthread */
void uthread_exit(int retcode) __NO_RETURN;


static inline uthread_t *uthread_get_current(void)
{
	return (uthread_t *)tls_get(TLS_ENTRY_UTHREAD);
}

#endif /* __UTHREAD_H */

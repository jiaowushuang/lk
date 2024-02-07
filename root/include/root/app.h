/*
 * Copyright (c) 2012-2013, NVIDIA CORPORATION. All rights reserved
 * Copyright (c) 2013, Google, Inc. All rights reserved
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

#ifndef __LIB_APP_H
#define __LIB_APP_H

#include <assert.h>
#include <kern/list.h>
#include <sys/types.h>
#include <root/uthread.h>

#include <root/uuid.h>

#define PF_TO_UTM_FLAGS(x) ((((x) & PF_W) ? 0 : ARCH_MMU_FLAG_PERM_RO) | \
			    (((x) & PF_X) ? 0 : ARCH_MMU_FLAG_PERM_NO_EXECUTE))

typedef struct
{
	uuid_t		uuid;
	uint32_t	min_stack_size;
	uint32_t	min_heap_size;
	uint32_t	map_io_mem_cnt;
	uint32_t	config_entry_cnt;
	uint32_t	*config_blob;
} app_props_t;

typedef struct app
{
	char 	app_name[256];

	vaddr_t end_bss;

	vaddr_t start_brk;
	vaddr_t cur_brk;
	vaddr_t end_brk;
    	vaddr_t load_bias;
	app_props_t props;

	void *app_img;

	uthread_t *ut;

	/* app local storage */
	void **als;
} app_t;

void app_init(void);
status_t app_setup_mmio(app_t *app,
		u_int mmio_id, vaddr_t *vaddr, uint32_t size);
app_t *app_find_by_uuid(uuid_t *uuid);
void app_forall(void (*fn)(app_t *ta, void *data), void *data);
user_addr_t thread_write_elf_tables(void* thread, user_addr_t stack_ptr);

typedef struct app_notifier
{
	struct list_node node;
	status_t (*startup)(app_t *app);
	status_t (*shutdown)(app_t *app);
} app_notifier_t;


/*
 * All app notifiers registration has to be complete before
 * libtrusty is initialized which is happening at INIT_LEVEL_APPS-1
 * init level.
 */
status_t register_app_notifier(app_notifier_t *n);

/*
 * All als slots must be allocated before libtrusty is initialized
 * which is happening at INIT_LEVEL_APPS-1 init level.
 */
int als_alloc_slot(void);

extern uint als_slot_cnt;

static inline void *als_get(struct app *app, int slot_id)
{
	uint slot = slot_id - 1;
	ASSERT(slot < als_slot_cnt);
	return app->als[slot];
}

static inline void als_set(struct app *app, int slot_id, void *ptr)
{
	uint slot = slot_id - 1;
	ASSERT(slot < als_slot_cnt);
	app->als[slot] = ptr;
}

#endif

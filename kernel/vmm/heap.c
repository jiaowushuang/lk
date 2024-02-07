#include "heap.h"

#include <kern/err.h>
#include <kernel/spinlock.h>

int g_size_mapping[] = { OBJ_MIN_SIZE, 16,   32,   64,	 128,
			 192,	       256,  384,  512,	 768,
			 1024,	       1280, 1536, 1792, 2048,
			 2560,	       3072, 3584, 3880, OBJ_MAX_SIZE };

struct pm_heap_struct_vtable g_pm_heap_struct_vtable = {
	.blk_alloc = blk_alloc,
	.blk_free = blk_free,
	.dump_heap = dump_heap,
};

struct pm_heap_struct g_pm_heap_struct[NR_HEAP_SIZE];

static struct vmm_page *free_slot(struct pm_heap_struct *slot, word_t table_no)
{
	paddr_t paddr;
	struct vmm_page *page;
	struct pm_node_struct *pn = get_recent_pn();

	status_t ret = pn->vtbl->alloc(pn, &paddr, 1, FUNC_OBJECT);

	if (ret) {
		dump_pages(ret);
		return NULL;
	}

	page = paddr_to_page(paddr);
	set_page_state(page, STATE_PAGE_SLUB_TYPES);
	page->slot_head = slot;
	set_page_table_no(page, table_no);
	clear_slot_object_present_all(page);
	list_add_head(&page->node, &HEAP_CACHE_ID(slot, table_no).partial_list);
	HEAP_CACHE_ID(slot, table_no).nr_partial++;
	HEAP_CACHE_ID(slot, table_no).nr_freepages++;

	return page;
}

static void *partial_slot(struct pm_heap_struct *slot, word_t table_no)
{
	struct vmm_page *page;
	void *object = NULL;
	struct list_node *partial_list;

	if (list_is_empty(&HEAP_CACHE_ID(slot, table_no).partial_list)) {
		page = free_slot(slot, table_no);
		if (page)
			goto final_partial;
		return NULL;
	}

	partial_list = &HEAP_CACHE_ID(slot, table_no).partial_list;
	page = list_head_entry(partial_list, page, node);

final_partial:

	// coverity[var_deref_model:SUPPRESS]
	if (get_page_slub_empty(page))
		HEAP_CACHE_ID(slot, table_no).nr_freepages--;
	// display_bitmap(page);
	object = get_freelist(page);
	assert(object);

	set_slot_object_present(page, object);
	HEAP_CACHE_ID(slot, table_no).current_page = page;
	HEAP_CACHE_ID(slot, table_no).free_pointer = get_freelist(page);

	return object;
}

/* alloc a slot */
void *kmalloc_slot(struct pm_heap_struct *slot)
{
	void *free_pointer;
	struct vmm_page *current_page;
	spin_lock_saved_state_t flags;
	word_t table_no;

	spin_lock_irqsave(&slot->slot_node_lock, flags);
	table_no = arch_curr_cpu_num();

	/* next free pointer */
	free_pointer = HEAP_CACHE_ID(slot, table_no).free_pointer;

	if (!free_pointer) {
		free_pointer = partial_slot(slot, table_no);
		current_page = HEAP_CACHE_ID(slot, table_no).current_page;
	} else {
		current_page = HEAP_CACHE_ID(slot, table_no).current_page;
		set_slot_object_present(current_page, free_pointer);
		HEAP_CACHE_ID(slot, table_no).free_pointer =
			get_freelist(current_page);
	}

	if (unlikely(!free_pointer)) {
		spin_unlock_irqrestore(&slot->slot_node_lock, flags);
		return NULL;
	}

	if (get_page_slub_full(current_page)) {
		list_del(&current_page->node);
		HEAP_CACHE_ID(slot, table_no).nr_partial--;
		list_add_head(&current_page->node,
			     &HEAP_CACHE_ID(slot, table_no).full_list);
		HEAP_CACHE_ID(slot, table_no).nr_full++;
		HEAP_CACHE_ID(slot, table_no).free_pointer = NULL;
		HEAP_CACHE_ID(slot, table_no).current_page = NULL;
	}

	spin_unlock_irqrestore(&slot->slot_node_lock, flags);
	// display_bitmap(current_page);
	return free_pointer;
}

/* free a slot */
status_t kfree_slot(struct pm_heap_struct *slot, struct vmm_page *page,
		    void *object)
{
	spin_lock_saved_state_t flags;
	word_t table_no;

	if (unlikely(!slot || !object || !page))
		return ERR_INVALID_ARGS;

	spin_lock_irqsave(&slot->slot_node_lock, flags);
	if (!get_slot_object_present(page, object)) {
		spin_unlock_irqrestore(&slot->slot_node_lock, flags);
		printf("FRee the memory that has been freed 0x%lx!\n",
			POINTER_TO_UINT(object));
		return ERR_NOT_ALLOWED;
	}

	table_no = get_page_table_no(page);
	if (table_no >= NODE_X_NU) {
		spin_unlock_irqrestore(&slot->slot_node_lock, flags);
		printf("saved core num is error!\n");
		return ERR_INVALID_ARGS;
	}

	if (get_page_slub_full(page)) {
		list_del(&page->node);
		HEAP_CACHE_ID(slot, table_no).nr_full--;
		list_add_head(&page->node,
			     &HEAP_CACHE_ID(slot, table_no).partial_list);
		HEAP_CACHE_ID(slot, table_no).nr_partial++;
	}

	/* update the page object usage */
	clear_slot_object_present(page, object);
	if (get_page_slub_empty(page)) {
		HEAP_CACHE_ID(slot, table_no).nr_freepages++;
		if (HEAP_CACHE_ID(slot, table_no).nr_freepages >
		    HEAP_FREE_PAGES) {
			struct pm_node_struct *pn = get_recent_pn();

			page->slot_head = NULL;
			clear_page_table_no(page);
			clear_page_state(page);
			list_del(&page->node);
			HEAP_CACHE_ID(slot, table_no).nr_partial--;
			HEAP_CACHE_ID(slot, table_no).nr_freepages--;
			if (page ==
			    HEAP_CACHE_ID(slot, table_no).current_page) {
				HEAP_CACHE_ID(slot, table_no).free_pointer =
					NULL;
				HEAP_CACHE_ID(slot, table_no).current_page =
					NULL;
			}

			status_t ret = pn->vtbl->free(page_to_paddr(page));

			spin_unlock_irqrestore(&slot->slot_node_lock, flags);
			if (ret)
				dump_pages(ret);
			return ret;
		}
	}

	spin_unlock_irqrestore(&slot->slot_node_lock, flags);
	return NO_ERROR;
}

status_t blk_alloc(void **ptr, size_t size)
{
	void *obj;
	int blk_i = size_mapping_heap(size);

	if (blk_i < 0)
		return ERR_INVALID_ARGS;

	obj = kmalloc_slot(&g_pm_heap_struct[blk_i]);
	if (!obj)
		return ERR_NO_MEMORY;
	if (ptr)
		*ptr = obj;
	return NO_ERROR;
}
status_t blk_free(void *ptr)
{
	struct vmm_page *page;
	struct pm_heap_struct *slot;

	page = virt_to_page(ptr);
	slot = page->slot_head;
	status_t ret = kfree_slot(slot, page, ptr);
	return ret;
}

status_t dump_heap(struct pm_heap_struct *const self)
{
	spin_lock_saved_state_t flags;
	struct pm_slot_struct *slot;

	spin_lock_irqsave(&self->slot_node_lock, flags);
	printf("[heap, size, count]: %p, %d, %d\n", self, self->total_size,
		  self->total_count);
	for (int cid = 0; cid < NODE_X_NU; cid++) {
		slot = &self->slot[cid];
		if (!slot->nr_partial && !slot->nr_full && !slot->nr_freepages)
			continue;
		printf(
			"[partial_list, nr_partial, full_list, nr_full, nr_freepages, freepoint, current_page]:%p, %d, %p, %d, %d, %p, %p\n",
			&slot->partial_list, slot->nr_partial, &slot->full_list,
			slot->nr_full, slot->nr_freepages, slot->free_pointer,
			slot->current_page);
	}
	spin_unlock_irqrestore(&self->slot_node_lock, flags);
	return NO_ERROR;
}

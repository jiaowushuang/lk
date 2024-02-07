#pragma once

#include "pa.h"


#define HEAP_FREE_PAGES 1024
#define NR_HEAP_SIZE 20

struct pm_heap_struct;
struct pm_heap_struct_vtable {
	status_t (*blk_alloc)(void **ptr, size_t size);
	status_t (*blk_free)(void *ptr);
	status_t (*dump_heap)(struct pm_heap_struct *const self);
};

struct pm_slot_struct {
	struct list_node partial_list;
	/* nr_partial = 0, means freelist is all full,
	 * free_count records the free or partial page
	 */
	word_t nr_partial;
	/* node empty page count to return buddy. */
	word_t nr_freepages;

	/* full */
	struct list_node full_list;
	word_t nr_full;

	/* pointer to next available object(free) */
	void *free_pointer;
	struct vmm_page *current_page;
};

struct pm_heap_struct {
	struct pm_slot_struct slot[NODE_X_NU];
	word_t total_size;
	word_t total_count;
	struct pm_heap_struct_vtable *vtbl;
	spin_lock_t slot_node_lock;
};

#define HEAP_CACHE(slot) ((slot)->slot[arch_curr_cpu_num()])
#define HEAP_CACHE_ID(slot, id) ((slot)->slot[(id)])
#define OBJ_MIN_SIZE 8
#define BITS_PER_PAGE (PAGE_SIZE / OBJ_MIN_SIZE) // 512
#define OBJ_BITS_SIZE BITMAP_SIZE(BITS_PER_PAGE)
#define OBJ_MAX_SIZE (PAGE_SIZE - OBJ_BITS_SIZE)

extern struct pm_heap_struct g_pm_heap_struct[NR_HEAP_SIZE];
extern struct pm_heap_struct_vtable g_pm_heap_struct_vtable;
extern int g_size_mapping[];

status_t blk_alloc(void **ptr, size_t size);
status_t blk_free(void *ptr);
status_t dump_heap(struct pm_heap_struct *const self);

static inline void page_to_bits(struct vmm_page *page, void **bits_pptr,
				char **obj_pptr, int *cnt, size_t *size)
{
	struct pm_heap_struct *slot = page->slot_head;
	void *bitmap_ptr = page_to_virt(page);

	assert(slot && bitmap_ptr);

	char *obj_ptr = (char *)bitmap_ptr + OBJ_BITS_SIZE;

	if (bits_pptr)
		*bits_pptr = bitmap_ptr;
	if (obj_pptr)
		*obj_pptr = obj_ptr;
	if (cnt)
		*cnt = slot->total_count;
	if (size)
		*size = slot->total_size;
}

static inline bool get_page_slub_full(struct vmm_page *page)
{
	int bit, cnt;
	void *bitmap_ptr;

	page_to_bits(page, &bitmap_ptr, NULL, &cnt, NULL);
	bit = find_first_zero_bit(bitmap_ptr, cnt);

	// printf("full bit is %ld\n", bit);
	if (bit != cnt)
		return false;

	return true;
}

static inline bool get_page_slub_empty(struct vmm_page *page)
{
	int bit, cnt;
	void *bitmap_ptr;

	page_to_bits(page, &bitmap_ptr, NULL, &cnt, NULL);
	bit = find_first_bit(bitmap_ptr, cnt);
	// printf("empty bit is %ld\n", bit);
	if (bit != cnt)
		return false;

	return true;
}

static inline int get_page_object_count(struct vmm_page *page)
{
	int bit, cnt, obj_cnt = 0;
	void *bitmap_ptr;

	page_to_bits(page, &bitmap_ptr, NULL, &cnt, NULL);
	for_each_set_bit(bit, bitmap_ptr, cnt) {
		if (bit != cnt)
			obj_cnt++;
	}
	return obj_cnt;
}

static inline void set_slot_object_present(struct vmm_page *page, void *object)
{
	void *bitmap_ptr;
	char *first_object;
	size_t size;
	int cnt;

	page_to_bits(page, &bitmap_ptr, &first_object, &cnt, &size);
	int nr = ((char *)object - first_object) / size;

	assert(nr < cnt);
	set_bit(nr, bitmap_ptr);
}

static inline void clear_slot_object_present(struct vmm_page *page, void *object)
{
	void *bitmap_ptr;
	char *first_object;
	size_t size;
	int cnt;

	page_to_bits(page, &bitmap_ptr, &first_object, &cnt, &size);
	int nr = ((char *)object - first_object) / size;

	assert(nr < cnt);
	clear_bit(nr, bitmap_ptr);
}

static inline void clear_slot_object_present_all(struct vmm_page *page)
{
	int cnt;
	void *bitmap_ptr;

	page_to_bits(page, &bitmap_ptr, NULL, &cnt, NULL);

	for (int bit = 0; bit < cnt; bit++)
		clear_bit(bit, bitmap_ptr);

	for (int bit = cnt; bit < BITS_PER_PAGE; bit++)
		set_bit(bit, bitmap_ptr);
}

static inline bool get_slot_object_present(struct vmm_page *page, void *object)
{
	int bit;
	int nr;

	void *bitmap_ptr;
	char *first_object;
	size_t size;
	int cnt;

	page_to_bits(page, &bitmap_ptr, &first_object, &cnt, &size);

	nr = ((char *)object - first_object) / size;
	assert(nr < cnt);

	bit = find_next_bit(bitmap_ptr, cnt, nr);
	if (bit != nr)
		return false;
	return true;
}

static inline void display_bitmap(struct vmm_page *page)
{
	unsigned long *bitmap_ptr = (unsigned long *)page_to_virt(page);

	for (int i = 0; i < BITS_TO_LONGS(BITS_PER_PAGE); i++)
		printf("bitmap-%d, %lx\n", i, bitmap_ptr[i]);
}

/* get current freelist */
static inline void *get_freelist(struct vmm_page *page)
{
	int bit;
	void *bitmap_ptr;
	char *first_object;
	size_t size;
	int cnt;

	page_to_bits(page, &bitmap_ptr, &first_object, &cnt, &size);
	bit = find_first_zero_bit(bitmap_ptr, cnt);

	if (bit != cnt)
		return (void *)(first_object + bit * size);
	return NULL;
}

static inline int size_mapping_heap(size_t size)
{
	if (size < OBJ_MIN_SIZE)
		return 0;
	if (size == OBJ_MAX_SIZE)
		return NR_HEAP_SIZE - 1;
	for (int i = 0; i < NR_HEAP_SIZE - 1; i++) {
		if (size == g_size_mapping[i])
			return i;
		if (size > g_size_mapping[i] && size < g_size_mapping[i + 1])
			return i + 1;
	}
	return -1;
}

static inline int heap_to_index(struct pm_heap_struct *slot)
{
	return slot - g_pm_heap_struct;
}

static inline struct pm_heap_struct *size_to_heap(size_t size)
{
	int i = size_mapping_heap(size);

	if (i < 0)
		return NULL;
	return &g_pm_heap_struct[i];
}

static inline word_t size_to_order(size_t size)
{
	unsigned long x = (unsigned long)((size - 1) >> PAGE_SIZE_SHIFT);

	return fls(x);
}

static inline word_t size_to_pages(size_t size)
{
	unsigned long x = (unsigned long)(size >> PAGE_SIZE_SHIFT);
	unsigned long y =
		(unsigned long)(size << (sizeof(x) * 8 - PAGE_SIZE_SHIFT));

	return y ? (x + 1) : x;
}



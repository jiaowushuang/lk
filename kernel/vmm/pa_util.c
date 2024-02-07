#include <kern/err.h>
#include <errno.h>
#include <kernel/usercopy.h>
#include "heap.h"
#include "pa.h"

void *kmalloc(size_t size)
{
	void *vaddr;
	status_t ret;

	if (!size)
		return NULL;

	/* if the size is more than SLOT_SHIFT_MAX, give the change to `buddy alloctor` */
	if (unlikely(size > OBJ_MAX_SIZE)) {
		paddr_t paddr = 0;
		struct pm_node_struct *pn = get_recent_pn();
		int gfp_flags = FUNC_OBJECT;

		ret = pn->vtbl->alloc(pn, &paddr, size_to_pages(size),
				      gfp_flags);
		if (ret) {
			dump_pages(ret);
			return NULL;
		}

		vaddr = (void *)paddr_to_kvaddr(paddr);
		goto done;
	}

	ret = blk_alloc(&vaddr, size);
	if (ret) {
		dump_objects(ret);
		return NULL;
	}


done:
	return vaddr;
}

/* free a 'kernel object' */
int kfree(void *object)
{
	status_t ret;

	if (!object)
		return ERR_INVALID_ARGS;

	struct pm_node_struct *pn = get_recent_pn();
	struct vmm_page *page = virt_to_page(object);

	if (get_page_state(page) != STATE_PAGE_SLUB_TYPES) {
		paddr_t paddr = page_to_paddr(page);

		ret = pn->vtbl->free(paddr);
		if (ret)
			dump_pages(ret);
		goto done;
	}

	ret = blk_free(object);
	if (ret)
		dump_objects(ret);
done:
	return ret;
}

void *kzalloc(size_t size)
{
	void *ptr;

	ptr = kmalloc(size);

	/* lack of memory */
	if (ptr == NULL)
		return NULL;

	memset(ptr, 0, size);
	return ptr;
}

void *get_pages(int order)
{
	void *object;
	status_t ret;
	paddr_t paddr;
	int gfp_flags;

	struct pm_node_struct *pn = get_recent_pn();

	gfp_flags = FUNC_MMU;
	ret = pn->vtbl->alloc(pn, &paddr, BIT(order), gfp_flags);
	if (ret) {
		dump_pages(ret);
		return NULL;
	}

	object = (void *)paddr_to_kvaddr(paddr);

	return object;
}

void dump_pages(status_t ret)
{
	struct pm_node_struct *pn = get_recent_pn();

	printf("pma page-caller retv is %d\n", ret);
	pn->vtbl->dump_node(pn);
}

void dump_objects(status_t ret)
{
	size_t size;
	int blk_i;

	printf("pma object-caller retv is %d\n", ret);
	for (int i = 0; i < NR_HEAP_SIZE; i++) {
		size = g_size_mapping[i];
		blk_i = size_mapping_heap(size);
		if (blk_i < 0)
			return;

		struct pm_heap_struct *heap = &g_pm_heap_struct[blk_i];

		heap->vtbl->dump_heap(heap);
	}
}

/**
 * memdup_user_nul - duplicate memory region from user space and NUL-terminate
 *
 * @src: source address in user space
 * @len: number of bytes to copy
 *
 * Return: an ERR_PTR() on failure.
 */
// void *memdup_user_nul(const void __user *src, size_t len)
void *memdup_user_nul(const void *src, size_t len)
{
	char *p;

	/*
	 * Always use GFP_KERNEL, since copy_from_user() can sleep and
	 * cause pagefault, which makes it pointless to use GFP_NOFS
	 * or GFP_ATOMIC.
	 */
	p = kmalloc(len + 1);
	if (!p)
		return ERR_PTR(-ENOMEM);

	if (copy_from_user(p, src, len)) {
		kfree(p);
		return ERR_PTR(-EFAULT);
	}
	p[len] = '\0';

	return p;
}



#pragma once

#include <sys/types.h>

// Pending TLBs to flush are stored as 63 bits, with the bottom bit stolen to
// store the terminal flag. 63 bits is more than enough as these entries are
// page aligned at the minimum.
struct pending_tlbs_t {
#ifdef IS_64BIT
	word_t va_shifted : 63;
#else
	word_t va_shifted : 31;
#endif	
	word_t terminal : 1;
};

// Maximum number of TLB entries we will queue before switching to ASID
// invalidation.
#define MAX_PENDING_TLBS 16

struct vspace;
struct vspace_vtable {
	status_t (*query)(struct vspace *const vspace, vaddr_t vaddr,
			  paddr_t *paddr, word_t *mmu_flags);
	status_t (*unmap)(struct vspace *const vspace, vaddr_t vaddr,
			  word_t npages);
	status_t (*map)(struct vspace *const vspace, unsigned long pfn,
			unsigned long vaddr, unsigned long npages,
			unsigned long right);
	status_t (*grant)(struct vspace *const vspace, unsigned long pfn,
			  unsigned long vaddr, unsigned long npages,
			  unsigned long right);
	status_t (*protect)(struct vspace *const vspace, unsigned long vaddr,
			    unsigned long npages, unsigned long right);
};

// system params
struct vspace {
	word_t type;
	word_t size;
	word_t base;
	word_t vaddr_base;
	word_t top_size_shift;
	word_t top_index_shift;
	word_t page_size_shift;
	paddr_t pgtbl_phys;
	word_t *pgtbl;
#ifdef KERNEL_32BIT	
	word_t asid : 8;
	word_t reserved : 24;
#else
	word_t asid : 8;
	word_t reserved : 56;
#endif	
	word_t pt_pages;
	bool need_invalidate;
	struct pending_tlbs_t pending_tlbs[MAX_PENDING_TLBS];
	size_t num_pending_tlbs;
	word_t map_recursion_cnts;
	word_t unmap_recursion_cnts;
	const struct vspace_vtable *vtbl;
	spin_lock_t lock;
};


static inline bool valid_vaddr(struct vspace *vspace, vaddr_t vaddr)
{
	return (vaddr >= vspace->base &&
		vaddr <= vspace->base + vspace->size - 1);
}

extern status_t vspace_create(struct vspace *vspace, word_t type, word_t base,
			      size_t size, word_t id);
extern status_t vspace_destroy(struct vspace *vspace);
extern void vspace_dump(struct vspace *vspace);
extern status_t query_locked(struct vspace *vspace, vaddr_t vaddr,
			     paddr_t *paddr, word_t *mmu_flags);
extern status_t handle_query(struct vspace *vspace, vaddr_t vaddr,
			     paddr_t *paddr, word_t *mmu_flags);
extern status_t handle_unmap(struct vspace *vspace, vaddr_t vaddr,
			     word_t npages);
extern status_t handle_map(struct vspace *vspace, unsigned long pfn,
			   unsigned long vaddr, unsigned long npages,
			   unsigned long right);
extern status_t handle_grant(struct vspace *vspace, unsigned long pfn,
			     unsigned long vaddr, unsigned long npages,
			     unsigned long right);
extern status_t handle_protect(struct vspace *vspace, unsigned long vaddr,
			       unsigned long npages, unsigned long right);
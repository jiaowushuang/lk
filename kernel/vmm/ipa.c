#include "ipa.h"
#include "pa.h"


struct compare_vaddr {
	word_t addr;
	word_t end;
};

enum g_mm_type { priv_guest, hyp_host, nr_gmm_type };

spin_lock_t g_vm_area_lock;
// priv guest or hyp host level (only the shemem of the level include it,
// but __init pgtable of priv or hyp not include here)
struct mm_struct g_mm_struct;
// void *g_mm_struct_test;

struct shemem_op g_shemem_op;

// global var
struct mm_struct_vtable g_mm_vtbl = {
	.ctor = mm_struct_ctor,
	.dtor = mm_struct_dtor,
	.dispatch = mm_struct_dispatch,
	.donate = mm_struct_donate,
	.lend = mm_struct_lend,
	.share = mm_struct_share,
	.relinquish = mm_struct_relinquish,
	.dump = mm_struct_dump,
	.query = query_va_ipa,
};

struct vm_area_struct_vtable g_area_vtbl = {
	.ctor = vm_area_struct_ctor,
	.dtor = vm_area_struct_dtor,
	.init = vm_area_struct_init,
	.dispatch = vm_area_struct_dispatch,
	.alloc = vm_area_struct_alloc,
	.free = vm_area_struct_free,
	.map = vm_area_struct_map,
	.unmap = vm_area_struct_unmap,
	.grant = vm_area_struct_grant,
	.protect = vm_area_struct_protect,
	.query = query_vma,
};

int owner_compare(void *area_node, void *key);
int access_compare(void *area_node, void *key);
status_t init_vma(struct mm_struct *mm, struct vm_area_struct **child, word_t addr, word_t end, word_t attr, word_t flags);
status_t merge_vma(struct vm_area_struct *self);
status_t destroy_vma(struct vm_area_struct *self);
status_t destroy_vma_node(avl_node_t *area_node);
status_t copy_vma(struct mm_struct *borrower, paddr_t paddr, struct vm_area_struct *parent, struct vm_area_struct **child,
		  word_t addr, word_t end, word_t flags, word_t prot);
status_t new_vma(struct mm_struct *mm, struct vm_area_struct *parent, struct vm_area_struct **child, word_t addr, word_t end,
		 word_t attr, word_t flags);
struct vm_area_struct *forward_merge_vma(struct mm_struct *mm, struct vm_area_struct *area, bool already, paddr_t paddr);
struct vm_area_struct *backword_merge_vma(struct mm_struct *mm, struct vm_area_struct *area, paddr_t paddr);
status_t insert_vma(struct mm_struct *mm, struct vm_area_struct *area,
		    paddr_t paddr);
status_t delete_vma(struct mm_struct *mm, struct vm_area_struct *self);
status_t revoke_vma(struct mm_struct *mm, struct vm_area_struct *self);
status_t revoke_delete_vma(struct mm_struct *mm, struct vm_area_struct *self);
status_t split_vma(struct mm_struct *mm, struct vm_area_struct **child, word_t addr, word_t end, word_t attr, word_t flags);
void *find_vma_tree(struct mm_struct *mm, word_t addr, word_t end);
void *find_vma_list(struct mm_struct *mm, word_t addr);
void *find_vma(struct mm_struct *mm, word_t addr, word_t end);
void *find_vma_locked(struct mm_struct *mm, word_t addr, word_t end);
void *find_vma_intersection(struct mm_struct *mm, word_t start_addr,
			    word_t end_addr, size_t size);
void *get_unmapped_area(struct mm_struct *mm, word_t addr, size_t size);
	void *get_mapped_area(struct mm_struct *mm, word_t addr, size_t size);	
status_t create_root_vma(struct mm_struct **mm_struct, word_t addr, word_t end,
			 word_t attr, word_t flags);
status_t destroy_root_vma(struct mm_struct *mm);
status_t create_root_vma_mapping(struct mm_struct *mm, word_t prot);
status_t destroy_vma_mapping(struct vm_area_struct *self);
status_t create_sub_vma(struct mm_struct *mm, struct vm_area_struct **child, word_t addr, word_t end, word_t attr, word_t flags);
status_t create_bro_vma(struct mm_struct *mm, struct vm_area_struct *child, paddr_t paddr);
status_t create_sub_vma_mapping(struct vm_area_struct *vma, word_t prot, word_t paddr);
status_t create_bro_vma_mapping(struct vm_area_struct *vma, word_t prot, paddr_t paddr);
status_t create_sub_vma_default(struct mm_struct *mm, struct vm_area_struct **child, word_t addr, word_t end);
status_t create_sub_vma_mapping_default(struct vm_area_struct *vma);
void insert_vma_struct(struct mm_struct *mm, struct vm_area_struct *area);
void revoke_vma_struct(struct vm_area_struct *area);

status_t find_vma_struct(struct mm_struct *mm, struct vm_area_struct **area, word_t addr);
status_t query_vma(struct vm_area_struct *self, paddr_t *paddr, word_t *mmu_flags);
status_t query_va_ipa(struct mm_struct *mm, vaddr_t va_ipa, paddr_t *paddr, word_t *mmu_flags);
status_t donate_vma(struct vm_area_struct *lender, struct mm_struct *borrower, word_t addr, word_t end, word_t attr, word_t flags,
		    word_t prot);
status_t lend_vma(struct vm_area_struct *lender, struct mm_struct *borrower, word_t addr, word_t end, word_t flags, word_t prot);
status_t share_vma(struct vm_area_struct *lender, struct mm_struct *borrower, word_t addr, word_t end, word_t flags, word_t prot);
status_t relinquish_vma(struct mm_struct *borrower, word_t addr);
struct vm_area_struct *expand_vma(struct mm_struct *borrower, paddr_t paddr, word_t addr, word_t end, word_t attr, word_t flags,
				  word_t prot);
const char *dump_stat(struct vm_area_struct *area);
const char *dump_attr(struct vm_area_struct *area);
status_t dump_title(struct vm_area_struct *area, char *title_str, char *prot_str);
void parse_command_of_owner(word_t *addr, word_t *end, word_t *attr, word_t *flags, word_t *prot, word_t *dpa,
			    struct prot_transaction *para);
uint16_t parse_command_of_access_count(struct prot_transaction *para);
struct mm_struct *get_mm_struct_of_vmid(uint16_t vmid);
void parse_command_of_access(word_t *addr, word_t *end, word_t *flags, struct mm_struct **mm, uint16_t *id, uint16_t index,
			     struct prot_transaction *para);
void dump_vma(avl_root_t *root);
// base function
static inline void get_area_vaddr(struct compare_vaddr *cmp_vaddr,
				  struct vm_area_struct *area)
{
	cmp_vaddr->addr = area->area_region.addr;
	cmp_vaddr->end = area->area_end;
}

int owner_compare(void *area_node, void *key)
{
	struct vm_area_struct *area =
		avl_entry(area_node, struct vm_area_struct, vm_area_avl);
	word_t addr = area->area_region.addr;
	word_t end = area->area_end;

	word_t key_addr = ((struct compare_vaddr *)key)->addr;
	word_t key_end = ((struct compare_vaddr *)key)->end;

	if (key_addr >= addr && key_end <= end)
		return 0;
	return (addr >= key_end) ? -1 : (end <= key_addr) ? 1 : ERR_FAULT;
}

int access_compare(void *area_node, void *key)
{
	struct vm_area_struct *area =
		list_entry(area_node, struct vm_area_struct, vm_area_list);
	word_t addr = area->area_region.addr;

	word_t key_addr = *((word_t *)key);

	return (addr <= key_addr && area->area_end > key_addr) ? 1 : 0;
}

status_t init_vma(struct mm_struct *mm, struct vm_area_struct **child,
		  word_t addr, word_t end, word_t attr, word_t flags)
{
	struct vm_area_struct *new_area = NULL;
	status_t ret;

	new_area = mm->area_vtbl->ctor(mm);
	if (!new_area)
		return ERR_NO_MEMORY;

	ret = mm->area_vtbl->init(new_area, addr, end, attr, flags);
	if (ret) {
		mm->area_vtbl->dtor(new_area);
		return ERR_INVALID_ARGS;
	}

	struct mmio_vdev *vdev;

	vdev = &new_area->vdev_ops;
	if (new_area->attr == MMIO) {
		if (get_init_flags(flags) && vdev->reset)
			vdev->reset(new_area);
		// OR
		else if (vdev->resume)
			vdev->resume(new_area);
	}
	if (child)
		*child = new_area;
	return NO_ERROR;
}

status_t merge_vma(struct vm_area_struct *self)
{
	struct vm_area_struct *prev, *next;
	word_t attr;
	status_t ret;

	prev = self->prev;
	next = self->next;
	if (prev) {
		attr = prev->attr;
		if (attr == GAPS && prev->area_end == self->area_region.addr) {
			ret = revoke_delete_vma(self->area_mm, self);
			if (ret)
				return ret;
			prev->area_end = self->area_end;
			ret = merge_vma(prev);
			if (ret)
				return ret;
			return NO_ERROR;
		}
	}
	if (next) {
		attr = next->attr;
		if (attr == GAPS && self->area_end == next->area_region.addr) {
			ret = revoke_delete_vma(self->area_mm, self);
			if (ret)
				return ret;
			next->area_region.addr = self->area_region.addr;
			ret = merge_vma(next);
			if (ret)
				return ret;
			return NO_ERROR;
		}
	}

	self->attr = GAPS;
	self->flags = 0;
	self->prot.word = 0;
	atomic_set(&self->stat, NA);

	return NO_ERROR;
}

status_t destroy_vma(struct vm_area_struct *self)
{
	status_t ret;

	ret = destroy_vma_mapping(self);
	if (ret) {
		printf("cannot dis-mapping\n");
		return ret;
	}

	ret = merge_vma(self);
	if (ret) {
		printf("cannot merge vma\n");
		return ret;
	}

	return NO_ERROR;
}

status_t destroy_vma_node(avl_node_t *area_node)
{
	status_t ret;
	struct vm_area_struct *area =
		avl_entry(area_node, struct vm_area_struct, vm_area_avl);

	if (vma_is_owner_parent(area->parent))
		return ERR_INVALID_ARGS;

	ret = destroy_vma_mapping(area);
	if (ret)
		return ret;

	atomic_sub(&area->area_owners, 1);
	area->area_mm->area_struct_cnt--;
	ret = delete_vma(area->area_mm, area);
	return ret;
}

status_t copy_vma(struct mm_struct *borrower, paddr_t paddr,
		  struct vm_area_struct *parent, struct vm_area_struct **child,
		  word_t addr, word_t end, word_t flags, word_t prot)
{
	struct vm_area_struct *new_area = NULL;
	word_t attr;

	// thread/process/vm linear address space isomorphism
	if (!addr && !end) {
		addr = parent->area_region.addr;
		end = parent->area_end;
	}
	// thread/process/vm linear address space heterogeneity
	attr = parent->attr;
	if (!flags)
		flags = parent->flags;
	if (!prot)
		prot = parent->prot.word;

	new_area = expand_vma(borrower, paddr, addr, end, attr, flags, prot);
	if (!new_area)
		return ERR_INVALID_ARGS;

	new_area->parent = parent;
	atomic_add(&parent->area_users, 1);
	if (child)
		*child = new_area;

	return NO_ERROR;
}

status_t new_vma(struct mm_struct *mm, struct vm_area_struct *parent,
		 struct vm_area_struct **child, word_t addr, word_t end,
		 word_t attr, word_t flags)
{
	struct vm_area_struct *new_area = NULL;
	status_t ret;
	vaddr_t old_vaddr;
	struct compare_vaddr cmp_vaddr;

	cmp_vaddr.addr = addr;
	cmp_vaddr.end = end;

	if (!mm || !parent)
		return ERR_INVALID_ARGS;

	if (!attr)
		attr = parent->prot.word;
	if (!flags)
		flags = parent->flags;

	ret = init_vma(mm, &new_area, addr, end, attr, flags);
	if (ret)
		return ret;

	old_vaddr = parent->area_end;
	parent->area_end = addr;

	ret = avl_insert(&mm->mm_area_avl, &new_area->vm_area_avl,
			 (void *)&cmp_vaddr, owner_compare);
	if (ret) {
		printf("overlap addr!\n");
		parent->area_end = old_vaddr;
		mm->area_vtbl->dtor(new_area);
		return ERR_NO_MEMORY;
	}

	new_area->parent = parent;
	parent->parent = parent;
	insert_dlist(parent, new_area);
	mm->area_struct_cnt++;

	if (child)
		*child = new_area;
	return NO_ERROR;
}

/* A tree represents a continuous vma, but there may be several continuous vmas
 * in the whole system. For this reason, it is not necessary to set up a
 * multi-class tree, but use the following as a bro-node method, but the
 * original tree with continuous addresses will also produce many holes.
 */
/* 1)
 * <----A---->
 *              <---vma--->
 *                            <----B---->
 * 2)
 * <----A---->
 *           <---vma--->
 *                            <----B---->
 * 3)
 * <----A---->
 *                  <---vma--->
 *                            <----B---->
 * 4) no vma overlap
 * <----A---->
 *        <----------vma------------>
 *                            <----B---->
 */

struct vm_area_struct *forward_merge_vma(struct mm_struct *mm,
					 struct vm_area_struct *area,
					 bool already, paddr_t paddr)
{
	struct compare_vaddr cmp_vaddr;
	struct vm_area_struct *vma;
	word_t addr, end, old_end;
	status_t ret;

	// forward
	get_area_vaddr(&cmp_vaddr, area);
	vma = avl_inferior_get(mm->mm_area_avl, (void *)&(cmp_vaddr),
			       owner_compare);
	if (!vma || ipa_pa_boundary(paddr))
		return vma;

	addr = area->area_region.addr;
	end = vma->area_end;
	old_end = area->area_end;
	if (addr == end && vma_is_friend(vma, area)) {
		if (already) {
			if (vma_users_derived(vma, area)) {
				ret = revoke_delete_vma(mm, area);
				if (ret)
					return vma;
			} else {
				return vma;
			}
		} else {
			atomic_sub(&area->area_owners, 1);
			ret = delete_vma(mm, area);
			if (ret)
				return vma;
		}

		vma->area_end = old_end;
		return area;
	}
	return vma;
}

struct vm_area_struct *backword_merge_vma(struct mm_struct *mm,
					  struct vm_area_struct *area,
					  paddr_t paddr)
{
	struct compare_vaddr cmp_vaddr;
	struct vm_area_struct *vma;
	word_t addr, end;
	status_t ret;

	// backword

	get_area_vaddr(&cmp_vaddr, area);
	vma = avl_superior_get(mm->mm_area_avl, (void *)&(cmp_vaddr),
			       owner_compare);
	if (!vma || ipa_pa_boundary(paddr))
		return area;

	addr = vma->area_region.addr;
	end = area->area_end;

	if (addr == end && vma_is_friend(vma, area)) {
		vma->area_region.addr = area->area_region.addr;
		atomic_sub(&area->area_owners, 1);
		ret = delete_vma(mm, area);
		if (ret)
			return area;
		return vma;
	}
	return area;
}

status_t insert_vma(struct mm_struct *mm, struct vm_area_struct *area,
		    paddr_t paddr)
{
	struct vm_area_struct *vma;
	status_t ret = NO_ERROR;
	bool already;

	vma = backword_merge_vma(mm, area, paddr);
	already = area != vma;
	area = vma;
	vma = forward_merge_vma(mm, area, already, paddr);

	if (!area_is_in_list(area) && vma != area) {
		if (!vma) {
			insert_dlist_before(mm->mm_area_q, area); // before
			mm->mm_area_q = area;
		} else {
			insert_dlist(vma, area);
		}

		struct compare_vaddr cmp_vaddr;

		get_area_vaddr(&cmp_vaddr, area);
		ret = avl_insert(&mm->mm_area_avl, &area->vm_area_avl,
				 (void *)&(cmp_vaddr), owner_compare);

		if (ret)
			return ret;

		mm->area_struct_cnt++;
		area->area_mm = mm;
	}
	return NO_ERROR;
}

status_t delete_vma(struct mm_struct *mm, struct vm_area_struct *self)
{
	if (atomic_read(&self->area_owners) || atomic_read(&self->area_users))
		return ERR_INVALID_ARGS;
	return mm->area_vtbl->dtor(self);
}

status_t revoke_vma(struct mm_struct *mm, struct vm_area_struct *self)
{
	void *vma;
	struct compare_vaddr cmp_vaddr;

	if (atomic_read(&self->area_owners) != 1 ||
	    atomic_read(&self->area_users) > 0)
		return ERR_INVALID_ARGS;

	get_area_vaddr(&cmp_vaddr, self);
	vma = avl_delete(&mm->mm_area_avl, (void *)&cmp_vaddr, owner_compare);

	if (!vma_valid(vma))
		return ERR_INVALID_ARGS;
	if (self == mm->mm_area_q)
		mm->mm_area_q = self->next;
	delete_dlist(self);
	atomic_sub(&self->area_owners, 1);
	mm->area_struct_cnt--;
	return NO_ERROR;
}

status_t revoke_delete_vma(struct mm_struct *mm, struct vm_area_struct *self)
{
	status_t ret;

	ret = revoke_vma(mm, self);
	if (ret)
		return ERR_INVALID_ARGS;
	ret = delete_vma(mm, self);
	if (ret)
		return ERR_INVALID_ARGS;
	return NO_ERROR;
}

/* 1)
 * <----A---->
 * <---vma--->
 * 2)
 * <------A------>
 * <---vma--->
 * 3)
 * <------A------>
 *     <---vma--->
 * 4)
 * <------A------>
 *   <---vma--->
 *
 * 5) no vma overlap
 * <------A------>
 *          <---vma--->
 *
 *  OR
 *
 *    <------A------>
 * <---vma--->
 */
status_t split_vma(struct mm_struct *mm, struct vm_area_struct **child,
		   word_t addr, word_t end, word_t attr, word_t flags)
{
	struct vm_area_struct *parent;
	struct compare_vaddr cmp_vaddr;

	cmp_vaddr.addr = addr;
	cmp_vaddr.end = end;
	parent = avl_inheritor_get(mm->mm_area_avl, (void *)&cmp_vaddr,
				   owner_compare);

	if (!parent)
		return ERR_OUT_OF_RANGE;
	if (vma_is_sharing(parent) || vma_is_lent(parent))
		return ERR_INVALID_ARGS;

	word_t start_addr, end_addr;

	start_addr = parent->area_region.addr;
	end_addr = parent->area_end;

	if (end > end_addr || addr < start_addr)
		return ERR_INVALID_ARGS;

	struct vm_area_struct *new_area = NULL;
	struct vm_area_struct *vma = NULL;
	word_t index;

	for (index = addr, vma = parent;; index = end, start_addr = end_addr) {
		if (index != start_addr) {
			status_t ret = new_vma(mm, vma, &new_area, index,
					       end_addr, 0, 0);
			if (ret)
				return ERR_NO_MEMORY;
		}

		if (index == end)
			break;
		if (new_area)
			vma = new_area;
		else
			vma = parent;
	}

	if (attr && flags) {
		vma->attr = attr;
		vma->flags = flags;
	}

	vma->parent = parent;

	if (child)
		*child = vma;

	return NO_ERROR;
}

// owner
void *find_vma_tree(struct mm_struct *mm, word_t addr, word_t end)
{
	struct vm_area_struct *area;
	void *area_node;
	struct compare_vaddr cmp_vaddr;

	cmp_vaddr.addr = addr;
	cmp_vaddr.end = end;
	area_node =
		avl_search(mm->mm_area_avl, (void *)&cmp_vaddr, owner_compare);
	if (!area_node)
		return NULL;
	if (area_node == AVL_ERR_NODE)
		return AVL_ERR_NODE;
	area = containerof(area_node, struct vm_area_struct, vm_area_avl);
	return area;
}

// access
void *find_vma_list(struct mm_struct *mm, word_t addr)
{
	struct vm_area_struct *area;
	void *area_node;

	area_node =
		list_search(&mm->mm_area_list, (void *)&addr, access_compare);
	if (!area_node)
		return NULL;
	area = containerof(area_node, struct vm_area_struct, vm_area_list);
	return area;
}

// owner
void *find_vma(struct mm_struct *mm, word_t addr, word_t end)
{
	return find_vma_tree(mm, addr, end);
}

void *find_vma_locked(struct mm_struct *mm, word_t addr, word_t end)
{
	spin_lock_saved_state_t sflags;
	void *obj;

	spin_lock_irqsave(&mm->area_struct_lock, sflags);
	obj = find_vma(mm, addr, end);
	spin_unlock_irqrestore(&mm->area_struct_lock, sflags);
	return obj;
}

void *find_vma_intersection(struct mm_struct *mm, word_t start_addr,
			    word_t end_addr, size_t size)
{
	void *vma;

	for (word_t addr = start_addr; addr < end_addr; addr += PAGE_SIZE) {
		vma = find_vma_locked(mm, addr, addr + size);
		if (vma_valid(vma))
			return vma;
	}
	return NULL;
}

void *get_unmapped_area(struct mm_struct *mm, word_t addr, size_t size)
{
	struct vm_area_struct *vma;

	vma = find_vma_locked(mm, addr, addr + 1);

	if (!vma_valid(vma))
		return NULL;
	for (; vma != NULL; vma = vma->next) {
		if (vma->attr == GAPS && size <= vma_size(vma))
			return vma;
	}

	return NULL;
}

void *get_mapped_area(struct mm_struct *mm, word_t addr, size_t size)
{
	struct vm_area_struct *vma;

	vma = find_vma_locked(mm, addr, addr + 1);

	if (!vma_valid(vma))
		return NULL;
	for (; vma != NULL; vma = vma->next) {
		if (vma_is_mapping(vma) && size <= vma_size(vma))
			return vma;
	}

	return NULL;
}

/* The physical address ranges of different thread/process/vm root nodes cannot
 * overlap, and thus the physical address ranges of their child nodes cannot
 * overlap; a single thread/process/vm divides child nodes from the root node,
 * so linear addresses cannot overlap. But there is an exception, that is, the
 * physical address space that is borrowed and shared does not consider the
 * range set by the root node when allocating the virtual address space, which
 * overlaps with the linear address range set by the root node.
 */

// init, extern
status_t create_root_vma(struct mm_struct **mm_struct, word_t addr, word_t end,
			 word_t attr, word_t flags)
{
	// LOCK
	struct mm_struct *mm;
	uint16_t mid;
	uint8_t vs_type;
	spin_lock_saved_state_t iflags;

	mm = mm_struct_ctor(get_g_flags(flags));
	if (!mm)
		return ERR_NO_MEMORY;

	spin_lock_irqsave(&mm->area_struct_lock, iflags);
	struct vspace  *space = kmalloc(sizeof(struct vspace ));

	if (!space) {
		spin_unlock_irqrestore(&mm->area_struct_lock, iflags);
		kfree(mm);
		return ERR_NO_MEMORY;
	}

	struct vm_area_struct *root_vma = mm->area_vtbl->ctor(mm);

	vs_type = get_vs_flags(flags);
	mid = get_mid_flags(flags);

	status_t ret;

	ret = vspace_create(space, vs_type, addr, end - addr, mid);
	printf("vmid %d\n", mid);
	// guest
	// USER_ASPACE_BASE
	// BIT(MMU_GUEST_SIZE_SHIFT)
	if (ret || !root_vma) {
		vspace_destroy(space);
		kfree(space);
		mm->area_vtbl->dtor(root_vma);
		spin_unlock_irqrestore(&mm->area_struct_lock, iflags);
		mm->mm_vtbl->dtor(mm);
		return ERR_NO_MEMORY;
	}

	mm->mm_space = space;

	if (!addr)
		addr = space->base;
	if (!end)
		end = space->base + space->size;

	mm->area_vtbl->init(root_vma, addr, end, attr, flags);
	mm->mm_area_avl = &root_vma->vm_area_avl;
	mm->mm_area_q = root_vma;
	mm->area_struct_cnt++;
	mm->mm_start = addr;
	mm->mm_end = end;

	if (mm_struct)
		*mm_struct = mm;

	spin_unlock_irqrestore(&mm->area_struct_lock, iflags);
	return NO_ERROR;
}

// runtime, extern
status_t destroy_root_vma(struct mm_struct *mm)
{
	status_t ret;
	spin_lock_saved_state_t flags;

	spin_lock_irqsave(&mm->area_struct_lock, flags);

	// mm->mm_vtbl->dump(mm);

	ret = avl_tree_erase(&mm->mm_area_avl, destroy_vma_node);
	if (ret) {
		printf("destroy fail!\n");
		spin_unlock_irqrestore(&mm->area_struct_lock, flags);
		return ret;
	}

	ret = vspace_destroy(mm->mm_space);
	if (ret) {
		printf("destroy fail!\n");
		spin_unlock_irqrestore(&mm->area_struct_lock, flags);
		return ret;
	}

	kfree(mm->mm_space);
	spin_unlock_irqrestore(&mm->area_struct_lock, flags);

	ret = mm->mm_vtbl->dtor(mm);
	return ret;
}

// init, extern
status_t create_root_vma_mapping(struct mm_struct *mm, word_t prot)
{
	status_t ret;
	struct vm_area_struct *root_vma;
	paddr_t paddr;
	word_t type;

	assert(mm);

	if (!prot)
		return ERR_INVALID_ARGS;

	spin_lock_saved_state_t flags;

	spin_lock_irqsave(&mm->area_struct_lock, flags);

	root_vma = mm->mm_area_q;
	type = root_vma->attr;
	if (type == GAPS) {
		spin_unlock_irqrestore(&mm->area_struct_lock, flags);
		return ERR_INVALID_ARGS;
	}

	root_vma->prot.word = prot;
	atomic_set(&root_vma->stat, EA);
	type = get_res_flags(root_vma->flags);
	paddr = root_vma->vtbl->alloc(root_vma, type);
	if (!paddr) {
		spin_unlock_irqrestore(&mm->area_struct_lock, flags);
		return ERR_NO_MEMORY;
	}

	ret = root_vma->vtbl->map(root_vma, paddr, prot);
	spin_unlock_irqrestore(&mm->area_struct_lock, flags);

	if (ret)
		return ret;
	return NO_ERROR;
}

status_t destroy_vma_mapping(struct vm_area_struct *self)
{
	status_t ret;

	if (atomic_read(&self->area_owners) != 1 ||
	    atomic_read(&self->area_users) > 0) {
		printf("still other vma in sharing!\n");
		return ERR_INVALID_ARGS;
	}

	if (self->prot.word != 0) {
		paddr_t paddr;

		ret = query_vma(self, &paddr, NULL);
		if (ret)
			return ERR_INVALID_ARGS;
		ret = self->vtbl->unmap(self);
		if (ret)
			return ERR_INVALID_ARGS;

		self->prot.word = 0;
		atomic_set(&self->stat, NA);

		if (get_alloc_flags(self->flags)) {
			ret = self->vtbl->free(self, paddr);
			if (ret)
				return ERR_INVALID_ARGS;
		}
	}
	return NO_ERROR;
}

// runtime
status_t create_sub_vma(struct mm_struct *mm, struct vm_area_struct **child,
			word_t addr, word_t end, word_t attr, word_t flags)
{
	status_t ret;

	assert(mm);

	ret = split_vma(mm, child, addr, end, attr, flags);
	if (ret)
		return ret;

	return NO_ERROR;
}

// bro address space is diff from root address space
status_t create_bro_vma(struct mm_struct *mm, struct vm_area_struct *child,
			paddr_t paddr)
{
	status_t ret;

	assert(mm && child);

	ret = insert_vma(mm, child, paddr);
	if (ret)
		return ret;

	return NO_ERROR;
}

status_t create_sub_vma_mapping(struct vm_area_struct *vma, word_t prot,
				word_t paddr)
{
	status_t ret;
	word_t type;

	assert(vma);

	struct area_flags flags;

	flags.word = vma->flags;

	if (!prot)
		prot = vma->parent->prot.word;

	type = vma->attr;
	if (type == GAPS)
		return ERR_INVALID_ARGS;
	vma->prot.word = prot;
	atomic_set(&vma->stat, EA);
	if (ipa_pa_boundary(paddr))
		return NO_ERROR;
	if (ipa_pa_omap(paddr) && vma->parent->attr != GAPS &&
	    get_alloc_flags(vma->parent->flags)) {
		ret = vma->vtbl->protect(vma, prot);
		return ret;
	}
	type = get_res_flags(vma->flags);
	if (ipa_pa_omap(paddr)) {
		set_alloc_flags(&flags, true);
		vma->flags = flags.word;
		paddr = vma->vtbl->alloc(vma, type);
		if (!paddr)
			return ERR_NO_MEMORY;
	}

	ret = vma->vtbl->map(vma, paddr, prot);
	if (ret)
		return ret;
	return NO_ERROR;
}

status_t create_bro_vma_mapping(struct vm_area_struct *vma, word_t prot,
				paddr_t paddr)
{
	status_t ret;
	word_t type;

	assert(vma);

	struct area_flags flags;

	flags.word = vma->flags;

	if (!prot)
		return ERR_INVALID_ARGS;
	type = vma->attr;
	if (type == GAPS)
		return ERR_INVALID_ARGS;
	vma->prot.word = prot;
	atomic_set(&vma->stat, EA);
	if (ipa_pa_boundary(paddr))
		return NO_ERROR;
	type = get_res_flags(vma->flags);
	if (ipa_pa_omap(paddr)) {
		set_alloc_flags(&flags, true);
		vma->flags = flags.word;
		paddr = vma->vtbl->alloc(vma, type);
		if (!paddr)
			return ERR_NO_MEMORY;
	}

	ret = vma->vtbl->map(vma, paddr, prot);
	if (ret)
		return ret;
	return NO_ERROR;
}

status_t create_sub_vma_default(struct mm_struct *mm,
				struct vm_area_struct **child, word_t addr,
				word_t end)
{
	return create_sub_vma(mm, child, addr, end, 0, 0);
}

status_t create_sub_vma_mapping_default(struct vm_area_struct *vma)
{
	return create_sub_vma_mapping(vma, 0, 0);
}

// access - canot change the area, and copy one
/* The linked list is sorted from high address to low address, and the head is
 * the highest address range
 */
void insert_vma_struct(struct mm_struct *mm, struct vm_area_struct *area)
{
	struct vm_area_struct *vma, *next_vma;

	list_for_each_entry_safe(&mm->mm_area_list, vma, next_vma,
				  vm_area_list) {
		if (area->area_end < vma->area_end) {
			list_add_before(&area->vm_area_list,
					&vma->vm_area_list);
			return;
		}
	}

	list_add_tail(&area->vm_area_list, &mm->mm_area_list);
}

void revoke_vma_struct(struct vm_area_struct *area)
{
	list_del(&area->vm_area_list);
	atomic_sub(&area->parent->area_users, 1);
	if (!atomic_read(&area->parent->area_users))
		atomic_set(&area->parent->stat, EA);
}

status_t find_vma_struct(struct mm_struct *mm, struct vm_area_struct **area,
			 word_t addr)
{
	struct vm_area_struct *vma;

	vma = find_vma_list(mm, addr);
	if (!vma)
		return ERR_INVALID_ARGS;
	if (area)
		*area = vma;
	return NO_ERROR;
}

// derived function
/* It is necessary to limit the access range of each hypervisor in the
 * privileged state. Its most basic code segment, data segment, etc. can be
 * pre-configured, and sufficient free space is reserved for it. The free space
 * can be statically allocated to the corresponding memory area. For global data
 * structures, and dynamically allocate corresponding memory blocks or page
 * frames, but the above parts can only be used exclusively by the hypervisor
 * (including data structures or caches designed by the hypervisor), and other
 * free space can be used by the Guest OS running on it Or the user application
 * is exclusive (or allocated and shared by itself to other Guest OS or user
 * application), once marked as exclusive, neither the hypervisor nor the user
 * can access each other; the remaining physical memory(not in hypervisor
 * vspace ) can be used for the global Guest OS or user application The data
 * shared between the two can be mapped to the hypervisor first, and then trap
 * into use, which can control the scope of influence, because the physical
 * address of the remaining memory is not a linear mapping relationship with the
 * virtual address of the hypervisor, so when the Guest OS or user application
 * accesses out of bounds , you can report an error in time
 */

// owner, runtime, extern
status_t query_vma(struct vm_area_struct *self, paddr_t *paddr,
		   word_t *mmu_flags)
{
	status_t ret;
	struct vspace  *mm_space;

	mm_space = self->area_mm->mm_space;

	ret = mm_space->vtbl->query(mm_space, self->area_region.addr, paddr,
				    mmu_flags);
	return ret;
}

status_t query_va_ipa(struct mm_struct *mm, vaddr_t va_ipa, paddr_t *paddr,
		      word_t *mmu_flags)
{
	status_t ret;
	struct vspace  *mm_space;

	mm_space = mm->mm_space;

	ret = mm_space->vtbl->query(mm_space, va_ipa, paddr, mmu_flags);
	return ret;
}

// owner
status_t donate_vma(struct vm_area_struct *lender, struct mm_struct *borrower,
		    word_t addr, word_t end, word_t attr, word_t flags,
		    word_t prot)
{
	status_t ret;
	paddr_t paddr;
	size_t size;
	struct vm_area_struct *vma;

	assert(lender);

	size = lender->area_end - lender->area_region.addr;
	if (size != end - addr)
		return ERR_INVALID_ARGS;

	ret = query_vma(lender, &paddr, NULL);
	if (ret)
		return ERR_INVALID_ARGS;

	ret = lender->vtbl->unmap(lender);
	if (ret)
		return ERR_INVALID_ARGS;
	ret = merge_vma(lender);
	if (ret)
		return ret;

	vma = expand_vma(borrower, paddr, addr, end, attr, flags, prot);
	if (!vma)
		return ERR_INVALID_ARGS;
	return NO_ERROR;
}

// access

status_t lend_vma(struct vm_area_struct *lender, struct mm_struct *borrower,
		  word_t addr, word_t end, word_t flags, word_t prot)
{
	status_t ret;
	paddr_t paddr;
	struct vm_area_struct *new_vma;
	size_t size;

	size = lender->area_end - lender->area_region.addr;
	if (size != end - addr)
		return ERR_INVALID_ARGS;

	ret = query_vma(lender, &paddr, NULL);
	if (!ret) {
		ret = lender->vtbl->unmap(lender);
		if (ret)
			return ERR_INVALID_ARGS;
	}

	ret = copy_vma(borrower, paddr, lender, &new_vma, addr, end, flags,
		       prot);
	if (ret)
		return ERR_INVALID_ARGS;

	insert_vma_struct(borrower, new_vma);

	return NO_ERROR;
}

// access

status_t share_vma(struct vm_area_struct *lender, struct mm_struct *borrower,
		   word_t addr, word_t end, word_t flags, word_t prot)
{
	status_t ret;
	paddr_t paddr;
	struct vm_area_struct *new_vma;
	size_t size;

	size = lender->area_end - lender->area_region.addr;
	if (size != end - addr)
		return ERR_INVALID_ARGS;
	ret = query_vma(lender, &paddr, NULL);
	if (ret)
		return ERR_INVALID_ARGS;

	ret = copy_vma(borrower, paddr, lender, &new_vma, addr, end, flags,
		       prot);
	if (ret)
		return ERR_INVALID_ARGS;

	insert_vma_struct(borrower, new_vma);

	return NO_ERROR;
}

// access
status_t relinquish_vma(struct mm_struct *borrower, word_t addr)
{
	status_t ret;
	struct vm_area_struct *borrower_vma;

	ret = find_vma_struct(borrower, &borrower_vma, addr);
	if (ret)
		return ERR_INVALID_ARGS;

	revoke_vma_struct(borrower_vma);

	ret = borrower_vma->vtbl->unmap(borrower_vma);
	if (ret)
		return ERR_INVALID_ARGS;

	ret = merge_vma(borrower_vma);
	if (ret)
		return ret;
	//ret = merge_vma(borrower_vma->parent);
	//if (ret)
		//return ret;
	return NO_ERROR;
}

const char *dump_stat(struct vm_area_struct *area)
{
	switch (atomic_read(&area->stat)) {
	case 0:
		return "NA";
	case 1:
		return "EA";
	case 2:
		return "SA";
	case 3:
		return "LA";
	default:
		return "NoA";
	}
}

const char *dump_attr(struct vm_area_struct *area)
{
	switch (area->attr) {
	case 0:
		return "GAPS";
	case 1:
		return "NORMAL";
	case 2:
		return "DMA";
	case 3:
		return "MMIO_EMUL";
	case 4:
		return "MMIO";
	default:
		return "NoT";
	}
}

status_t dump_title(struct vm_area_struct *area, char *title_str,
		    char *prot_str)
{
	struct map_flags prot;
	const char *r, *w, *x, *io, *mem_attr, *title;

	prot.word = area->prot.word;
	r = (prot.can_read) ? "r" : "-";
	w = (prot.can_write) ? "w" : "-";
	x = (prot.can_exec) ? "x" : "-";
	io = (prot.is_io_map) ? " io" : (prot.is_vio) ? " vio" : " -";

	switch (area->attr) {
	case GAPS:
		return ERR_INVALID_ARGS;
	case MMIO_EMUL:
		title = "Emulation Device";
		break;
	case MMIO:
		title = "Passthrough Device";
		break;
	case NORMAL:
		if (prot.is_vio)
			title = "Virtio Device";
		else if (prot.can_exec)
			title = "Payload";
		else
			title = "Normal";
		break;
	default:
		title = "NoT";
		break;
	}

	switch (prot.mem_attr) {
	case 1:
		mem_attr = " nGnRnE";
		break;
	case 2:
		mem_attr = " nGnRE";
		break;
	case 3:
		mem_attr = " GRE";
		break;
	case 4:
		mem_attr = " NC";
		break;
	case 5:
		mem_attr = " NORMAL";
		break;
	case 6:
		mem_attr = " WT";
		break;
	default:
		mem_attr = " NOT";
		break;
	}

	snprintf(prot_str, 20, "%s%s%s%s%s", r, w, x, io, mem_attr);
	snprintf(title_str, 20, "%s", title);
	return NO_ERROR;
}

void dump_vma(avl_root_t *root)
{
	char title[20] = { 0 };
	char prot[20] = { 0 };
	avl_node_t *area_node = *root;

	struct vm_area_struct *area =
		avl_entry(area_node, struct vm_area_struct, vm_area_avl);

	if (dump_title(area, title, prot))
		return;
	printf(
		"%s: [0x%lx, 0x%lx] flags[%d] stat[%s] attr[%s] prot[%s] users[%d] owers[%d]\n",
		title, area->area_region.addr, area->area_end, area->flags,
		dump_stat(area), dump_attr(area), prot,
		atomic_read(&area->area_users),
		atomic_read(&area->area_owners));
}

struct vm_area_struct *expand_vma(struct mm_struct *borrower, paddr_t paddr,
				  word_t addr, word_t end, word_t attr,
				  word_t flags, word_t prot)
{
	struct vm_area_struct *new_vma = NULL;
	struct vm_area_struct *area;
	status_t ret;
	bool g = get_g_flags(flags);

	area = find_vma(borrower, addr, end);
	if (!area && !g) {
		ret = init_vma(borrower, &new_vma, addr, end, attr, flags);
		if (ret)
			return NULL;
		ret = create_bro_vma_mapping(new_vma, prot, paddr);
		if (ret)
			return NULL;
		ret = create_bro_vma(borrower, new_vma, paddr);
		if (ret)
			return NULL;
	} else if (vma_valid(area) && (g || !vma_is_dirty(area))) {
		ret = create_sub_vma(borrower, &new_vma, addr, end, attr,
				     flags);
		if (ret)
			return NULL;

		ret = create_sub_vma_mapping(new_vma, prot, paddr);
		if (ret)
			return NULL;
	} else {
		return NULL;
	}
	return new_vma;
}

// mm_struct function
struct mm_struct *mm_struct_ctor(bool g)
{
	struct mm_struct *mm;

	if (g)
		mm = &g_mm_struct;
	else
		mm = kmalloc(sizeof(struct mm_struct));

	if (!mm)
		return NULL;

	mm->mm_area_avl = NULL;
	init_list_node(&mm->mm_area_list);
	mm->mm_area_q = NULL;
	mm->mm_vtbl = &g_mm_vtbl;
	mm->area_vtbl = &g_area_vtbl;
	if (g)
		mm->shemem_ops = &g_shemem_op;
	else
		mm->shemem_ops = NULL;
	mm->area_struct_cnt = 0;
	spin_lock_init(&mm->area_struct_lock);
	mm->mm_space = NULL;

	return mm;
}

status_t mm_struct_dtor(struct mm_struct *mm)
{
	if (!mm || mm == &g_mm_struct)
		return ERR_INVALID_ARGS;
	if (mm->area_struct_cnt)
		return ERR_INVALID_ARGS;
	kfree(mm);
	return NO_ERROR;
}

// extern __init
struct mm_struct *init_mm_struct(word_t addr, word_t end, word_t attr,
				 word_t flags)
{
	struct mm_struct *mm;
	status_t ret = create_root_vma(&mm, addr, end, attr, flags);

	if (ret)
		return NULL;
	return mm;
}

status_t deinit_mm_struct(struct mm_struct *mm)
{
	printf("vma cnt %ld\n", mm->area_struct_cnt);
	return destroy_root_vma(mm);
}

/* parse flags to dispatch para */
void parse_command_of_owner(word_t *addr, word_t *end, word_t *attr,
			    word_t *flags, word_t *prot, word_t *dpa,
			    struct prot_transaction *para)
{
	struct area_flags aflags;
	struct map_flags mflags;
	union vma_api_usage_flags uflags;
	union region_struct_handle r_handle;

	uint16_t vmid = para->owner_id;
	uint32_t pflags = para->flags;
	uint64_t handle = para->handle;
	uint64_t badge = para->badge;
	uint64_t range = para->range;

	r_handle.word = handle;
	uflags.word = pflags;

	bool g = r_handle.g;
	bool ns = !r_handle.secure;
	bool usr = r_handle.usr;
	uint8_t vs = r_handle.st;
	uint8_t mem_attr = uflags.mem_attr;
	uint8_t vma_attr = uflags.vma_attr;
	bool init = uflags.init;
	bool r = uflags.r;
	bool w = uflags.w;
	bool x = uflags.x;
	bool io = uflags.io;
	bool res = uflags.res;
	bool large = uflags.large;
	bool vio = uflags.vio;

	set_mm_struct_flags(&aflags, g, vmid, init, vs, res);
	set_mm_struct_mflags(&mflags, r, w, x, io, mem_attr, ns, usr, large,
			     vio);
	uint64_t mask = ~MASK(PAGE_SIZE_SHIFT);

	*addr = handle & mask;
	*end = *addr + range;
	*attr = vma_attr;
	*flags = aflags.word;
	*prot = mflags.word;
	*dpa = badge;
}

uint16_t parse_command_of_access_count(struct prot_transaction *para)
{
	return para->access_prot_cnt;
}

struct mm_struct *get_mm_struct_of_vmid(uint16_t vmid)
{
	return NULL;
}

void parse_command_of_access(word_t *addr, word_t *end, word_t *flags,
			     struct mm_struct **mm, uint16_t *id, uint16_t index,
			     struct prot_transaction *para)
{
	struct access_prot prot;
	union region_struct_handle handle;
	u64 range;
	uint16_t vmid;

	prot = para->access_prot_array[index];
	handle.word = prot.handle;
	range = prot.range;
	vmid = prot.ap_id;

	uint64_t mask = ~MASK(PAGE_SIZE_SHIFT);

	*addr = handle.word & mask;
	*end = *addr + range;
	*flags = update_mm_struct_mflags(*flags, prot.mem_ap & VMA_RIGHT_R,
					 prot.mem_ap & VMA_RIGHT_W,
					 prot.mem_ap & VMA_RIGHT_X);

	*mm = get_mm_struct_of_vmid(vmid); // vmid get
	*id = vmid;
}

// hypcall dispatch struct prot_transaction
status_t mm_struct_dispatch(struct mm_struct *mm, area_evt_t e, void *p)
{
	word_t addr;
	word_t end;
	word_t attr;
	word_t flags;
	word_t prot;
	word_t dpa;

	status_t ret;
	spin_lock_saved_state_t sflags, gflags;

	if (!mm || !valid_area_op(e) || !p)
		return ERR_INVALID_ARGS;

	parse_command_of_owner(&addr, &end, &attr, &flags, &prot, &dpa, p);

	if (!valid_vaddr(mm->mm_space, addr) || !valid_vaddr(mm->mm_space, end))
		return ERR_OUT_OF_RANGE;

	if (!IS_PAGE_ALIGNED(addr) || !IS_PAGE_ALIGNED(end) || addr == end)
		return ERR_INVALID_ARGS;

	spin_lock_irqsave(&mm->area_struct_lock, sflags);
	if (e == CREATE_OP) {
		ret = create_sub_vma(mm, NULL, addr, end, attr, flags);
		spin_unlock_irqrestore(&mm->area_struct_lock, sflags);
		return ret;
	}

	struct vm_area_struct *self;

	if (e == CREATE_MAP_OP || e == SHARE_OP || e == LEND_OP) {
		assert(attr != GAPS);
		ret = create_sub_vma(mm, &self, addr, end, attr, flags);
		if (ret) {
			spin_unlock_irqrestore(&mm->area_struct_lock, sflags);
			return ret;
		}

		ret = create_sub_vma_mapping(self, prot, dpa);

		if (e == CREATE_MAP_OP) {
			spin_unlock_irqrestore(&mm->area_struct_lock, sflags);
			return ret;
		} else if (ret) {
			spin_unlock_irqrestore(&mm->area_struct_lock, sflags);
			return ret;
		}
	}

	self = find_vma(mm, addr, end);
	if (!vma_valid(self)) {
		spin_unlock_irqrestore(&mm->area_struct_lock, sflags);
		printf("range [addr, end] usage error!\n");
		return ERR_INVALID_ARGS;
	}

	if (e == DESTROY_OP) {
		if (self->attr == GAPS || vma_is_owner_parent(self->parent)) {
			printf("cannot destroy vma attr %d, self %p, parent %p\n", self->attr,
				self, self->parent);
			spin_unlock_irqrestore(&mm->area_struct_lock, sflags);
			return ERR_INVALID_ARGS;
		}

		ret = destroy_vma(self);
		spin_unlock_irqrestore(&mm->area_struct_lock, sflags);
		return ret;
	}

	if (e == RELINQUISH_OP) {
		if (atomic_read(&self->stat) != EA ||
		    !vma_is_owner_parent(self->parent)) {
			spin_unlock_irqrestore(&mm->area_struct_lock, sflags);
			return ERR_INVALID_ARGS;
		}

		ret = relinquish_vma(mm, addr);
		spin_unlock_irqrestore(&mm->area_struct_lock, sflags);
		return ret;
	}

	uint16_t count = parse_command_of_access_count(p);

	for (uint16_t i = 0; i < count; i++) {
		struct mm_struct *borrower;
		uint16_t vmid;
		struct area_flags aflags;

		parse_command_of_access(&addr, &end, &prot, &borrower, &vmid, i,
					p);

		assert(borrower);

		aflags.word = flags;
		aflags.mid_flags = vmid;
		flags = aflags.word;
		spin_lock_irqsave(&g_vm_area_lock, gflags);
		//  area dispatch
		ret = self->vtbl->dispatch(borrower, self, e, addr, end, attr,
					   flags, prot);
		if (ret) {
			spin_unlock_irqrestore(&g_vm_area_lock, gflags);
			spin_unlock_irqrestore(&mm->area_struct_lock, sflags);
			return ret;
		}
		spin_unlock_irqrestore(&g_vm_area_lock, gflags);
	}

	spin_unlock_irqrestore(&mm->area_struct_lock, sflags);
	return ret;
}

status_t mm_struct_donate(struct vm_area_struct *lender,
			  struct mm_struct *borrower, word_t addr, word_t end,
			  word_t attr, word_t flags, word_t prot)
{
	return donate_vma(lender, borrower, addr, end, attr, flags, prot);
}
status_t mm_struct_lend(struct vm_area_struct *lender,
			struct mm_struct *borrower, word_t addr, word_t end,
			word_t flags, word_t prot)
{
	return lend_vma(lender, borrower, addr, end, flags, prot);
}
status_t mm_struct_share(struct vm_area_struct *lender,
			 struct mm_struct *borrower, word_t addr, word_t end,
			 word_t flags, word_t prot)
{
	return share_vma(lender, borrower, addr, end, flags, prot);
}

status_t mm_struct_relinquish(word_t addr, struct mm_struct *borrower)
{
	return relinquish_vma(borrower, addr);
}

void mm_struct_dump(struct mm_struct *self)
{
	printf("mm[%p] area_count[%d]\n", self, self->area_struct_cnt);
	avl_tree_walk(&self->mm_area_avl, dump_vma);
	vspace_dump(self->mm_space);
}

// vm_area_struct function
struct vm_area_struct *vm_area_struct_ctor(struct mm_struct *const mm)
{
	struct vm_area_struct *vma = kmalloc(sizeof(struct vm_area_struct));

	if (!vma)
		return NULL;

	vma->vm_area_avl.height = 1;
	vma->vm_area_avl.left = NULL;
	vma->vm_area_avl.right = NULL;

	init_list_node(&vma->vm_area_list);
	vma->prev = NULL;
	vma->next = NULL;
	vma->parent = NULL;
	vma->vtbl = &g_area_vtbl;
	reset_vdev_ops(vma);
	atomic_set(&vma->stat, NA);
	vma->attr = GAPS;
	vma->area_region.addr = 0;
	vma->area_region.apid = 0;
	vma->area_end = 0;
	vma->prot.word = 0;
	vma->flags = 0;
	atomic_set(&vma->area_users, 0);
	atomic_set(&vma->area_owners, 1);
	vma->area_mm = mm;
	return vma;
}

status_t vm_area_struct_dtor(struct vm_area_struct *vma)
{
	struct mmio_vdev *vdev;

	vdev = &vma->vdev_ops;
	if (!vma)
		return ERR_INVALID_ARGS;
	if (vma->attr == MMIO) {
		if (get_init_flags(vma->flags) && vdev->deinit)
			vdev->deinit(vma);
		// OR
		else if (vdev->suspend)
			vdev->suspend(vma);
	}

	kfree(vma);
	return NO_ERROR;
}

status_t vm_area_struct_init(struct vm_area_struct *const self, word_t addr,
			     word_t end, word_t attr, word_t flags)
{
	if (!self)
		return ERR_INVALID_ARGS;
	self->area_region.addr = addr;
	self->area_end = end;
	self->attr = attr;
	self->flags = flags;
	return NO_ERROR;
}

/* prot is owned by borrower */
status_t vm_area_struct_dispatch(struct mm_struct *borrower,
				 struct vm_area_struct *const self,
				 area_evt_t e, word_t addr, word_t end,
				 word_t attr, word_t flags, word_t prot)
{
	status_t ret;

	switch (atomic_read(&self->stat)) {
	case EA:
		if (e == DONATE_OP) {
			ret = self->area_mm->mm_vtbl->donate(
				self, borrower, addr, end, attr, flags, prot);
			return ret;
		}
		if (e == LEND_OP) {
			atomic_set(&self->stat, LA);
			ret = self->area_mm->mm_vtbl->lend(self, borrower, addr,
							   end, flags, prot);
			return ret;
		}
		if (e == SHARE_OP) {
			atomic_set(&self->stat, SA);
			ret = self->area_mm->mm_vtbl->share(
				self, borrower, addr, end, flags, prot);
			return ret;
		}
		break;
	case SA:
		if (e != SHARE_OP)
			break;

		atomic_set(&self->stat, SA);
		ret = self->area_mm->mm_vtbl->share(self, borrower, addr, end,
						    flags, prot);
		return ret;
	case LA:
		if (e != LEND_OP)
			break;
		atomic_set(&self->stat, LA);
		ret = self->area_mm->mm_vtbl->lend(self, borrower, addr, end,
						   flags, prot);
		return ret;
	default:
		printf("[%s] error op type!\n", __func__);
		break;
	}
	return ERR_INVALID_ARGS;
}

paddr_t vm_area_struct_alloc(struct vm_area_struct *const self, word_t type)
{
	int gfp_flags;
	paddr_t paddr = 0;
	status_t ret = -1;

	word_t nr_pages = (self->area_end - self->area_region.addr) / PAGE_SIZE;
	struct pm_node_struct *pn = get_recent_pn();

	printf("[%lx, %lx] npages %lx\n", self->area_region.addr,
		 self->area_end, nr_pages);

	switch (self->attr) {
	case NORMAL:
		if (type)
			gfp_flags = FUNC_RESERVED;
		else
			gfp_flags = FUNC_SYSCALL_CONT;

		ret = pn->vtbl->alloc(pn, &paddr, nr_pages, gfp_flags);
		break;
	case DMA:
		gfp_flags = FUNC_SYSCALL_CONT;
		ret = pn->vtbl->alloc(pn, &paddr, nr_pages, gfp_flags);
		break;
	case GAPS:
		printf("[vm_area_struct] do nothing in current attr %d!\n",
			 self->attr);
		break;
	default:
		break;
	}

	if (ret)
		return 0;
	return paddr;
}

status_t vm_area_struct_free(struct vm_area_struct *const self, paddr_t paddr)
{
	status_t ret = NO_ERROR;
	struct pm_node_struct *pn = get_recent_pn();

	switch (self->attr) {
	case NORMAL:
	case DMA:
		ret = pn->vtbl->free(paddr);
		break;
	case MMIO:
	case GAPS:
		printf("[vm_area_struct] do nothing in current attr %d!\n",
			 self->attr);
		break;
	default:
		printf("[vm_area_struct] has error attr!\n");
		break;
	}

	if (ret)
		return ERR_INVALID_ARGS;
	return NO_ERROR;
}

status_t vm_area_struct_map(struct vm_area_struct *const self, paddr_t pa,
			    word_t prot)
{
	word_t vaddr = self->area_region.addr;
	word_t nr_pages = (self->area_end - self->area_region.addr) / PAGE_SIZE;
	struct map_flags flags;
	status_t ret;
	struct vspace  *mm_space;

	printf("vaddr %lx, npages %lx\n", vaddr, nr_pages);
	prot_to_map_flags(&flags, prot);
	mm_space = self->area_mm->mm_space;

	ret = mm_space->vtbl->map(mm_space, paddr_to_pfn(pa), vaddr,
					  nr_pages, flags.word);
	return ret;
}

status_t vm_area_struct_unmap(struct vm_area_struct *const self)
{
	word_t vaddr = self->area_region.addr;
	word_t nr_pages = (self->area_end - self->area_region.addr) / PAGE_SIZE;
	status_t ret;
	struct vspace  *mm_space;

	mm_space = self->area_mm->mm_space;

	printf("[mm_space %p vaddr %lx nr_pages %ld]\n", mm_space, vaddr,
		 nr_pages);

	ret = mm_space->vtbl->unmap(mm_space, vaddr, nr_pages);

	return ret;
}

status_t vm_area_struct_grant(struct vm_area_struct *const self, paddr_t pa,
			      word_t prot)
{
	word_t vaddr = self->area_region.addr;
	word_t nr_pages = (self->area_end - self->area_region.addr) / PAGE_SIZE;
	struct map_flags flags;
	status_t ret;
	struct vspace  *mm_space;

	mm_space = self->area_mm->mm_space;

	prot_to_map_flags(&flags, prot);
	ret = mm_space->vtbl->grant(mm_space, paddr_to_pfn(pa), vaddr, nr_pages,
				    flags.word);

	return ret;
}
status_t vm_area_struct_protect(struct vm_area_struct *const self, word_t prot)
{
	word_t vaddr = self->area_region.addr;
	word_t nr_pages = (self->area_end - self->area_region.addr) / PAGE_SIZE;
	struct map_flags flags;
	status_t ret;
	struct vspace  *mm_space;

	mm_space = self->area_mm->mm_space;

	prot_to_map_flags(&flags, prot);
	ret = mm_space->vtbl->protect(mm_space, vaddr, nr_pages, flags.word);

	return ret;
}


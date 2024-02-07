#pragma once

#include <sys/types.h>
#include <stdlib.h>
#include <kern/list.h>
#include <lib/avl.h>
#include <kernel/spinlock.h>
#include <arch/atomic.h>
#include <kern/bitops.h>

#include <kern/err.h>

#include "vspace.h"

struct vm_area_struct;
struct mm_struct;

// struct
struct ar_region {
	word_t apid : PAGE_SIZE_SHIFT;
#ifdef IS_64BIT		
	word_t addr : 64 - PAGE_SIZE_SHIFT;
#else
	word_t addr : 32 - PAGE_SIZE_SHIFT;
#endif
};

enum area_op_type {
	CREATE_ROOT_OP = 0,
	CREATE_ROOT_MAP_OP,
	CREATE_OP,
	CREATE_MAP_OP,
	DESTROY_OP,
	DONATE_OP,
	LEND_OP,
	SHARE_OP,
	RELINQUISH_OP,
	// SHEMEM_OP,
	NR_OP_TYPE
};

enum aspace_type {
	ttbr0_hyper,  /* virtual memory - TTBR0_EL2(Non-secure)  */
	vttbr0_guest, /* ipa memory     - VTTBR0_EL2(Non-secure) */
};

typedef enum area_op_type area_evt_t;

enum area_state { NA = 0, EA, SA, LA, NR_AREA_STATE };
enum area_mm_type {
	GAPS = 0, // backboard for vma split, the root_vma usual be GAPS
	NORMAL, // buddy-normal
	DMA, // buddy-dma
	// SHARE, // buddy-share(the part is out of hypervisor vspace, shared by any
	// guest or app)
	// CACHE, // Cache1/2 I/D
	// GMM, // only address space, no buddy
	MMIO_EMUL,
	MMIO, // only address space, no buddy
	/* many devices */ NR_AERA_TYPE
}; // MMIOs(include MMIO) have many device types

union area_prot {
	u64 word;
	struct {
		u16 mem_attr; // memory attribute, include security, type, cacheability,
			// shareability

		u8 mem_ap; // access permissions, include data access, instruction access
		u8 res0; // MBZ
	};
};

struct map_flags {
	union {
		struct {
			uint64_t can_read : 1, can_write : 1, can_exec : 1,
				is_io_map : 1, mem_attr : 3, is_ns : 1,
				is_usr : 1, is_large : 1, is_vio : 1,
				reserved : 53;
		};

		uint64_t word;
	};
};

/*
 *struct dispatcher_callpara {
 *	word_t addr;
 *	word_t end;
 *	word_t attr;
 *	word_t flags;
 *	word_t prot;
 *	word_t dpa;
 *	word_t baddr;
 *	word_t bend;
 *	struct mm_struct *borrower;
 *};
 */

struct shemem_op {
	void *(*she_get)(size_t size);
	void (*she_release)(void *obj);
	int (*she_copyto)(struct mm_struct *cur_mm_struct, void *dest,
			  void *src, size_t size);
	int (*she_copyfrom)(struct mm_struct *cur_mm_struct, void *dest,
			    void *src, size_t size);
	int (*she_clear)(void *dest, size_t size);
};

struct vm_area_struct_vtable {
	// owner
	// ctor & dtor
	struct vm_area_struct *(*ctor)(struct mm_struct *const mm);
	status_t (*dtor)(struct vm_area_struct *self);
	// VA,IPA
	status_t (*init)(struct vm_area_struct *const self, word_t addr,
			 word_t end, word_t attr, word_t flags);
	status_t (*dispatch)(struct mm_struct *mm,
			     struct vm_area_struct *const self, area_evt_t e,
			     word_t addr, word_t end, word_t attr, word_t flags,
			     word_t prot); // area_state FSM
	// dispatch PA

	paddr_t (*alloc)(struct vm_area_struct *const self,
			 word_t type); // self contains nr_pages Owner-NA
	status_t (*free)(struct vm_area_struct *const self,
			 paddr_t paddr); // self contains address  !Owner-NA

	// VA-PA, VA-IPA
	// IPA-PA
	status_t (*map)(struct vm_area_struct *const self, paddr_t pa,
			word_t prot); // Owner-EA
	status_t (*unmap)(struct vm_area_struct *const self); // Owner-NA
	status_t (*grant)(struct vm_area_struct *const self, paddr_t pa,
			  word_t prot); // Owner-LA - !Owner-EA/SA
	// Owner-SA - !Owner-SA
	status_t (*protect)(struct vm_area_struct *const self,
			    word_t prot); // TBD
	status_t (*query)(struct vm_area_struct *self, paddr_t *paddr,
			  word_t *mmu_flags);
};

struct mm_struct_vtable {
	// ctor & dtor
	struct mm_struct *(*ctor)(bool g);
	status_t (*dtor)(struct mm_struct *self);
	status_t (*dispatch)(struct mm_struct *mm, area_evt_t e, void *p);
	// owner
	status_t (*donate)(struct vm_area_struct *lender,
			   struct mm_struct *borrower, word_t addr, word_t end,
			   word_t attr, word_t flags, word_t prot);
	// access
	status_t (*lend)(struct vm_area_struct *lender,
			 struct mm_struct *borrower, word_t addr, word_t end,
			 word_t flags, word_t prot);
	status_t (*share)(struct vm_area_struct *lender,
			  struct mm_struct *borrower, word_t addr, word_t end,
			  word_t flags, word_t prot);
	status_t (*relinquish)(word_t addr, struct mm_struct *borrower);
	status_t (*query)(struct mm_struct *mm, vaddr_t va_ipa, paddr_t *paddr,
			  word_t *mmu_flags);

	void (*dump)(struct mm_struct *self);
};

struct area_flags {
	union {
		word_t word;
		struct {
			word_t g_flags : 1; /* 0:non-global mm, 1:global mm */
			word_t init_flags : 1; /* 0: non-init vm_area, 1:init vm_area */
			word_t vs_flags : 3; /* support vspace type : 8 */
			word_t mid_flags : 16; /* support 256 mm id */
			word_t res_flags : 1;
			word_t alloc_flags : 1;
#ifdef IS_64BIT			
			word_t mbz : 64-23;
#else
			word_t mbz : 32-23;
#endif
		};
	};
};

#define AREA_FLAGS_GUEST(vmid) BM(2, 3, vttbr0_guest) | BM(5, 16, (vmid))
#define AREA_FLAGS_HYPER(vmid)                                                 \
	BM(0, 1, 1) | BM(2, 3, ttbr0_hyper) | BM(5, 16, (vmid))

struct mmio_vdev {
	void *base; // hyper-va
	int (*read)(void *vcpu, uint64_t addr, uint64_t *value,
		    void *maccess);
	int (*write)(void *vcpu, uint64_t addr, uint64_t value,
		     void *maccess);
	int (*deinit)(struct vm_area_struct *const vdev);
	int (*reset)(struct vm_area_struct *const vdev);
	int (*suspend)(struct vm_area_struct *const vdev);
	int (*resume)(struct vm_area_struct *const vdev);
};

/* Make sure that the area corresponding to vdev of each mm_struct (the user
 * virtual address starts), the area corresponding to dram, and the area
 * corresponding to share are isolated from each other, so that (prev, next) can
 * point to all areas in the same family
 */

struct vm_area_struct {
	avl_node_t vm_area_avl;
	// struct rb_node vm_area_rb;
	struct list_node vm_area_list;
	struct vm_area_struct *prev; // owner double list
	struct vm_area_struct *next;
	struct vm_area_struct
		*parent; // owner: parent node of some attrs; access:
	// the owner of lent or shared
	const struct vm_area_struct_vtable *vtbl;
	// spinlock prot
	atomic_t stat; // EA,SA <=> is_mapping, NA,LA <=> no_mapping
	enum area_mm_type attr; // NORMAL(LinearA/MAP/PhyicsA), DMA(LA/SMAP/PA),
		// MMIO(LA/MAP-delay), GAPS(LA),
		// GMM(LinearA/MAP/PhyicsA) SHARE(LinearA/MAP/PhyicsA)
	struct ar_region area_region; // LA
	vaddr_t area_end; // NR_PAGES

	union area_prot prot;
	word_t flags; // area usage flags

	atomic_t area_users; // access: shared or lent users
	atomic_t area_owners; // owner: include donated user

	struct mmio_vdev vdev_ops;

	struct mm_struct *area_mm; // area owner, donater is another choose
		// spinlock prot
}; // __randomize_layout

/* Ensure the uniqueness of each data structure (mm_struct/vm_area_struct),
 * that is, the data structure cannot be shared by multiple vm/thread/process,
 * and even if a certain data structure of vmA is logically shared with
 * multiple vms, modifying the shared information of vmA is still done by vmA
 * is responsible; when vmA needs to share the information of a data structure
 * with other vms, it needs to copy a new data structure that contains part of
 * the shared information, and other vms can only use, copy, and delete this
 * part of the information, but it cannot be updated
 */ // thread vs process vs vm (mm_struct cannot have many owners)
struct mm_struct {
	// spinlock prot
	avl_root_t mm_area_avl;
	// struct rb_root mm_area_rb;
	struct list_node mm_area_list; // access double list
	struct vm_area_struct *mm_area_q; // owner double list
	const struct mm_struct_vtable *mm_vtbl;
	const struct vm_area_struct_vtable *area_vtbl;
	const struct shemem_op *shemem_ops; // traps ops
	word_t area_struct_cnt;
	spin_lock_t area_struct_lock;
	word_t mm_start; // mm start address(dram(code/data/bss/res)+mmio)
	word_t mm_end; // mm end address

	// spinlock prot
	struct vspace  *mm_space;
}; //__randomize_layout

// hypcall
union region_struct_handle {
	u64 word;
	struct {
		u64 type : 1; // PA(1) OR VA/IPA(0)
		u64 secure : 1; // secure/non-secure
		u64 usr : 1;
		u64 g : 1;
		u64 st : 3;
		u64 mbz : 57;
	};
};

// ap one vm_area_struct
struct access_prot {
	u16 ap_id; // 16-bit ID of VM that has memory access permission
	union {
		struct {
			u8 mem_ap; // access permissions, include data access, instruction access
				// <union area_prot>
			u8 flags; // api usage MBZ
		}; /* data */
		uint16_t word;
	};

	u32 range; //
	u64 handle; // offset of vm_area_struct, for owner mm_struct
		// one area
};

// owner with one mm_struct to transact manys vm_area_structs
struct prot_transaction {
	u16 owner_id; // 16-bit ID of VM that has memory owner
	// u16 mem_attr; // memory attribute, include security, type, cacheability,
	// shareability <union area_prot>
	u32 flags; // api usage, include mem_attr
	u64 handle; // ap_vmid handle(region_struct_handle)
	u64 badge;
	u64 range; // vma range

	u16 access_prot_cnt;

	struct access_prot access_prot_array[];
};

static inline void owner_region_handle(union region_struct_handle *handle,
				       vaddr_t addr, bool type, bool secure,
				       bool usr, bool g, uint8_t st)
{
	handle->word = ROUNDDOWN(addr, PAGE_SIZE);
	handle->type = type;
	handle->secure = secure;
	handle->usr = usr;
	handle->g = g;
	handle->st = st;
}

static inline void guest_prot_transaction(struct prot_transaction *trans,
					  uint16_t vmid, vaddr_t vir,
					  paddr_t phy, size_t size,
					  uint32_t flags)
{
	union region_struct_handle handle;

	owner_region_handle(&handle, vir, 0, 0, 0, 0, vttbr0_guest);
	trans->owner_id = vmid;
	trans->flags = flags;
	trans->handle = handle.word;
	trans->range = size;
	trans->badge = phy;
	trans->access_prot_cnt = 0;
}

static inline void guest_prot_transaction_secure(struct prot_transaction *trans,
						 uint16_t vmid, vaddr_t vir,
						 paddr_t phy, size_t size,
						 uint32_t flags)
{
	union region_struct_handle handle;

	owner_region_handle(&handle, vir, 0, 1, 0, 0, vttbr0_guest);
	trans->owner_id = vmid;
	trans->flags = flags;
	trans->handle = handle.word;
	trans->range = size;
	trans->badge = phy;
	trans->access_prot_cnt = 0;
}

static inline void hyper_prot_transaction(struct prot_transaction *trans,
					  uint16_t vmid, vaddr_t vir,
					  paddr_t phy, size_t size,
					  uint32_t flags)
{
	union region_struct_handle handle;

	owner_region_handle(&handle, vir, 0, 0, 0, 1, ttbr0_hyper);
	trans->owner_id = vmid;
	trans->flags = flags;
	trans->handle = handle.word;
	trans->range = size;
	trans->badge = phy;
	trans->access_prot_cnt = 0;
}

static inline void hyper_prot_transaction_secure(struct prot_transaction *trans,
						 uint16_t vmid, vaddr_t vir,
						 paddr_t phy, size_t size,
						 uint32_t flags)
{
	union region_struct_handle handle;

	owner_region_handle(&handle, vir, 0, 1, 0, 1, ttbr0_hyper);
	trans->owner_id = vmid;
	trans->flags = flags;
	trans->handle = handle.word;
	trans->range = size;
	trans->badge = phy;
	trans->access_prot_cnt = 0;
}

static inline void guest_access_transaction(struct prot_transaction *trans,
					    uint16_t vmid, vaddr_t vir,
					    size_t size, uint16_t flags)
{
	struct access_prot prot;
	union region_struct_handle handle;

	owner_region_handle(&handle, vir, 0, 0, 0, 0, vttbr0_guest);
	prot.ap_id = vmid;
	prot.word = flags;
	prot.handle = handle.word;
	prot.range = size;
	trans->access_prot_array[trans->access_prot_cnt] = prot;
	trans->access_prot_cnt++;
}

static inline void
guest_access_transaction_secure(struct prot_transaction *trans, uint16_t vmid,
				vaddr_t vir, size_t size, uint16_t flags)
{
	struct access_prot prot;
	union region_struct_handle handle;

	owner_region_handle(&handle, vir, 0, 1, 0, 0, vttbr0_guest);
	prot.ap_id = vmid;
	prot.word = flags;
	prot.handle = handle.word;
	prot.range = size;
	trans->access_prot_array[trans->access_prot_cnt] = prot;
	trans->access_prot_cnt++;
}

union vma_api_usage_flags {
	struct {
		u32 r : 1;
		u32 w : 1;
		u32 x : 1;
		u32 large : 1;
		u32 vio : 1;
		u32 res : 1;
		u32 mbz : 16;
		u32 io : 1;
		u32 init : 1;
		u32 mem_attr : 4;
		u32 vma_attr : 4;
	};
	u32 word;
};

#define VMA_RIGHT_R BIT(0)
#define VMA_RIGHT_W BIT(1)
#define VMA_RIGHT_X BIT(2)
#define VMA_SIZE_LARGE BIT(3)
#define VMA_SIZE_VIO BIT(4)
/* only read, canot execu */
#define VMA_RIGHT_RO BM(0, 3, 1)
/* only read, can execu */
#define VMA_RIGHT_RX BM(0, 3, 5)
/* read and write, but canot execu */
#define VMA_RIGHT_RW BM(0, 3, 3)
#define VMA_RIGHT_RWX BM(0, 3, 7)
/* vma type */
#define VMA_RES BM(5, 1, 1)
#define VMA_MMIO (BM(28, 4, MMIO) | BM(24, 4, MAP_IO_DEFAULT) | BM(22, 1, 1))
#define VMA_MMIO_EMUL BM(28, 4, MMIO_EMUL)
#define VMA_MMIO_EMUL_INIT (BM(28, 4, MMIO_EMUL) | BM(23, 1, 1))
#define VMA_NORMAL (BM(28, 4, NORMAL) | BM(24, 4, MAP_NORMAL))
#define VMA_DMA (BM(28, 4, DMA) | BM(24, 4, MAP_NC))
#define VMA_SHARE_NORMAL (BM(28, 4, SHARE) | BM(24, 4, MAP_NORMAL))
#define VMA_SHARE_MMIO                                                         \
	(BM(28, 4, SHARE) | BM(24, 4, MAP_IO_DEFAULT) | BM(22, 1, 1))
#define VMA_GMM (BM(28, 4, GMM) | BM(22, 1, 1))

#define VMA_FLAGS_NORMAL (VMA_NORMAL | VMA_RIGHT_RW)
#define VMA_FLAGS_PAYLOAD (VMA_NORMAL | VMA_RIGHT_RWX)
#define VMA_FLAGS_PAYLOAD_RESERVED                                             \
	(VMA_NORMAL | VMA_RIGHT_RWX | VMA_RES) //VMA_SIZE_LARGE
#define VMA_FLAGS_DMA (VMA_DMA | VMA_RIGHT_RW)
#define VMA_FLAGS_SHARE (VMA_SHARE_NORMAL | VMA_RIGHT_RW)
#define VMA_FLAGS_SHARE_IO (VMA_SHARE_MMIO | VMA_RIGHT_RW)
#define VMA_FLAGS_GMM (VMA_GMM)
#define VMA_FLAGS_MMIO (VMA_MMIO | VMA_RIGHT_RW)
#define VMA_FLAGS_MMIO_EMUL (VMA_MMIO_EMUL)
#ifdef CONFIG_HYPER_SUPPORT_VIRTIO_BLK_HYPERLESS
#define VMA_FLAGS_MMIO_VIRTIO (VMA_NORMAL | VMA_RIGHT_RW | VMA_SIZE_VIO)
#else
#define VMA_FLAGS_MMIO_VIRTIO (VMA_NORMAL | VMA_RIGHT_R | VMA_SIZE_VIO)
#endif

#define VMA_FLAGS_MMIO_EMUL_INIT (VMA_MMIO_EMUL_INIT)
#define VMA_FLAGS_MMIO_SHMEM (VMA_NORMAL | VMA_RIGHT_R)

#define IPA_BOUND_PADDR 0xffffffffffffffff
#define IPA_OMAP_PADDR (IPA_BOUND_PADDR - 1)

static inline bool ipa_pa_boundary(paddr_t pa)
{
	return pa == IPA_BOUND_PADDR;
}

static inline bool ipa_pa_omap(paddr_t pa)
{
	return pa == IPA_OMAP_PADDR;
}

static inline bool vma_valid(void *vma)
{
	if (vma && vma != AVL_ERR_NODE)
		return true;
	return false;
}

static inline void reset_vdev_ops(struct vm_area_struct *vma)
{
	vma->vdev_ops.read = 0;
	vma->vdev_ops.write = 0;
	vma->vdev_ops.deinit = 0;
	vma->vdev_ops.reset = 0;
	vma->vdev_ops.suspend = 0;
	vma->vdev_ops.resume = 0;
}

/* mm_struct contains a root node, which is the entire VA/IPA address space in
 * the root node. The root node is only a linear address space vm_area_struct,
 * and is not mapped to the physical address space. When creating a new
 * vm_area_struct, it is equivalent to creating a child node at the root node,
 * and the child node belongs to the address subset of the root node; when the
 * child node creates a new vm_area_struct, the child node is also the root
 * node, so that the root node and child nodes to form an AVL tree.
 */

/* The kinds of nodes are as follows, assign linear address only, assign linear
 * address and map, assign linear address and map and assign physical memory
 */

/* mm_struct has ownership of each node of the avl tree, and has the following
 * states, NA, EA, SA, LA; when mm_struct does not own an area, it cannot be
 * added to its own AVL tree; but if mm_struct has no ownership of this area
 * When the area has access rights, it can be added to the page table of
 * mm_struct, or deleted from the page table when the area is not accessed
 */

/* The memory type of each node is divided into: DMA, MMIO, NORMAL; for a
 * non-ownership node, it can be placed in the doubly linked list of mm_struct,
 * and the node can have the following states: NA, EA, SA
 */

// inline
static inline bool vma_is_mapping(struct vm_area_struct *vma)
{
	return atomic_read(&vma->stat) == EA || atomic_read(&vma->stat) == SA;
}

static inline bool vma_is_sharing(struct vm_area_struct *vma)
{
	return atomic_read(&vma->stat) == SA;
}

static inline bool vma_is_lent(struct vm_area_struct *vma)
{
	return atomic_read(&vma->stat) == LA;
}

static inline bool vma_is_dirty(struct vm_area_struct *vma)
{
	return atomic_read(&vma->stat) != NA;
}

static inline bool vma_is_fusing(struct vm_area_struct *vma)
{
	return atomic_read(&vma->stat) == EA || atomic_read(&vma->stat) == NA;
}

static inline bool vma_is_owner_parent(struct vm_area_struct *vma)
{
	return atomic_read(&vma->area_users) > 0;
}

static inline bool vma_prot_derived(struct vm_area_struct *vma,
				    struct vm_area_struct *friend)
{
	return vma->prot.mem_ap == friend->prot.mem_ap &&
	       vma->prot.mem_attr == friend->prot.mem_attr;
}

static inline bool vma_attr_derived(struct vm_area_struct *vma,
				    struct vm_area_struct *friend)
{
	return vma_is_fusing(vma) && vma_is_fusing(friend) &&
	       vma->attr == friend->attr;
}

static inline bool vma_users_derived(struct vm_area_struct *vma,
				     struct vm_area_struct *friend)
{
	return atomic_read(&vma->area_users) ==
		       atomic_read(&friend->area_users) &&
	       atomic_read(&vma->area_owners) ==
		       atomic_read(&friend->area_owners);
}
static inline bool vma_is_friend(struct vm_area_struct *vma,
				 struct vm_area_struct *friend)
{
	return vma_attr_derived(vma, friend) && vma_prot_derived(vma, friend);
}

static inline void insert_dlist(struct vm_area_struct *parent,
				struct vm_area_struct *child)
{
	if (parent->next) {
		child->next = parent->next;
		parent->next->prev = child;
	}

	parent->next = child;
	child->prev = parent;
}

static inline void insert_dlist_before(struct vm_area_struct *parent,
				       struct vm_area_struct *child)
{
	if (parent->prev) {
		child->prev = parent->prev;
		parent->prev->next = child;
	}

	parent->prev = child;
	child->next = parent;
}

static inline word_t vma_ipa_addr(struct vm_area_struct *vma)
{
	return vma->area_region.addr & ~MASK(PAGE_SIZE_SHIFT);
}

static inline size_t vma_size(struct vm_area_struct *vma)
{
	return (vma->area_end - vma_ipa_addr(vma));
}

/*
 * static inline void append_dlist(struct vm_area_struct *list, struct
 * vm_area_struct *vma) { struct vm_area_struct *tail = list->prev;
 *         insert_dlist(tail, vma);
 * }
 *
 * static inline void prepend_dlist(struct vm_area_struct *list, struct
 * vm_area_struct *vma) { struct vm_area_struct *head = list->next;
 *         insert_dlist_before(head, vma);
 * }
 *
 * static inline void init_dlist(struct vm_area_struct *list) {
 *         list->next = list;
 *         list->prev = list;
 * }
 */

static inline void delete_dlist(struct vm_area_struct *self)
{
	if (self->next)
		self->next->prev = self->prev;
	if (self->prev)
		self->prev->next = self->next;

	self->next = self->prev = NULL;
}

static inline bool area_is_in_list(struct vm_area_struct *self)
{
	return self->prev != NULL || self->next != NULL;
}

static inline void prot_to_map_flags(struct map_flags *flags, word_t prot)
{
	//
	flags->word = prot;
}

static inline void map_flags_to_prot(word_t map_flags, word_t *prot)
{
	//
	*prot = map_flags;
}

static inline bool valid_area_op(area_evt_t e)
{
	return e > 0 && e < NR_OP_TYPE;
}

static inline bool get_g_flags(word_t flags)
{
	struct area_flags aflags;

	aflags.word = flags;
	return aflags.g_flags;
}

static inline void set_g_flags(struct area_flags *flags, bool g)
{
	flags->g_flags = g;
}

static inline uint16_t get_mid_flags(word_t flags)
{
	struct area_flags aflags;

	aflags.word = flags;
	return aflags.mid_flags;
}

static inline void set_mid_flags(struct area_flags *flags, uint16_t mid)
{
	flags->mid_flags = mid;
}

static inline bool get_init_flags(word_t flags)
{
	struct area_flags aflags;

	aflags.word = flags;
	return aflags.init_flags;
}

static inline void set_init_flags(struct area_flags *flags, bool i)
{
	flags->init_flags = i;
}

static inline bool get_alloc_flags(word_t flags)
{
	struct area_flags aflags;

	aflags.word = flags;
	return aflags.alloc_flags;
}

static inline void set_alloc_flags(struct area_flags *flags, bool i)
{
	flags->alloc_flags = i;
}

static inline uint8_t get_vs_flags(word_t flags)
{
	struct area_flags aflags;

	aflags.word = flags;
	return aflags.vs_flags;
}

static inline void set_vs_flags(struct area_flags *flags, uint8_t type)
{
	flags->vs_flags = type;
}

static inline bool get_res_flags(word_t flags)
{
	struct area_flags aflags;

	aflags.word = flags;
	return aflags.res_flags;
}

static inline void set_res_flags(struct area_flags *flags, bool res)
{
	flags->res_flags = res;
}

static inline void set_mm_struct_flags(struct area_flags *flags, bool g,
				       uint16_t mid, bool i, uint8_t type,
				       bool res)
{
	flags->g_flags = g;
	flags->init_flags = i;
	flags->mid_flags = mid;
	flags->vs_flags = type;
	flags->res_flags = res;
	flags->alloc_flags = false;
}

static inline bool out_of_mm_bound(struct mm_struct *mm, word_t addr,
				   word_t end)
{
	return addr < mm->mm_start || end > mm->mm_end;
}

/*
 * static inline struct mm_struct *get_cur_mm_struct(void)
 * {
 *	extern struct mm_struct *cur_mm_struct;
 *
 *	return cur_mm_struct;
 * }
 * static inline void set_mm_struct_para(struct dispatcher_callpara *para,
 *				      word_t addr, word_t end, word_t attr,
 *				      word_t flags, word_t prot)
 * {
 *	para->addr = addr;
 *	para->end = end;
 *	para->attr = attr;
 *	para->flags = flags;
 *	para->prot = prot;
 * }
 *
 * static inline void set_mm_struct_para_extra(struct dispatcher_callpara *para,
 *					    word_t dpa, word_t baddr,
 *					    word_t bend,
 *					    struct mm_struct *borrower)
 * {
 *	para->dpa = dpa;
 *	para->baddr = baddr;
 *	para->bend = bend;
 *	para->borrower = borrower;
 * }
 */

static inline void set_mm_struct_mflags(struct map_flags *flags, bool r, bool w,
					bool e, bool io, uint8_t mem_attr,
					bool ns, bool usr, bool large, bool vio)
{
	flags->can_read = r;
	flags->can_write = w;
	flags->can_exec = e;
	flags->is_io_map = io;
	flags->mem_attr = mem_attr;
	flags->is_ns = ns;
	flags->is_usr = usr;
	flags->is_large = large;
	flags->is_vio = vio;
	flags->reserved = 0;
}

static inline word_t update_mm_struct_mflags(word_t flags, bool r, bool w,
					     bool e)
{
	struct map_flags mflags;

	mflags.word = flags;

	mflags.can_read = r;
	mflags.can_write = w;
	mflags.can_exec = e;
	return mflags.word;
}
struct mm_struct *init_mm_struct(word_t addr, word_t end, word_t attr,
				 word_t flags);

status_t deinit_mm_struct(struct mm_struct *mm);

void *find_vma_intersection(struct mm_struct *mm, word_t start_addr,
			    word_t end_addr, size_t size);

void *get_unmapped_area(struct mm_struct *mm, word_t addr, size_t size);

void *get_mapped_area(struct mm_struct *mm, word_t addr, size_t size);

void *find_vma_locked(struct mm_struct *mm, word_t addr, word_t end);





status_t create_root_vma(struct mm_struct **mm_struct, word_t addr, word_t end,
			 word_t attr, word_t flags);

struct mm_struct *mm_struct_ctor(bool g);

status_t mm_struct_dtor(struct mm_struct *mm);

status_t mm_struct_dispatch(struct mm_struct *mm, area_evt_t e, void *p);

status_t mm_struct_donate(struct vm_area_struct *lender,
			  struct mm_struct *borrower, word_t addr, word_t end,
			  word_t attr, word_t flags, word_t prot);

status_t mm_struct_lend(struct vm_area_struct *lender,
			struct mm_struct *borrower, word_t addr, word_t end,
			word_t flags, word_t prot);

status_t mm_struct_share(struct vm_area_struct *lender,
			 struct mm_struct *borrower, word_t addr, word_t end,
			 word_t flags, word_t prot);

status_t mm_struct_relinquish(word_t addr, struct mm_struct *borrower);

void mm_struct_dump(struct mm_struct *self);

struct vm_area_struct *vm_area_struct_ctor(struct mm_struct *const mm);

status_t vm_area_struct_dtor(struct vm_area_struct *vma);

status_t vm_area_struct_init(struct vm_area_struct *const self, word_t addr,
			     word_t end, word_t attr, word_t flags);

status_t vm_area_struct_dispatch(struct mm_struct *borrower,
				 struct vm_area_struct *const self,
				 area_evt_t e, word_t addr, word_t end,
				 word_t attr, word_t flags, word_t prot);

paddr_t vm_area_struct_alloc(struct vm_area_struct *const self, word_t type);

status_t vm_area_struct_free(struct vm_area_struct *const self, paddr_t paddr);

status_t vm_area_struct_map(struct vm_area_struct *const self, paddr_t pa,
			    word_t prot);

status_t vm_area_struct_unmap(struct vm_area_struct *const self);

status_t vm_area_struct_grant(struct vm_area_struct *const self, paddr_t pa,
			      word_t prot);
status_t vm_area_struct_protect(struct vm_area_struct *const self, word_t prot);

status_t query_vma(struct vm_area_struct *self, paddr_t *paddr,
		   word_t *mmu_flags);

status_t delete_vma(struct mm_struct *mm, struct vm_area_struct *self);

status_t destroy_vma_mapping(struct vm_area_struct *self);

status_t revoke_delete_vma(struct mm_struct *mm, struct vm_area_struct *self);

struct vm_area_struct *expand_vma(struct mm_struct *borrower, paddr_t paddr,
				  word_t addr, word_t end, word_t attr,
				  word_t flags, word_t prot);

status_t query_va_ipa(struct mm_struct *mm, vaddr_t va_ipa, paddr_t *paddr,
		      word_t *mmu_flags);


#include "pa.h"

#include <kern/err.h>
#include <kernel/spinlock.h>

/* bootmem
 * preparer
 * alloc
 * pcp
 * borrow(node/table)
 * cma
 * reclaim(swap from allocated to others)
 * oom(swap from one task owned pages to others)
 * free
 * compact(migrate, free memory shred)
 */

// one node
#ifndef PNODE_NUM
struct pm_area_struct g_pm_area_struct[MAX_RECURSION_CNTS];
struct pm_node_struct g_pm_node_struct[1];
struct vmm_page *g_vm_pages;
struct vmm_page *g_vm_pages_end;
void *g_vm_pages_of;
int g_area_lvl;
int g_area_order;
#else
struct pm_area_struct g_pm_area_struct[PNODE_NUM][MAX_RECURSION_CNTS];
struct pm_node_struct g_pm_node_struct[PNODE_NUM];
struct vmm_page *g_vm_pages[PNODE_NUM];
struct vmm_page *g_vm_pages_end[PNODE_NUM];
void *g_vm_pages_of[PNODE_NUM];
int g_area_lvl[PNODE_NUM];
int g_area_order[PNODE_NUM];
#endif

static status_t pm_area_struct_compound(struct vmm_page **hp, word_t npages,
					int migration_type, bool inlay);
static status_t pm_area_struct_decompose(struct vmm_page *hp);
static status_t pm_area_struct_dump_area(struct pm_area_struct *const self);
static status_t pm_node_struct_migrate(struct pm_node_struct *const self,
				       struct vmm_page **hp, int order, int lvl,
				       int type, bool inlay);
static status_t pm_node_struct_dispatch(struct pm_node_struct *const self,
					struct vmm_page **hp, int lvl,
					int migration_type, int order,
					bool inlay);
static status_t pm_node_struct_init(struct pm_node_struct *const self);
static status_t pm_node_struct_lend(struct vmm_page **hp, int migration_type,
				    int lvl, int lvl_order, bool inlay);
static status_t pm_node_struct_alloc(struct pm_node_struct *pn, paddr_t *paddr,
				     word_t npages, int flags);
static status_t pm_node_struct_free(paddr_t paddr);
static status_t pm_node_struct_dump_node(struct pm_node_struct *const self);
static void pm_area_struct_dump_migration(struct pm_area_struct *const self,
					  int type);
struct pm_area_struct_vtable g_area_struct_vtbl = {
	.compound = pm_area_struct_compound,
	.decompose = pm_area_struct_decompose,
	.dump_area = pm_area_struct_dump_area,
};

struct pm_node_struct_vtable g_node_vtbl = {
	.migrate = pm_node_struct_migrate,
	.dispatch = pm_node_struct_dispatch,
	.init = pm_node_struct_init,
	.lend = pm_node_struct_lend,
	.alloc = pm_node_struct_alloc,
	.free = pm_node_struct_free,
	.dump_node = pm_node_struct_dump_node,
};

/*
 * Locate the struct page for both the matching buddy in our
 * pair (buddy1) and the combined O(n+1) page they form (page).
 *
 * 1) Any buddy B1 will have an order O twin B2 which satisfies
 * the following equation:
 *     B2 = B1 ^ (1 << O)
 * For example, if the starting buddy (buddy2) is #8 its order
 * 1 buddy is #10:
 *     B2 = 8 ^ (1 << 1) = 8 ^ 2 = 10
 *
 * 2) Any buddy B will have an order O+1 parent P which
 * satisfies the following equation:
 *     P = B & ~(1 << O)
 *
 * Assumption: *vmm_page is contiguous at least up to MAX_ORDER
 */
/* relative order */
void split2_area_migration(struct pm_area_struct *pma, struct vmm_page *page,
			   int size, int order_max, int migration_type, int lvl)
{
	struct vmm_page *present;
	struct list_node *temp;

	present = &page[size];
	temp = &AREA_FREELIST(pma, migration_type)[order_max];

	list_add_head(&present->node, temp);
	clear_phys_present(present);
	set_page_order(present, order_max);
	clear_page_state(present);
	set_page_node(present, get_recent_nu());
	set_page_section(present, lvl);
	AREA_FREECOUNT(pma, migration_type)
	[order_max]++;
	/* according to paddr, can find which node or area */
}

static void split_area_migration(struct pm_area_struct *pma,
				 struct vmm_page *page, int lvl, int order_min,
				 int order_max, int migration_type)
{
	int block_shift = PM_LX_X(PAGE_SIZE_SHIFT, lvl);

	while (order_max > order_min) {
		order_max--; /* size >>= 1; */
		split2_area_migration(pma, page, BIT(order_max + block_shift),
				      order_max, migration_type, lvl);
	}

	set_phys_present(page);
	set_page_order(page, order_max);
	set_page_node(page, get_recent_nu());
	set_page_section(page, lvl);
}

struct vmm_page *remove_area_migration(struct pm_area_struct *pma, int lvl,
				      int order, int migration_type)
{
	struct vmm_page *page;
	word_t order_index;
	struct list_node *temp;
	int block_shift;
	int max_order;
	long page_pfn = -1;

#ifndef PNODE_NUM
	max_order = (lvl == g_area_lvl) ? g_area_order : PAGE_SIZE_SHIFT;
#else
	max_order = (lvl == g_area_lvl[get_recent_nu()]) ?
			    g_area_order[get_recent_nu()] :
			    PAGE_SIZE_SHIFT;
#endif

	for (order_index = order; order_index < max_order; order_index++) {
		temp = &AREA_FREELIST(pma, migration_type)[order_index];
		page = list_head_entry(temp, page, node);

		if (!page)
			continue;

		page_pfn = page_to_pfn(page);
		/* pfn align */
		block_shift = PM_LX_X(PAGE_SIZE_SHIFT, lvl);
		if (page_pfn & MASK(block_shift))
			goto bad_align;

		assert(!page_is_present(page));
		list_del(&page->node);
		AREA_FREECOUNT(pma, migration_type)
		[order_index]--;
		split_area_migration(pma, page, lvl, order, order_index,
				     migration_type);

		return page;
	}

bad_align: // means the current lvl is not prepared
	printf("page %d order %d bad align by level %d of migration_type %d of npages %d!\n",
		page_pfn, order, lvl, migration_type,
		node_budget(migration_type));

	return NULL;
}

static inline struct vmm_page *find_index_page(struct pm_area_struct *pma,
					      int order, int index,
					      int migration_type)
{
	struct vmm_page *page;
	struct list_node *temp;
	int i = 0;

	temp = &AREA_FREELIST(pma, migration_type)[order];
	// coverity[dead_error_line:SUPPRESS]
	list_for_each_entry_list(temp, page, node) {
		if (i == index)
			return page;
		++i;
	}
	return NULL;
}

static inline void compound_append(struct vmm_page *node, struct vmm_page *head)
{
	struct vmm_page *temp;

	temp = head->compound_head;
	if (!temp) {
		head->compound_head = node;
		return;
	}
	while (temp->compound_node)
		temp = temp->compound_node;
	temp->compound_node = node;
}

static inline struct vmm_page *compound_remove_head(struct vmm_page *head)
{
	struct vmm_page *temp;

	temp = head->compound_head;
	if (!temp)
		return NULL;
	head->compound_head = temp->compound_node;
	temp->compound_node = NULL;

	return temp;
}

static inline void freelist_to_compound(struct list_node *freelist,
					struct vmm_page *split,
					struct vmm_page *head)
{
	struct vmm_page *temp;
	struct list_node *head_list;

	// split != head of freelist
	while (!list_is_empty(freelist)) {
		head_list = list_remove_head(freelist);
		temp = list_entry_list(head_list, temp, node);
		if (split && list_next_entry(freelist, temp, node) == split) {
			compound_append(temp, head);
			break;
		}
		compound_append(temp, head);
	}
}

int enqueue_area_migration(struct pm_area_struct *pma, struct vmm_page **hp,
			   int order, int norder, int migration_type)
{
	struct vmm_page *page, *split_page;
	struct vmm_page *head = *hp;
	struct list_node *temp;
	word_t order_index, temp_count, rem_count = 2 * norder;

	for (order_index = order - 1;; order_index--) {
		temp = &AREA_FREELIST(pma, migration_type)[order_index];
		temp_count = AREA_FREECOUNT(pma, migration_type)[order_index];
		page = list_head_entry(temp, page, node);

		if (!page)
			continue;

		if (!head)
			head = page;

		if (rem_count > temp_count) {
			rem_count -= temp_count;
			rem_count *= 2;
			freelist_to_compound(temp, NULL, head);
			AREA_FREECOUNT(pma, migration_type)
			[order_index] -= temp_count;
			continue;
		} else {
			// rem_count != 0
			split_page = find_index_page(pma, order_index,
						     rem_count, migration_type);
			freelist_to_compound(temp, split_page, head);
			AREA_FREECOUNT(pma, migration_type)
			[order_index] -= rem_count;
		}

		*hp = head;
		return 0;
	}

	return rem_count;
}

static inline bool buddy_is_valid(int migration_type, long buddy_pfn,
				  struct vmm_page *buddy_page)
{
	bool valid = pfn_is_valid(buddy_pfn);
	bool in_list = list_in_list(&buddy_page->node);
	int state = get_page_state(buddy_page);
	int buddy_migration_type = page_state_to_migration(state);

	return valid && in_list && buddy_migration_type == migration_type;
}

struct vmm_page *free_area_migration(struct vmm_page *page,
				    struct pm_area_struct *pma, int lvl,
				    int order, int migration_type)
{
	long combined_pfn;
	long buddy_pfn;
	struct list_node *temp;
	struct vmm_page *buddy_page;
	word_t max_order;
	long pfn;
	int pid = get_page_node(page);
	int old_lvl = -1;
	int block_shift, abs_order;

	assert(page_is_present(page));

	block_shift = PM_LX_X(PAGE_SIZE_SHIFT, lvl);
	abs_order = order + block_shift;

	while (old_lvl != lvl) {
		old_lvl = lvl;
		pfn = page_to_rel_pfn(page, pid);
#ifndef PNODE_NUM
		max_order =
			(lvl == g_area_lvl) ? g_area_order : PAGE_SIZE_SHIFT;
#else
		max_order = (lvl == g_area_lvl[get_recent_nu()]) ?
				    g_area_order[get_recent_nu()] :
				    PAGE_SIZE_SHIFT;
#endif
		while (order <= max_order - 1) {
			clear_phys_present(page);
			// FOR buddy_is_valid
			// clear_page_state(page);

			if (migration_type == MIGRATION_PCPTYPE ||
			    (lvl == g_area_lvl && order == max_order - 1))
				goto free_page_migration_direct;

			buddy_pfn = next_buddy_pfn(pfn, abs_order);
			buddy_page = page + (buddy_pfn - pfn);
			if (!buddy_is_valid(migration_type, buddy_pfn,
					    buddy_page) ||
			    !page_is_buddy(buddy_page, order))
				goto free_page_migration_done;

			list_del(&buddy_page->node);
			clear_page_order(buddy_page);
			clear_page_node(buddy_page);
			AREA_FREECOUNT(pma, migration_type)[order]--;
			combined_pfn = buddy_pfn & pfn;
			page = page + (combined_pfn - pfn);
			pfn = combined_pfn;
			order++;
			abs_order++;
			set_page_order(page, order);
		}

free_page_migration_done:
		if ((order < max_order - 2) &&
		    buddy_is_valid(migration_type, buddy_pfn, buddy_page)) {
			struct vmm_page *higher_page;
			struct vmm_page *higher_buddy;

			combined_pfn = buddy_pfn & pfn;
			higher_page = page + (combined_pfn - pfn);
			buddy_pfn = next_buddy_pfn(combined_pfn, abs_order + 1);
			higher_buddy = higher_page + (buddy_pfn - combined_pfn);
			if (pfn_is_valid(buddy_pfn) &&
			    page_is_buddy(higher_buddy, order + 1)) {
				temp = &AREA_FREELIST(pma,
						      migration_type)[order];
				list_add_tail(&page->node, temp);
				goto free_page_migration_out;
			}
		}

		if (order >= PAGE_SIZE_SHIFT) {
			order = 0;
			pma = get_pma_lvl(--lvl, pid);
			continue;
		}

free_page_migration_direct:

		temp = &AREA_FREELIST(pma, migration_type)[order];
		list_add_head(&page->node, temp);

free_page_migration_out:
		set_page_order(page, order);
		set_page_section(page, lvl);
		AREA_FREECOUNT(pma, migration_type)
		[order]++;
	}

	return page;
}

void dequeue_area_migration(struct vmm_page *page, int migration_type)
{
	int order_index, lvl_index;
	struct vmm_page *temp_page;
	int pn_id;
	word_t npages = 0;
	int block_shift;
	struct pm_node_struct *pn;

	do {
		temp_page = (struct vmm_page *)compound_remove_head(page);
		if (!temp_page)
			temp_page = page;

		pn_id = get_page_node(temp_page);
#ifndef PNODE_NUM
		if (pn_id >= 1)
			pn_id = 0;
#else
		if (pn_id >= PNODE_NUM)
			pn_id = 0;
#endif
		pn = get_pn_nu(pn_id);
		order_index = get_page_order(temp_page);
		lvl_index = get_page_section(temp_page);
		if (lvl_index > MAX_AREA_LVLS)
			lvl_index = MAX_AREA_LVLS;
		free_area_migration(temp_page, get_pma_lvl(lvl_index, pn_id),
				    lvl_index, order_index, migration_type);
		block_shift = PM_LX_X(PAGE_SIZE_SHIFT, lvl_index);
		npages = BIT(order_index + block_shift);
		if (migration_type != MIGRATION_UNMOVABLE)
			pn->attr.nr_freepages += npages;
	} while (compound_page(page));
}

void split_page_migration(struct pm_area_struct *pma, int lvl, int order,
			  struct vmm_page *page, int migration_type)
{
	int block_shift = PM_LX_X(PAGE_SIZE_SHIFT, lvl);

	split2_area_migration(pma, page, BIT(order - 1 + block_shift),
			      order - 1, migration_type, lvl);
	split2_area_migration(pma, page, 0, order - 1, migration_type, lvl);
}

word_t area_budget(struct pm_area_struct *pma, int lvl, int migration_type,
		   int order, int order_end)
{
	int order_cnt = 0;
	word_t npages = 0;
	int npages_base;

	npages_base = PM_LX_X(PAGE_SIZE_SHIFT, lvl);
	for (int index = order; index <= order_end; ++index) {
		order_cnt = AREA_FREECOUNT(pma, migration_type)[index];
		npages += order_cnt * BIT(index) * BIT(npages_base);
	}
	return npages;
}

word_t node_budget_lvl(int migration_type, int lvl)
{
	word_t budget = 0;

	budget += area_budget(get_pma_lvl(lvl, get_recent_nu()), lvl,
			      migration_type, 0, MAX_PAGE_ORDER);

	return budget;
}

word_t node_budget(int migration_type)
{
	word_t budget = 0;

	for (int i = g_area_lvl; i < MAX_AREA_LVLS; i++)
		budget += node_budget_lvl(migration_type, i);

	return budget;
}

/* In the memory allocator, for each node, there is a continuous space without
 * holes and the node has a start address, so relative page descriptors can be
 * applied to each page of the node, that is, the page descriptor of each node
 * is pfn from 0 to the maximum page frame number, and when the absolute page
 * frame number needs to be used, it needs to be calculated in combination
 * with the start address of the node; the memory allocator is implemented
 * using the hierarchical Buddy algorithm, which divides all pages of a node
 * into different groups, each group having the same order range, using the
 * aligned mapping address as the boundary
 */

static inline int forward_area_budget(int reverse_index, int migration_type,
				      int order)
{
	int order_index = order;

	while (reverse_index >= g_area_lvl) {
		if (area_budget(get_pma_lvl(reverse_index, get_recent_nu()),
				reverse_index, migration_type, order_index,
				MAX_PAGE_ORDER)) {
			break;
		}
		--reverse_index;
		order_index = 0;
	}

	return reverse_index;
}

static inline int backward_area_budget(int index, int migration_type, int order)
{
	int order_index = order;
	int order_npages = 0;
	int npages_need = BIT(order);

	while (index < MAX_RECURSION_CNTS) {
		order_npages =
			area_budget(get_pma_lvl(index, get_recent_nu()), index,
				    migration_type, 0, order_index - 1);
		if (order_npages >= npages_need)
			break;
		npages_need -= order_npages;
		++index;
		order_index = PAGE_SIZE_SHIFT;
		npages_need <<= PM_LX_R(PAGE_SIZE_SHIFT);
	}
	return index;
}

struct vmm_page *forward_remove_area_migration(int reverse_index,
					      int migration_type, int order)
{
	int last_index;
	struct vmm_page *last_page = NULL;

	last_index = forward_area_budget(reverse_index, migration_type, order);
	if (last_index < g_area_lvl)
		goto bad_budget;
	while (last_index != reverse_index) {
		last_page = remove_area_migration(
			get_pma_lvl(last_index, get_recent_nu()), last_index, 0,
			migration_type);
		assert(last_page);
		split_page_migration(get_pma_lvl(last_index + 1,
						 get_recent_nu()),
				     last_index + 1, PAGE_SIZE_SHIFT,
				     last_page, migration_type);
		++last_index;
	}
	last_page =
		remove_area_migration(get_pma_lvl(last_index, get_recent_nu()),
				      last_index, order, migration_type);

bad_budget:

	return last_page;
}

struct vmm_page *backward_remove_area_migration(int index, int migration_type,
					       int order)
{
	int last_index, order_index = order, norder = 1;
	struct vmm_page *last_page = NULL;

	last_index = backward_area_budget(index, migration_type, order);
	if (last_index == MAX_RECURSION_CNTS)
		goto bad_budget;
	while (last_index != index) {
		norder = enqueue_area_migration(
			get_pma_lvl(index, get_recent_nu()), &last_page,
			order_index, norder, migration_type);
		order_index = PAGE_SIZE_SHIFT;
		++index;
		// norder != 0
	}
	norder = enqueue_area_migration(get_pma_lvl(index, get_recent_nu()),
					&last_page, order_index, norder,
					migration_type);
	// norder == 0
bad_budget:
	return last_page;
}

status_t dispatch_pma_migration(struct pm_node_struct *pn, struct vmm_page **hp,
				int order, int lvl, int migration_type,
				bool inlay)
{
	struct vmm_page *tm_hp;
	status_t ret;

	if (migration_type == MIGRATION_RECLAIMABLE)
		return ERR_INVALID_ARGS;

	if (migration_type == MIGRATION_PCPTYPE) {
		/* Migrate the memory page from MOVABLE to the object of the specified
		 * migration type(PCP), and delay the return to MOVABLE
		 */
		/* only forward */
		assert(lvl == MAX_AREA_LVLS && order == 0 &&
		       lvl == MAX_AREA_LVLS);
		ret = pn->vtbl->migrate(pn, &tm_hp, order, lvl,
					MIGRATION_PCPTYPE, inlay);
		if (ret)
			return ret;
		goto final_pfn;
	} else if (migration_type == MIGRATION_UNMOVABLE) {
		/* Migrate the memory page from MOVABLE to the object of the specified
		 * migration type (UNMOVABLE), and never return it to MOVABLE
		 */

		/* not inlay, only one lvl(?TODO:), aligned if need */
		/* only forward */
		ret = pn->vtbl->migrate(pn, &tm_hp, order, lvl,
					MIGRATION_UNMOVABLE, inlay);
		if (ret)
			return ret;
		goto final_pfn;
	} else { /* for MIGRATION_MOVABLE, only for backward find */
		pn->attr.status = AS_BACKWARD;
		ret = pn->vtbl->dispatch(pn, &tm_hp, lvl, migration_type, order,
					 inlay);
		if (ret) /* backward fail, for other node to migrate */
			ret = pn->vtbl->migrate(pn, &tm_hp, order, lvl,
						MIGRATION_MOVABLE, inlay);
		if (ret)
			return ERR_NO_MEMORY;
	}
final_pfn:
	if (hp)
		*hp = tm_hp;
	return NO_ERROR;
}

status_t dispatch_pma(struct pm_node_struct *pn, struct vmm_page **hp, int lvl,
		      int migration_type, int order, bool inlay)
{
	struct vmm_page *tm_hp;
	int block_shift;
	word_t status;

	assert(pn && hp);

	if (migration_type == MIGRATION_PCPTYPE) {
		struct list_node *tm_head;

		tm_head = list_remove_head(&PCP_FREELIST(pn));
		if (tm_head) {
			/* head == hot */
			tm_hp = list_entry_list(tm_head, tm_hp, node);
			*hp = tm_hp;
			PCP_FREECOUNT(pn)--;
			goto direct_dispatch;
		}
	}

	status = pn->attr.status;

	tm_hp = *hp;
	block_shift = PM_LX_X(PAGE_SIZE_SHIFT, lvl);

	switch (status) {
	case AS_IDLE:
	case AS_ONLY_FORWARD:
	case AS_FORWARD: {
		tm_hp = forward_remove_area_migration(lvl, migration_type,
						      order);
		if (tm_hp) {
			pn->attr.status = AS_FORWARD;
			*hp = tm_hp;
			goto final_dispatch;
		}

		if (status == AS_ONLY_FORWARD) {
			printf("lvl is %d, migration_type is %d, order is %d, inlay is %d\n",
			       lvl, migration_type, order, inlay);
			return ERR_NO_MEMORY;
		}

		status_t ret = dispatch_pma_migration(pn, &tm_hp, order, lvl,
						      migration_type, inlay);
		if (ret) {
			printf("lvl is %d, migration_type is %d, order is %d, inlay is %d\n",
			       lvl, migration_type, order, inlay);
			return ret;
		}

		if (tm_hp)
			*hp = tm_hp;
		return NO_ERROR;
	}
	case AS_BACKWARD: {
		if (!inlay) {
			printf("lvl is %d, migration_type is %d, order is %d, inlay is %d\n",
			       lvl, migration_type, order, inlay);
			return ERR_INVALID_ARGS;
		}

		tm_hp = backward_remove_area_migration(lvl, migration_type,
						       order);
		if (tm_hp) {
			pn->attr.status = AS_IDLE;
			*hp = tm_hp;
			goto final_dispatch;
		}
		printf("lvl is %d, migration_type is %d, order is %d, inlay is %d\n",
		       lvl, migration_type, order, inlay);
		return ERR_NO_MEMORY;
	}
	default:
		panic("NO dispatch type!");
		// unreachable
	}

final_dispatch:
	pn->attr.nr_freepages -= BIT(order + block_shift);
	if (compound_page(tm_hp))
		set_page_state(tm_hp, STATE_PAGE_MOVABLE_COMP);
	else
		set_page_state(tm_hp, STATE_PAGE_MOVABLE_NORMAL);

direct_dispatch:

	return NO_ERROR;
}

/* TODO: pcp free to balance for each core */
status_t find_pma_reversed(int migration_type, struct vmm_page *hpa)
{
	struct vmm_page *hpa_h_ptr = hpa, *hp_tmp;

	int pn_id = get_page_node(hpa_h_ptr);

#ifndef PNODE_NUM
	if (pn_id >= 1)
		pn_id = 0;
#else
	if (pn_id >= PNODE_NUM)
		pn_id = 0;
#endif
	struct pm_node_struct *pn = get_pn_nu(pn_id);

	/* Migrate the memory page from MOVABLE to the object of the specified
	 * migration type(PCP), and delay the return to MOVABLE by PCP count
	 */
	if (migration_type == MIGRATION_PCPTYPE) {
		list_add_head(&hpa_h_ptr->node,
			     &PCP_FREELIST(pn)); // free hot page
		PCP_FREECOUNT(pn)++;
		if (PCP_FREECOUNT(pn) < PCP_MAX_NR)
			goto final_reversed;
		hp_tmp = list_tail_entry(&PCP_FREELIST(pn), hp_tmp, node);
		// coverity[var_deref_model:SUPPRESS]
		list_del(&hp_tmp->node); // tail = cold
		PCP_FREECOUNT(pn)--;
		hpa_h_ptr = hp_tmp;
		// TO DEBUG
		// migration_type = MIGRATION_MOVABLE;
	}

	dequeue_area_migration(hpa_h_ptr, migration_type);

final_reversed:
	return NO_ERROR;
}

// for compound hpa
status_t find_pma(word_t npages, int migration_type, bool inlay, int lvl,
		  int lvl_end, struct vmm_page **hpa)
{
	int lvl_order;
	int level;
	int block_shift;
	word_t block_mask;
	word_t block_size;
	word_t lvl_npages;
	struct vmm_page *hp_tmp;
	struct vmm_page *hpa_h_ptr = NULL;
	struct pm_node_struct *pn = get_recent_pn();

	for (level = lvl; level < lvl_end; ++level) {
		block_shift = PM_LX_X(PAGE_SIZE_SHIFT, level);
		block_size = BIT(block_shift);
		block_mask = block_size - 1;
		lvl_npages = npages >> block_shift;
		npages &= block_mask;
		if (!lvl_npages)
			continue;

		do {
			if (inlay)
				lvl_order = pages_to_orders(&lvl_npages);
			else
				lvl_order = pages_to_order(&lvl_npages);

			status_t ret = pn->vtbl->dispatch(pn, &hp_tmp, level,
							  migration_type,
							  lvl_order, inlay);
			if (ret)
				goto bad_pfn;

			if (!hpa_h_ptr)
				hpa_h_ptr = hp_tmp;

			if (inlay &&
			    (hpa_h_ptr != hp_tmp || !compound_page(hp_tmp)))
				compound_append(hp_tmp, hpa_h_ptr);
		} while (lvl_npages);
	}

	if (hpa)
		*hpa = hpa_h_ptr;

	return NO_ERROR;

bad_pfn:

	if (hpa_h_ptr)
		find_pma_reversed(migration_type, hpa_h_ptr);
	return ERR_NO_MEMORY;
}

status_t compound_pma(struct vmm_page **h, word_t npages, int migration_type,
		      bool inlay)
{
	struct vmm_page *hp_h_ptr = NULL;

	if (!npages)
		return ERR_INVALID_ARGS;
	if (migration_type < 0 || migration_type >= MIGRATION_TYPES)
		return ERR_INVALID_ARGS;

#ifndef PNODE_NUM
	status_t ret = find_pma(npages, migration_type, inlay, g_area_lvl,
				MAX_RECURSION_CNTS, &hp_h_ptr);
#else
	status_t ret = find_pma(npages, migration_type, inlay,
				g_area_lvl[get_recent_nu()], MAX_RECURSION_CNTS,
				&hp_h_ptr);
#endif

	if (ret)
		return ret;
	if (h)
		*h = hp_h_ptr;

	return NO_ERROR;
}

status_t decompose_pma(struct vmm_page *hp)
{
	struct vmm_page *hp_h_ptr = hp;

	int migration_type = page_state_to_migration(get_page_state(hp));

	if (migration_type < 0 || migration_type >= MIGRATION_TYPES)
		return ERR_INVALID_ARGS;

	status_t ret = find_pma_reversed(migration_type, hp_h_ptr);
	return ret;
}

// __init
status_t init_pnode(struct pm_node_struct *self)
{
	_init_pnode(self);
	return NO_ERROR;
}

status_t migrate_pma(struct pm_node_struct *const self, struct vmm_page **hp,
		     int order, int lvl, int type, bool inlay)
{
	struct pm_node_struct *const pn = self;
	struct vmm_page *hp_tmp;

	switch (type) {
	case MIGRATION_PCPTYPE: {
		status_t ret = pn->vtbl->dispatch(
			pn, &hp_tmp, lvl, MIGRATION_MOVABLE, order, inlay);
		if (ret)
			return ret;

		// not do that list append, but free to diff pos
		set_page_state(hp_tmp, STATE_PAGE_PCP_TYPES);
		break;
	}

	case MIGRATION_UNMOVABLE: {
		pn->attr.status = AS_ONLY_FORWARD;

		status_t ret = pn->vtbl->dispatch(
			pn, &hp_tmp, lvl, MIGRATION_MOVABLE, order, inlay);
		if (ret)
			return ret;
		set_page_state(hp_tmp, STATE_PAGE_NOMOVABLE_TYPES);
		break;
	}

	case MIGRATION_MOVABLE: {
		status_t ret = pn->vtbl->lend(&hp_tmp, MIGRATION_MOVABLE, lvl,
					      order, inlay);
		if (ret)
			return ret;
		// not do that list append, but free to diff pos
		set_page_state(hp_tmp, STATE_PAGE_MOVABLE_NODE);

		break;
	}

	case MIGRATION_RECLAIMABLE:
	default:
		panic("INvalid migration type!\n");
		// unreachable
	}

	if (hp)
		*hp = hp_tmp;
	return NO_ERROR;
}

#ifdef PNODE_NUM
// the only one level
status_t repeating_dispatch(struct vmm_page **hp, struct pm_node_struct *pn,
			    int lvl, int migration_type, word_t npages,
			    bool inlay)
{
	struct vmm_page *hp_tmp, *hp_h_ptr = NULL;
	int order;
	status_t ret;

	while (npages) {
		if (inlay)
			order = pages_to_orders(&npages);
		else
			order = pages_to_order(&npages);

		ret = pn->vtbl->dispatch(pn, &hp_tmp, lvl, migration_type,
					 order, inlay);
		if (ret)
			return ret;
		if (!hp_h_ptr)
			hp_h_ptr = hp_tmp;

		if (inlay && (hp_h_ptr != hp_tmp || !compound_page(hp_h_ptr)))
			compound_append(hp_tmp, hp_h_ptr);
	}

	if (hp)
		*hp = hp_h_ptr;
	return NO_ERROR;
}
#endif

status_t lend_pma(struct vmm_page **hp, int migration_type, int lvl,
		  int lvl_order, bool inlay)
{
#ifdef PNODE_NUM

	struct vmm_page *hp_tmp, *hp_h_ptr = NULL;
	struct pm_node_struct *pn;
	int i;
	status_t ret;
	int block_shift;
	long nr_pages, pn_freepages;

	block_shift = PM_LX_X(PAGE_SIZE_SHIFT, lvl);
	nr_pages = BIT(lvl_order + block_shift);

	for (i = 0; i < PNODE_NUM && nr_pages; i++) {
		pn = get_pn_nu(i);
		pn_freepages = pn->attr.nr_freepages;
		if (!pn_freepages)
			continue;

		if (nr_pages > pn_freepages) {
			if (!inlay)
				continue;
			ret = repeating_dispatch(&hp_tmp, pn, lvl,
						 migration_type, pn_freepages,
						 inlay);
			if (ret)
				return ret;
			nr_pages -= pn_freepages;
		} else {
			ret = repeating_dispatch(&hp_tmp, pn, lvl,
						 migration_type, nr_pages,
						 inlay);
			if (ret)
				return ret;
			nr_pages = 0;
		}
		if (!hp_h_ptr)
			hp_h_ptr = hp_tmp;
		if (inlay && (hp_h_ptr != hp_tmp || !compound_page(hp_h_ptr)))
			compound_append(hp_tmp, hp_h_ptr);
	}

	if (hp)
		*hp = hp_h_ptr;
	return NO_ERROR;
#endif

	return ERR_NO_MEMORY;
}

bool budget_sufficient(long npages, int migration_type)
{
	struct pm_node_struct *pn;
	long remaining_npages = 0;

#ifdef PNODE_NUM
	for (int i = 0; i < PNODE_NUM; i++) {
		pn = get_pn_nu(i);
		npages -= pn->attr.nr_freepages;
		if (npages < 0)
			return true;
	}
#else
	pn = get_recent_pn();
	npages -= pn->attr.nr_freepages;
	if (npages <= 0)
		return true;
#endif
	if (migration_type == MIGRATION_PCPTYPE)
		remaining_npages = PCP_FREECOUNT(pn);
	else if (migration_type == MIGRATION_UNMOVABLE)
		remaining_npages = node_budget(migration_type);
	if (remaining_npages >= npages)
		return true;
	return false;
}

// pcp & migration
status_t pm_node_struct_migrate(struct pm_node_struct *const self,
				struct vmm_page **hp, int order, int lvl,
				int type, bool inlay)
{
	return migrate_pma(self, hp, order, lvl, type, inlay);
}

// (hpa)<single unit(ha), signle unit(ha) ...> or (hpa)<signal unit(ha),
// compound unit(ha) ...> hpa=NULL, only ha
static status_t pm_area_struct_compound(struct vmm_page **hp, word_t npages,
					int migration_type, bool inlay)
{
	return compound_pma(hp, npages, migration_type, inlay);
}

static status_t pm_area_struct_decompose(struct vmm_page *hp)
{
	return decompose_pma(hp);
}

static void
pm_area_struct_dump_migration(struct pm_area_struct *const self, int type)
{
	struct area_list *area_list;

	area_list = &self->free_area[type];
	for (int order = 0; order < PAGE_SIZE_SHIFT; order++) {
		if (area_list->free_count[order])
			printf(
				"[type, freelist, order, count]:%d, %p, %d, %ld\n",
				type, &area_list->freelist[order], order,
				area_list->free_count[order]);
	}
}

static status_t pm_area_struct_dump_area(struct pm_area_struct *const self)
{
	for (int type = 0; type < MIGRATION_TYPES; type++)
		pm_area_struct_dump_migration(self, type);

	return NO_ERROR;
}

static status_t pm_node_struct_dispatch(struct pm_node_struct *const self,
					struct vmm_page **hp, int lvl,
					int migration_type, int order,
					bool inlay)
{
	return dispatch_pma(self, hp, lvl, migration_type, order, inlay);
}

static status_t pm_node_struct_init(struct pm_node_struct *const self)
{
	return init_pnode(self);
}

static status_t pm_node_struct_lend(struct vmm_page **hp, int migration_type,
				    int lvl, int lvl_order, bool inlay)
{
	return lend_pma(hp, migration_type, lvl, lvl_order, inlay);
}

ssize_t preprocess_pm_node_struct_alloc(int flags, word_t npages,
					int migration_type)
{
	word_t tmp_npages = npages;

	switch (flags) {
	case FUNC_RESERVED: {
		if (tmp_npages == node_budget(migration_type))
			break;
		for (int i = g_area_lvl; i < MAX_AREA_LVLS; i++)
			if (tmp_npages == node_budget_lvl(migration_type, i))
				break;
		pages_to_orders(&tmp_npages);
		if (!tmp_npages)
			break;
		return ERR_INVALID_ARGS;
	}
	case FUNC_DEBUG: {
		word_t node_npages = node_budget(migration_type);

		if (node_npages > UNMOV_MAX_NR || tmp_npages > UNMOV_MAX_NR)
			return ERR_INVALID_ARGS;
		pages_to_orders(&tmp_npages);
		if (tmp_npages)
			return ERR_INVALID_ARGS;
		break;
	}
	case FUNC_SYSCALL_CONT:
	case FUNC_MMU:
	case FUNC_OBJECT: {
		npages = BIT(pages_to_order(&tmp_npages));
		break;
	}
	default:
		break;
	}
	return npages;
}

// owner or borrower
static status_t pm_node_struct_alloc(struct pm_node_struct *pn, paddr_t *paddr,
				     word_t npages, int flags)
{
	struct vmm_page *hp;
	int migration_type;
	bool inlay;
	ssize_t psize;

	// G_LOCK
	// watermark
	if (!npages) {
		printf("npages args is zero\n");
		return ERR_INVALID_ARGS;
	}

	prepare_pm_node_struct_alloc(npages, flags, &migration_type, &inlay);

	spin_lock_saved_state_t sflags;

	spin_lock_irqsave(&pn->node_struct_lock, sflags);
	psize = preprocess_pm_node_struct_alloc(flags, npages, migration_type);
	if (psize <= 0) {
		spin_unlock_irqrestore(&pn->node_struct_lock, sflags);
		printf("args parse is error, npages is %d, flags is %d, migration_type is %d, inlay is %d\n",
		       psize, flags, migration_type, inlay);
		return ERR_INVALID_ARGS;
	}

	bool suff = budget_sufficient(psize, migration_type);

	if (!suff) {
		spin_unlock_irqrestore(&pn->node_struct_lock, sflags);
		printf("budge is zero, npages args is %d\n", psize);
		return ERR_NO_MEMORY;
	}

	status_t ret =
		pn->area_vtbl->compound(&hp, psize, migration_type, inlay);
	spin_unlock_irqrestore(&pn->node_struct_lock, sflags);

	if (ret)
		return ret;
	if (paddr)
		*paddr = page_to_paddr(hp);
	return NO_ERROR;
}

// owner or borrower
static status_t pm_node_struct_free(paddr_t paddr)
{
	// G_LOCK

	long pfn = paddr_to_pfn(paddr);
	struct pm_node_struct *pn = pfn_to_node(pfn);

	if (!pn) {
		printf("paddr to pn is error, paddr is 0x%lx\n", paddr);
		return ERR_INVALID_ARGS;
	}

	spin_lock_saved_state_t flags;

	spin_lock_irqsave(&pn->node_struct_lock, flags);
	status_t ret = pn->area_vtbl->decompose(paddr_to_page(paddr));

	spin_unlock_irqrestore(&pn->node_struct_lock, flags);

	return ret;
}

void pm_node_struct_dump_pcp(struct pm_node_struct *const self)
{
	struct node_list *node_list;

	node_list = &self->pcp;
	for (int cid = 0; cid < NODE_X_NU; cid++) {
		if (node_list->free_count[cid])
			printf("[pcplist, pcpcount]: %p, %d\n",
				  &node_list->freelist[cid],
				  node_list->free_count[cid]);
	}
}

void pn_node_struct_dump_area(int nid)
{
	struct pm_area_struct *pma;

	for (int lvl = g_area_lvl; lvl < MAX_RECURSION_CNTS; lvl++) {
#ifndef PNODE_NUM
		pma = &g_pm_area_struct[lvl];
#else
		pma = &g_pm_area_struct[nid][lvl];
#endif
		printf("[pma, lvl]: %p, %d\n", pma, lvl);
		pma->vtbl->dump_area(pma);
	}
}

static status_t pm_node_struct_dump_node(struct pm_node_struct *const self)
{
	spin_lock_saved_state_t sflags;

	spin_lock_irqsave(&self->node_struct_lock, sflags);

	int nid = self->pn_id;

	printf(
		"[nid %d, addr 0x%lx, end 0x%lx, status %d, nr_freepages 0x%lx]\n",
		nid, self->attr.addr, self->attr.size, self->attr.status,
		self->attr.nr_freepages);
	pm_node_struct_dump_pcp(self);
	pn_node_struct_dump_area(nid);
#ifndef PNODE_NUM
	printf(
		"[pg_start %p, pg_end %p, pg_align %p, entry_lvl %d, entry_order %d]\n",
		g_vm_pages, g_vm_pages_end, g_vm_pages_of, g_area_lvl,
		g_area_order);
#else
	printf(
		"[pg_start %p, pg_end %p, pg_align %p, entry_lvl %d, entry_order %d]\n",
		g_vm_pages[nid], g_vm_pages_end[nid], g_vm_pages_of[nid],
		g_area_lvl[nid], g_area_order[nid]);
#endif
	spin_unlock_irqrestore(&self->node_struct_lock, sflags);
	return NO_ERROR;
}

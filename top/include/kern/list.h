/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <kern/compiler.h>
#include <stdbool.h>
#include <stddef.h>

__BEGIN_CDECLS

#define containerof(ptr, type, member) \
    ((type *)((addr_t)(ptr) - offsetof(type, member)))

struct list_node {
    struct list_node *prev;
    struct list_node *next;
};

#define LIST_INITIAL_VALUE(list) { &(list), &(list) }
#define LIST_INITIAL_CLEARED_VALUE { NULL, NULL }

static inline void list_initialize(struct list_node *list) {
    list->prev = list->next = list;
}

static inline void list_clear_node(struct list_node *item) {
    item->prev = item->next = 0;
}

static inline bool list_in_list(struct list_node *item) {
    if (item->prev == 0 && item->next == 0)
        return false;
    else
        return true;
}

static inline void list_add_head(struct list_node *list, struct list_node *item) {
    item->next = list->next;
    item->prev = list;
    list->next->prev = item;
    list->next = item;
}

#define list_add_after(entry, new_entry) list_add_head(entry, new_entry)

static inline void list_add_tail(struct list_node *list, struct list_node *item) {
    item->prev = list->prev;
    item->next = list;
    list->prev->next = item;
    list->prev = item;
}

#define list_add_before(entry, new_entry) list_add_tail(entry, new_entry)

static inline void list_delete(struct list_node *item) {
    item->next->prev = item->prev;
    item->prev->next = item->next;
    item->prev = item->next = 0;
}

static inline struct list_node *list_remove_head(struct list_node *list) {
    if (list->next != list) {
        struct list_node *item = list->next;
        list_delete(item);
        return item;
    } else {
        return NULL;
    }
}

#define list_remove_head_type(list, type, element) ({\
    struct list_node *__nod = list_remove_head(list);\
    type *__t;\
    if(__nod)\
        __t = containerof(__nod, type, element);\
    else\
        __t = (type *)0;\
    __t;\
})

static inline struct list_node *list_remove_tail(struct list_node *list) {
    if (list->prev != list) {
        struct list_node *item = list->prev;
        list_delete(item);
        return item;
    } else {
        return NULL;
    }
}

#define list_remove_tail_type(list, type, element) ({\
    struct list_node *__nod = list_remove_tail(list);\
    type *__t;\
    if(__nod)\
        __t = containerof(__nod, type, element);\
    else\
        __t = (type *)0;\
    __t;\
})

static inline struct list_node *list_peek_head(struct list_node *list) {
    if (list->next != list) {
        return list->next;
    } else {
        return NULL;
    }
}

#define list_peek_head_type(list, type, element) ({\
    struct list_node *__nod = list_peek_head(list);\
    type *__t;\
    if(__nod)\
        __t = containerof(__nod, type, element);\
    else\
        __t = (type *)0;\
    __t;\
})

static inline struct list_node *list_peek_tail(struct list_node *list) {
    if (list->prev != list) {
        return list->prev;
    } else {
        return NULL;
    }
}

#define list_peek_tail_type(list, type, element) ({\
    struct list_node *__nod = list_peek_tail(list);\
    type *__t;\
    if(__nod)\
        __t = containerof(__nod, type, element);\
    else\
        __t = (type *)0;\
    __t;\
})

static inline struct list_node *list_prev(struct list_node *list, struct list_node *item) {
    if (item->prev != list)
        return item->prev;
    else
        return NULL;
}

#define list_prev_type(list, item, type, element) ({\
    struct list_node *__nod = list_prev(list, item);\
    type *__t;\
    if(__nod)\
        __t = containerof(__nod, type, element);\
    else\
        __t = (type *)0;\
    __t;\
})

static inline struct list_node *list_prev_wrap(struct list_node *list, struct list_node *item) {
    if (item->prev != list)
        return item->prev;
    else if (item->prev->prev != list)
        return item->prev->prev;
    else
        return NULL;
}

#define list_prev_wrap_type(list, item, type, element) ({\
    struct list_node *__nod = list_prev_wrap(list, item);\
    type *__t;\
    if(__nod)\
        __t = containerof(__nod, type, element);\
    else\
        __t = (type *)0;\
    __t;\
})

static inline struct list_node *list_next(struct list_node *list, struct list_node *item) {
    if (item->next != list)
        return item->next;
    else
        return NULL;
}

#define list_next_type(list, item, type, element) ({\
    struct list_node *__nod = list_next(list, item);\
    type *__t;\
    if(__nod)\
        __t = containerof(__nod, type, element);\
    else\
        __t = (type *)0;\
    __t;\
})

static inline struct list_node *list_next_wrap(struct list_node *list, struct list_node *item) {
    if (item->next != list)
        return item->next;
    else if (item->next->next != list)
        return item->next->next;
    else
        return NULL;
}

#define list_next_wrap_type(list, item, type, element) ({\
    struct list_node *__nod = list_next_wrap(list, item);\
    type *__t;\
    if(__nod)\
        __t = containerof(__nod, type, element);\
    else\
        __t = (type *)0;\
    __t;\
})

// iterates over the list, node should be struct list_node*
#define list_for_every(list, node) \
    for(node = (list)->next; node != (list); node = node->next)

// iterates over the list in a safe way for deletion of current node
// node and temp_node should be struct list_node*
#define list_for_every_safe(list, node, temp_node) \
    for(node = (list)->next, temp_node = (node)->next;\
    node != (list);\
    node = temp_node, temp_node = (node)->next)

// iterates over the list, entry should be the container structure type *
#define list_for_every_entry(list, entry, type, member) \
    for((entry) = containerof((list)->next, type, member);\
        &(entry)->member != (list);\
        (entry) = containerof((entry)->member.next, type, member))

// iterates over the list in a safe way for deletion of current node
// entry and temp_entry should be the container structure type *
#define list_for_every_entry_safe(list, entry, temp_entry, type, member) \
    for(entry = containerof((list)->next, type, member),\
        temp_entry = containerof((entry)->member.next, type, member);\
        &(entry)->member != (list);\
        entry = temp_entry, temp_entry = containerof((temp_entry)->member.next, type, member))

static inline bool list_is_empty(struct list_node *list) {
    return (list->next == list) ? true : false;
}

static inline size_t list_length(struct list_node *list) {
    size_t cnt = 0;
    struct list_node *node = list;
    list_for_every(list, node) {
        cnt++;
    }

    return cnt;
}



static inline void list_del(struct list_node *node)
{
	node->prev->next = node->next;
	node->next->prev = node->prev;
	node->prev = node->next = 0;
}

static inline void init_list_node(struct list_node *node)
{
	node->next = NULL;
	node->prev = NULL;
}

static inline bool list_is_head(struct list_node *list, struct list_node *node)
{
	return list->next == node;
}

static inline bool list_is_tail(struct list_node *list, struct list_node *node)
{
	return list->prev == node;
}

static inline struct list_node *list_peek_next_no_check(struct list_node *list,
							struct list_node *node)
{
	return (node == list->prev) ? NULL : node->next;
}

static inline struct list_node *list_peek_prev_no_check(struct list_node *list,
							struct list_node *node)
{
	return (node == list->next) ? NULL : node->prev;
}

static inline struct list_node *list_peek_prev(struct list_node *list,
					       struct list_node *node)
{
	return (node != NULL) ? list_peek_prev_no_check(list, node) : NULL;
}
static inline struct list_node *list_peek_next(struct list_node *list,
					       struct list_node *node)
{
	return (node != NULL) ? list_peek_next_no_check(list, node) : NULL;
}

#define list_entry(ptr, type, field) containerof(ptr, type, field)

#define list_entry_list(__dn, __cn, __n)                                       \
	((__dn != NULL) ? containerof(__dn, __typeof__(*__cn), __n) : NULL)

#define list_head_entry(__dl, __cn, __n)                                       \
	list_entry_list(list_peek_head(__dl), __cn, __n)

#define list_tail_entry(__dl, __cn, __n)                                       \
	list_entry_list(list_peek_tail(__dl), __cn, __n)

#define list_next_entry(__dl, __cn, __n)                                       \
	((__cn != NULL) ? list_entry_list(list_peek_next(__dl, &(__cn->__n)),  \
					  __cn, __n) :                         \
			  NULL)

#define list_prev_entry(__dl, __cn, __n)                                       \
	((__cn != NULL) ? list_entry_list(list_peek_prev(__dl, &(__cn->__n)),  \
					  __cn, __n) :                         \
			  NULL)

#define list_for_each_entry_list(__dl, __cn, __n)                              \
	for (__cn = list_head_entry(__dl, __cn, __n); __cn != NULL;            \
	     __cn = list_next_entry(__dl, __cn, __n))

#define list_for_each_entry_inverted(__dl, __cn, __n)                          \
	for (__cn = list_tail_entry(__dl, __cn, __n); __cn != NULL;            \
	     __cn = list_prev_entry(__dl, __cn, __n))

#define list_for_each_entry_safe(__dl, __cn, __cns, __n)                       \
	for (__cn = list_head_entry(__dl, __cn, __n),                          \
	    __cns = list_next_entry(__dl, __cn, __n);                          \
	     __cn != NULL;                                                     \
	     __cn = __cns, __cns = list_next_entry(__dl, __cn, __n))

#define list_for_each_entry_safe_inverted(__dl, __cn, __cns, __n)              \
	for (__cn = list_tail_entry(__dl, __cn, __n),                          \
	    __cns = list_prev_entry(__dl, __cn, __n);                          \
	     __cn != NULL;                                                     \
	     __cn = __cns, __cns = list_prev_entry(__dl, __cn, __n))

#define list_for_each_node(__dl, __dn)                                         \
	for (__dn = list_peek_head(__dl); __dn != NULL;                        \
	     __dn = list_peek_next(__dl, __dn))

static inline void *list_search(struct list_node *list, void *key,
				int (*compare)(void *, void *))
{
	struct list_node *node;

	list_for_each_node(list, node)
	{
		if (compare(node, key))
			return node;
	};
	return NULL;
}

__END_CDECLS

#pragma once

#include <sys/types.h>
#include <kern/err.h>

struct avl_node {
	struct avl_node *left;
	struct avl_node *right;
	int height;
};
typedef struct avl_node avl_node_t;
typedef avl_node_t *avl_root_t;


#define AVL_ERR_NODE ((void *) 1)
#define AVL_MAX_HEIGHT 42

#define avl_entry(ptr, type, field) containerof(ptr, type, field)

void *avl_search(avl_root_t root, void *key, int (*compare)(void *, void *));
status_t avl_insert(avl_root_t *root, void *new_node, void *key,
		    int (*compare)(void *, void *));
status_t avl_insert_inform(avl_root_t *root, void *new_node, void *key,
			   void **key_holder, int (*compare)(void *, void *));

void *avl_remove_insert(avl_root_t *root, void *new_node, void *key,
			int (*compare)(void *, void *));
void *avl_delete(avl_root_t *root, void *key, int (*compare)(void *, void *));
void *avl_superior_get(avl_root_t root, void *key,
		       int (*compare)(void *, void *));
void *avl_inferior_get(avl_root_t root, void *key,
		       int (*compare)(void *, void *));
void *avl_minimum_get(avl_root_t root);
void *avl_maximum_get(avl_root_t root);
status_t avl_tree_walk(avl_root_t *root, void (*walk_exec)(avl_root_t *));
status_t avl_tree_erase(avl_root_t *root, status_t (*free)(avl_node_t *node));
void *avl_inheritor_get(avl_root_t root, void *key,
			int (*compare)(void *, void *));



#include <lib/avl.h>

void avl_rebalance(avl_node_t ***ancestors, int count)
{
	while (count > 0) {
		avl_node_t **node_pptr;
		avl_node_t *node_ptr;
		avl_node_t *left_ptr;
		int lefth;
		avl_node_t *right_ptr;
		int righth;

		node_pptr = ancestors[--count];
		node_ptr = *node_pptr;
		left_ptr = node_ptr->left;
		right_ptr = node_ptr->right;
		lefth = (left_ptr != NULL) ? left_ptr->height : 0;
		righth = (right_ptr != NULL) ? right_ptr->height : 0;

		if (righth - lefth < -1) {
			/*
       *         *
       *       /   \
       *    n+2      n
       *
       * The current subtree violates the balancing rules by beeing too
       * high on the left side. We must use one of two different
       * rebalancing methods depending on the configuration of the left
       * subtree.
       *
       * Note that leftp cannot be NULL or we would not pass there !
       */
			avl_node_t *leftleft_ptr;
			avl_node_t *leftright_ptr;
			int leftrighth;

			leftleft_ptr = left_ptr->left;
			leftright_ptr = left_ptr->right;
			leftrighth = (leftright_ptr != NULL) ?
					     leftright_ptr->height :
					     0;
			if ((leftleft_ptr != NULL) &&
			    (leftleft_ptr->height >= leftrighth)) {
				/*
         *            <D>                     <B>
         *             *                    n+2|n+3
         *           /   \                   /   \
         *        <B>     <E>    ---->    <A>     <D>
         *        n+2      n              n+1   n+1|n+2
         *       /   \                           /   \
         *    <A>     <C>                     <C>     <E>
         *    n+1    n|n+1                   n|n+1     n
         */
				node_ptr->left = leftright_ptr;
				node_ptr->height = leftrighth + 1;
				left_ptr->right = node_ptr;
				left_ptr->height = leftrighth + 2;
				*node_pptr = left_ptr;
			} else {
				/*
         *           <F>
         *            *
         *          /   \                        <D>
         *       <B>     <G>                     n+2
         *       n+2      n                     /   \
         *      /   \           ---->        <B>     <F>
         *   <A>     <D>                     n+1     n+1
         *    n      n+1                    /  \     /  \
         *          /   \                <A>   <C> <E>   <G>
         *       <C>     <E>              n  n|n-1 n|n-1  n
         *      n|n-1   n|n-1
         *
         * We can assume that leftrightp is not NULL because we expect
         * leftp and rightp to conform to the AVL balancing rules.
         * Note that if this assumption is wrong, the algorithm will
         * crash here.
         */
				left_ptr->right = leftright_ptr->left;
				left_ptr->height = leftrighth;
				node_ptr->left = leftright_ptr->right;
				node_ptr->height = leftrighth;
				leftright_ptr->left = left_ptr;
				leftright_ptr->right = node_ptr;
				leftright_ptr->height = leftrighth + 1;
				*node_pptr = leftright_ptr;
			}
		} else if (righth - lefth > 1) {
			/*
       *        *
       *      /   \
       *    n      n+2
       *
       * The current subtree violates the balancing rules by beeing too
       * high on the right side. This is exactly symmetric to the
       * previous case. We must use one of two different rebalancing
       * methods depending on the configuration of the right subtree.
       *
       * Note that rightp cannot be NULL or we would not pass there !
       */
			avl_node_t *rightleft_ptr;
			avl_node_t *rightright_ptr;
			int rightlefth;

			rightleft_ptr = right_ptr->left;
			rightlefth =
				(rightleft_ptr != NULL) ? rightleft_ptr->height : 0;
			rightright_ptr = right_ptr->right;
			if ((rightright_ptr != NULL) &&
			    (rightright_ptr->height >= rightlefth)) {
				/*        <B>                             <D>
         *         *                            n+2|n+3
         *       /   \                           /   \
         *    <A>     <D>        ---->        <B>     <E>
         *     n      n+2                   n+1|n+2   n+1
         *           /   \                   /   \
         *        <C>     <E>             <A>     <C>
         *       n|n+1    n+1              n     n|n+1
         */
				node_ptr->right = rightleft_ptr;
				node_ptr->height = rightlefth + 1;
				right_ptr->left = node_ptr;
				right_ptr->height = rightlefth + 2;
				*node_pptr = right_ptr;
			} else {
				/*        <B>
         *         *
         *       /   \                            <D>
         *    <A>     <F>                         n+2
         *     n      n+2                        /   \
         *           /   \       ---->        <B>     <F>
         *        <D>     <G>                 n+1     n+1
         *        n+1      n                 /  \     /  \
         *       /   \                    <A>   <C> <E>   <G>
         *    <C>     <E>                  n  n|n-1 n|n-1  n
         *   n|n-1   n|n-1
         *
         * We can assume that rightleftp is not NULL because we expect
         * leftp and rightp to conform to the AVL balancing rules.
         * Note that if this assumption is wrong, the algorithm will
         * crash here.
         */
				node_ptr->right = rightleft_ptr->left;
				node_ptr->height = rightlefth;
				right_ptr->left = rightleft_ptr->right;
				right_ptr->height = rightlefth;
				rightleft_ptr->left = node_ptr;
				rightleft_ptr->right = right_ptr;
				rightleft_ptr->height = rightlefth + 1;
				*node_pptr = rightleft_ptr;
			}
		} else {
			/*
       * No rebalancing, just set the tree height
       *
       * If the height of the current subtree has not changed, we can
       * stop here because we know that we have not broken the AVL
       * balancing rules for our ancestors.
       */
			int height;

			height = ((righth > lefth) ? righth : lefth) + 1;
			if (node_ptr->height == height)
				break;
			node_ptr->height = height;
		}
	}
}

void *avl_search(avl_root_t root, void *key, int (*compare)(void *, void *))
{
	avl_node_t *node_ptr;

	node_ptr = root;
	while (1) {
		int delta;

		if (node_ptr == NULL)
			return NULL;
		delta = compare(node_ptr, key);
		if (delta == ERR_FAULT)
			return AVL_ERR_NODE;
		if (delta == 0)
			return node_ptr;
		else if (delta < 0)
			node_ptr = node_ptr->left;
		else
			node_ptr = node_ptr->right;
	}
}

status_t avl_insert(avl_root_t *root, void *new_node, void *key,
		    int (*compare)(void *, void *))
{
	avl_node_t **node_pptr;
	avl_node_t **ancestor[AVL_MAX_HEIGHT];
	int ancestor_cnt;

	node_pptr = root;
	ancestor_cnt = 0;

	while (1) {
		avl_node_t *node_ptr;
		int delta;

		node_ptr = *node_pptr;
		if (node_ptr == NULL)
			break;
		ancestor[ancestor_cnt++] = node_pptr;
		delta = compare(node_ptr, key);
		if (delta == ERR_FAULT)
			return ERR_FAULT;
		if (delta == 0)
			return ERR_BAD_PATH;
		else if (delta < 0)
			node_pptr = (avl_node_t **)&(node_ptr->left);
		else
			node_pptr = (avl_node_t **)&(node_ptr->right);
	}

	((avl_node_t *)new_node)->left = NULL;
	((avl_node_t *)new_node)->right = NULL;
	((avl_node_t *)new_node)->height = 1;
	*node_pptr = new_node;

	avl_rebalance(ancestor, ancestor_cnt);

	return NO_ERROR;
}

status_t avl_insert_inform(avl_root_t *root, void *new_node, void *key,
			   void **key_holder, int (*compare)(void *, void *))
{
	avl_node_t **node_pptr;
	avl_node_t **ancestor[AVL_MAX_HEIGHT];
	int ancestor_cnt;

	if (key_holder == NULL)
		return ERR_BAD_PATH;

	node_pptr = root;
	ancestor_cnt = 0;

	while (1) {
		avl_node_t *node_ptr;
		int delta;

		node_ptr = *node_pptr;
		if (node_ptr == NULL)
			break;
		ancestor[ancestor_cnt++] = node_pptr;
		delta = compare(node_ptr, key);
		if (delta == ERR_FAULT)
			return ERR_FAULT;
		if (delta == 0) {
			*key_holder = node_ptr;
			return ERR_BAD_PATH;
		} else if (delta < 0) {
			node_pptr = (avl_node_t **)&(node_ptr->left);
		} else {
			node_pptr = (avl_node_t **)&(node_ptr->right);
		}
	}

	((avl_node_t *)new_node)->left = NULL;
	((avl_node_t *)new_node)->right = NULL;
	((avl_node_t *)new_node)->height = 1;
	*node_pptr = new_node;
	*key_holder = new_node;
	avl_rebalance(ancestor, ancestor_cnt);

	return NO_ERROR;
}

void *avl_remove_insert(avl_root_t *root, void *new_node, void *key,
			int (*compare)(void *, void *))
{
	avl_node_t **node_pptr;
	avl_node_t **ancestor[AVL_MAX_HEIGHT];
	int ancestor_cnt;

	node_pptr = root;
	ancestor_cnt = 0;

	while (1) {
		avl_node_t *node_ptr;
		int delta;

		node_ptr = *node_pptr;
		if (node_ptr == NULL)
			break;
		ancestor[ancestor_cnt++] = node_pptr;
		delta = compare(node_ptr, key);
		if (delta == ERR_FAULT)
			return AVL_ERR_NODE;
		if (delta == 0) {
			((avl_node_t *)new_node)->left = node_ptr->left;
			((avl_node_t *)new_node)->right = node_ptr->right;
			((avl_node_t *)new_node)->height = node_ptr->height;

			*node_pptr = new_node;

			node_ptr->left = NULL;
			node_ptr->right = NULL;
			node_ptr->height = 1;

			return node_ptr;
		} else if (delta < 0) {
			node_pptr = (avl_node_t **)&(node_ptr->left);
		} else {
			node_pptr = (avl_node_t **)&(node_ptr->right);
		}
	}

	((avl_node_t *)new_node)->left = NULL;
	((avl_node_t *)new_node)->right = NULL;
	((avl_node_t *)new_node)->height = 1;
	*node_pptr = new_node;

	avl_rebalance(ancestor, ancestor_cnt);

	return NULL;
}

void *avl_delete(avl_root_t *root, void *key, int (*compare)(void *, void *))
{
	avl_node_t **node_pptr;
	avl_node_t *node_ptr;
	avl_node_t **ancestor[AVL_MAX_HEIGHT];
	int ancestor_cnt;
	avl_node_t *delete_ptr;

	node_pptr = root;
	ancestor_cnt = 0;
	while (1) {
		int delta;

		node_ptr = *node_pptr;
		if (node_ptr == NULL)
			return NULL;

		ancestor[ancestor_cnt++] = node_pptr;
		delta = compare(node_ptr, key);
		if (delta == ERR_FAULT)
			return AVL_ERR_NODE;
		if (delta == 0)
			break;
		else if (delta < 0)
			node_pptr = (avl_node_t **)&(node_ptr->left);
		else
			node_pptr = (avl_node_t **)&(node_ptr->right);
	}

	delete_ptr = node_ptr;

	if (node_ptr->left == NULL) {
		/*
     * There is no node on the left subtree of delNode.
     * Either there is one (and only one, because of the balancing rules)
     * on its right subtree, and it replaces delNode, or it has no child
     * nodes at all and it just gets deleted
     */

		*node_pptr = node_ptr->right;
		ancestor_cnt--;
	} else {
		avl_node_t **delete_pptr;
		int delete_ancestor_cnt;

		delete_ancestor_cnt = ancestor_cnt;
		delete_pptr = node_pptr;
		delete_ptr = node_ptr;

		node_pptr = (avl_node_t **)&(node_ptr->left);
		while (1) {
			node_ptr = *node_pptr;
			if (node_ptr->right == NULL)
				break;
			ancestor[ancestor_cnt++] = node_pptr;
			node_pptr = (avl_node_t **)&(node_ptr->right);
		}
		/*
     * this node gets replaced by its (unique, because of balancing rules)
     * left child, or deleted if it has no childs at all
     */
		*node_pptr = node_ptr->left;

		node_ptr->left = delete_ptr->left;
		node_ptr->right = delete_ptr->right;
		node_ptr->height = delete_ptr->height;
		*delete_pptr = node_ptr;

		ancestor[delete_ancestor_cnt] =
			(avl_node_t **)&(node_ptr->left);
	}

	avl_rebalance(ancestor, ancestor_cnt);

	return delete_ptr;
}

void *avl_inheritor_get(avl_root_t root, void *key,
			int (*compare)(void *, void *))
{
	avl_node_t *node_ptr;
	avl_node_t *inheritor_ptr;

	node_ptr = root;
	inheritor_ptr = NULL;
	while (1) {
		int delta;

		if (node_ptr == NULL)
			return inheritor_ptr;
		delta = compare(node_ptr, key);
		if (delta == ERR_FAULT)
			return NULL;
		if (delta < 0) {
			node_ptr = node_ptr->left;
		} else if (delta > 0) {
			node_ptr = node_ptr->right;
		} else {
			inheritor_ptr = node_ptr;
			return inheritor_ptr;
		}
	}
}

void *avl_superior_get(avl_root_t root, void *key,
		       int (*compare)(void *, void *))
{
	avl_node_t *node_ptr;
	avl_node_t *superior_ptr;

	node_ptr = root;
	superior_ptr = NULL;
	while (1) {
		int delta;

		if (node_ptr == NULL)
			return superior_ptr;
		delta = compare(node_ptr, key);
		if (delta == ERR_FAULT)
			return NULL;
		if (delta < 0) {
			superior_ptr = node_ptr;
			node_ptr = node_ptr->left;
		} else {
			node_ptr = node_ptr->right;
		}
	}
}

void *avl_inferior_get(avl_root_t root, void *key,
		       int (*compare)(void *, void *))
{
	avl_node_t *node_ptr;
	avl_node_t *inferior_ptr;

	node_ptr = root;
	inferior_ptr = NULL;
	while (1) {
		int delta;

		if (node_ptr == NULL)
			return inferior_ptr;
		delta = compare(node_ptr, key);
		if (delta == ERR_FAULT)
			return NULL;
		if (delta > 0) {
			inferior_ptr = node_ptr;
			node_ptr = node_ptr->right;
		} else {
			node_ptr = node_ptr->left;
		}
	}
}

void *avl_minimum_get(avl_root_t root)
{
	avl_node_t *node_ptr;

	node_ptr = root;

	if (node_ptr == NULL)
		return NULL;
	while (node_ptr->left != NULL) {
		node_ptr = node_ptr->left;
	}

	return node_ptr;
}

void *avl_maximum_get(avl_root_t root)
{
	avl_node_t *node_ptr;

	node_ptr = root;

	if (node_ptr == NULL)
		return NULL;
	while (node_ptr->right != NULL) {
		node_ptr = node_ptr->right;
	}

	return node_ptr;
}

status_t avl_tree_walk(avl_root_t *root, void (*walk_exec)(avl_root_t *))
{
	if ((root == NULL) || (*root == NULL))
		return NO_ERROR;

	if ((*root)->left != NULL)
		avl_tree_walk((avl_root_t *)&((*root)->left), walk_exec);
	if ((*root)->right != NULL)
		avl_tree_walk((avl_root_t *)&((*root)->right), walk_exec);

	walk_exec(root);
	return NO_ERROR;
}

status_t avl_tree_erase(avl_root_t *root, status_t (*free)(avl_node_t *node))
{
	status_t ret;

	if ((root == NULL) || (*root == NULL))
		return NO_ERROR;

	if ((*root)->left != NULL) {
		ret = avl_tree_erase((avl_root_t *)&((*root)->left), free);
		if (ret)
			return ret;
		//ret = free((*root)->left);
		//if (ret)
			//return ret;
		(*root)->left = NULL;
	}

	if ((*root)->right != NULL) {
		ret = avl_tree_erase((avl_root_t *)&((*root)->right), free);
		if (ret)
			return ret;
		//ret = free((*root)->right);
		//if (ret)
			//return ret;
		(*root)->right = NULL;
	}

	ret = free(*root);
	if (ret)
		return ret;
	*root = NULL;

	return NO_ERROR;
}

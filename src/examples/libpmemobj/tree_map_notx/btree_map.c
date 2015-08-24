/*
 * Copyright (c) 2015, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * btree_map.c -- textbook implementation of btree /w preemptive splitting
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "tree_map.h"

//TOID_DECLARE(struct tree_map_node, TREE_MAP_TYPE_OFFSET + 1);

#define	BTREE_ORDER 4 /* can't be odd */
#define	BTREE_MIN ((BTREE_ORDER / 2) - 1) /* min number of keys per node */

struct tree_map_node_item {
	uint64_t key;
	uint64_t value;
};

#define	EMPTY_ITEM ((struct tree_map_node_item)\
{0, 0})

struct tree_map_node {
	int n; /* number of occupied slots */
	struct tree_map_node_item items[BTREE_ORDER - 1];
	struct tree_map_node *slots[BTREE_ORDER];
};

struct tree_map {
	struct tree_map_node *root;
};

/*
 * tree_map_new -- allocates a new crit-bit tree instance
 */
int
tree_map_new(PMEMobjpool *pop, struct tree_map **map)
{
	int ret = 0;
	*map = (struct tree_map *)malloc(sizeof(struct tree_map));
	return ret;
}

/*
 * tree_map_insert_item_at -- (internal) inserts an item at position
 */
static void
tree_map_insert_item_at(struct tree_map_node *node, int pos,
		struct tree_map_node_item item)
{
	node->items[pos] = item;
	node->n += 1;
}


/*
 * tree_map_insert_empty -- (internal) inserts an item into an empty node
 */
static void
tree_map_insert_empty(struct tree_map *map, struct tree_map_node_item item)
{
	map->root = (struct tree_map_node*)malloc(sizeof(struct tree_map_node));
	tree_map_insert_item_at(map->root, 0, item);
}

/*
 * tree_map_insert_item -- (internal) inserts and makes space for new item
 */
static void
tree_map_insert_item(struct tree_map_node *node, int p,
	struct tree_map_node_item item)
{
	if (node->items[p].key != 0) {
		memmove(&node->items[p + 1], &node->items[p],
		sizeof (struct tree_map_node_item) * ((BTREE_ORDER - 2 - p)));
	}
	tree_map_insert_item_at(node, p, item);
}


/*
 * tree_map_is_empty -- checks whether the tree map is empty
 */
int
tree_map_is_empty(struct tree_map *map)
{
	return ((map->root == NULL) || (map->root->n == 0));
}

/*
 * tree_map_insert_node -- (internal) inserts and makes space for new node
 */
static void
tree_map_insert_node(struct tree_map_node *node, int p,
	struct tree_map_node_item item,
	struct tree_map_node *left, struct tree_map_node *right)
{
	if (node && node->items[p].key != 0) { /* move all existing data */
		memmove(&node->items[p + 1], &node->items[p],
		sizeof (struct tree_map_node_item) * ((BTREE_ORDER - 2 - p)));

		memmove(&node->slots[p + 1], &node->slots[p],
		sizeof (struct tree_map_node) * ((BTREE_ORDER - 1 - p)));
	}
	node->slots[p] = left;
	node->slots[p + 1] = right;
	tree_map_insert_item_at(node, p, item);
}

/*
 * tree_map_create_split_node -- (internal) splits a node into two
 */
static struct tree_map_node*
tree_map_create_split_node(struct tree_map_node *node,
	struct tree_map_node_item *m)
{
	struct tree_map_node *right = (struct tree_map_node *)malloc
			(sizeof(struct tree_map_node));

	int c = (BTREE_ORDER / 2);
	*m =node->items[c - 1]; /* select median item */
	node->items[c - 1] = EMPTY_ITEM;

	/* move everything right side of median to the new node */
	for (int i = c; i < BTREE_ORDER; ++i) {
		if (i != BTREE_ORDER - 1) {
			right->items[right->n++] =
				node->items[i];

			node->items[i] = EMPTY_ITEM;
		}
		right->slots[i - c] = node->slots[i];
		node->slots[i] = NULL;
	}
	node->n = c - 1;
	return right;
}

/*
 * tree_map_find_dest_node -- (internal) finds a place to insert the new key at
 */
static struct tree_map_node*
tree_map_find_dest_node(struct tree_map *map, struct tree_map_node *n,
	struct tree_map_node *parent, uint64_t key, int *p)
{
	if (n->n == BTREE_ORDER - 1) { /* node is full, perform a split */
		struct tree_map_node_item m;
		struct tree_map_node *right =
			tree_map_create_split_node(n, &m);

		if (parent != NULL) {
			tree_map_insert_node(parent, *p, m, n, right);
			if (key > m.key) /* select node to continue search */
				n = right;
		} else { /* replacing root node, the tree grows in height */
			struct tree_map_node *up =
				(struct tree_map_node *)malloc(sizeof(struct tree_map_node));
			up->n = 1;
			up->items[0] = m;
			up->slots[0] = n;
			up->slots[1] = right;
			map->root = up;
			n = up;
		}
	}

	for (int i = 0; i < BTREE_ORDER; ++i) {
		if (i == BTREE_ORDER - 1 || n->items[i].key == 0 ||
			n->items[i].key > key) {
			*p = i;
			return (n->slots[i] == NULL) ? n :
				tree_map_find_dest_node(map,
					n->slots[i], n, key, p);
		}
	}

	return NULL;
}

/*
 * tree_map_insert -- inserts a new key-value pair into the map
 */
int
tree_map_insert(PMEMobjpool *pop,
		struct tree_map *map, uint64_t key, uint64_t value)
{
	struct tree_map_node_item item = {key, value};

	if (tree_map_is_empty(map)) {
		tree_map_insert_empty(map, item);
	} else {
		int p; /* position at the dest node to insert */
		struct tree_map_node *parent = NULL;
		struct tree_map_node *dest =
				tree_map_find_dest_node(map, map->root,
						parent, key, &p);

		tree_map_insert_item(dest, p, item);
	}

	return 0;
}




/*
 * tree_map_rotate_right -- (internal) takes one element from right sibling
 */
static void
tree_map_rotate_right(struct tree_map_node *rsb,
	struct tree_map_node *node,
	struct tree_map_node *parent, int p)
{
	/* move the separator from parent to the deficient node */
	struct tree_map_node_item sep =parent->items[p];
	tree_map_insert_item(node, node->n, sep);

	/* the first element of the right sibling is the new separator */
	parent->items[p] = rsb->items[0];

	/* the nodes are not necessarily leafs, so copy also the slot */
	node->slots[node->n] = rsb->slots[0];

	rsb->n -= 1; /* it loses one element, but still > min */

	/* move all existing elements back by one array slot */
	memmove(rsb->items, rsb->items + 1,
		sizeof (struct tree_map_node_item) * (rsb->n));
	memmove(rsb->slots, rsb->slots + 1,
		sizeof (struct tree_map_node) * (rsb->n + 1));
}

/*
 * tree_map_rotate_left -- (internal) takes one element from left sibling
 */
static void
tree_map_rotate_left(struct tree_map_node *lsb,
	struct tree_map_node *node,
	struct tree_map_node *parent, int p)
{
	/* move the separator from parent to the deficient node */
	struct tree_map_node_item sep =parent->items[p - 1];
	tree_map_insert_item(node, 0, sep);

	/* the last element of the left sibling is the new separator */
	parent->items[p - 1] = lsb->items[lsb->n - 1];


	/* rotate the node children */
	memmove(node->slots + 1, node->slots,
		sizeof (struct tree_map_node) * (node->n));

	/* the nodes are not necessarily leafs, so copy also the slot */
	node->slots[0] = lsb->slots[lsb->n];
	lsb->n -= 1; /* it loses one element, but still > min */
}

/*
 * tree_map_merge -- (internal) merges node and right sibling
 */
static void
tree_map_merge(struct tree_map *map, struct tree_map_node *rn,
	struct tree_map_node *node,
	struct tree_map_node *parent, int p)
{
	struct tree_map_node_item sep = parent->items[p];

	/* add separator to the deficient node */
	node->items[node->n++] = sep;

	/* copy right sibling data to node */
	memcpy(&node->items[node->n], rn->items,
	sizeof (struct tree_map_node_item) * rn->n);
	memcpy(&node->slots[node->n], rn->slots,
	sizeof (struct tree_map_node) * (rn->n));

	node->n += rn->n;

	printf("Crashes first time \n");

	free(rn); /* right node is now empty */
	rn= NULL;

	parent->n -= 1;

	/* move everything to the right of the separator by one array slot */
	memmove(parent->items + p, parent->items + p + 1,
	sizeof (struct tree_map_node_item) * (parent->n - p));

	memmove(parent->slots + p + 1, parent->slots + p + 2,
	sizeof (struct tree_map_node) * (parent->n - p + 1));

	/* if the parent is empty then the tree shrinks in height */
	if (parent->n == 0 && (parent == map->root)) {
		free(map->root);
		map->root = node;
	}
}

/*
 * tree_map_rebalance -- (internal) performs tree rebalance
 */
static void
tree_map_rebalance(struct tree_map *map, struct tree_map_node *node,
	struct tree_map_node *parent, int p)
{
	struct tree_map_node *rsb = p >= parent->n ?
		NULL : parent->slots[p + 1];
	struct tree_map_node *lsb = p == 0 ?
		NULL: parent->slots[p - 1];

	if (rsb && rsb->n > BTREE_MIN)
		tree_map_rotate_right(rsb, node, parent, p);
	else if (lsb && lsb->n > BTREE_MIN)
		tree_map_rotate_left(lsb, node, parent, p);
	else if (rsb==NULL) /* always merge with rightmost node */
		tree_map_merge(map, node, lsb, parent, p - 1);
	else
		tree_map_merge(map, rsb, node, parent, p);
}

/*
 * tree_map_remove_from_node -- (internal) removes element from node
 */
static void
tree_map_remove_from_node(struct tree_map *map,
	struct tree_map_node *node,
	struct tree_map_node *parent, int p)
{
	if (node->slots[0] == NULL) { /* leaf */
		if (p == BTREE_ORDER - 2)
			node->items[p] = EMPTY_ITEM;
		else if (node->n != 1) {
			memmove(&node->items[p],
				&node->items[p + 1],
				sizeof (struct tree_map_node_item) *
				(node->n - p));
		}
		node->n -= 1;
		return;
	}

	/* can't delete from non-leaf nodes, remove from right child */
	struct tree_map_node *rchild = node->slots[p + 1];

	if(rchild != NULL) {
		node->items[p] = rchild->items[0];
		tree_map_remove_from_node(map, rchild, node, 0);
		if (rchild->n < BTREE_MIN) /* right child can be deficient now */
			tree_map_rebalance(map, rchild, node, p + 1);
	}else{
		return;
	}
}

/*
 * tree_map_remove_item -- (internal) removes item from node
 */
static int
tree_map_remove_item(struct tree_map *map, struct tree_map_node *node,
	struct tree_map_node *parent, uint64_t key, int p)
{
	int ret = 0;
	int i = 0;

	if(node == NULL) return -1;

	for (; i <= node->n; ++i) {

		if (i == node->n || node->items[i].key > key) {
			ret = tree_map_remove_item(map, node->slots[i],
				node, key, i);
			break;
		} else if (node->items[i].key == key) {
			tree_map_remove_from_node(map, node, parent, i);
			ret = node->items[i].value;
			break;
		}
	}

	/* check for deficient nodes walking up */
	if (parent && node->n < BTREE_MIN)
		tree_map_rebalance(map, node, parent, p);

	return ret;
}

/*
 * tree_map_remove -- removes key-value pair from the map
 */
int
tree_map_remove(PMEMobjpool *pop, struct tree_map *map, uint64_t key)
{
	int ret = tree_map_remove_item(map, map->root,NULL, key, 0);
	return ret;
}

#if 0
/*
 * tree_map_clear_node -- (internal) removes all elements from the node
 */
static void
tree_map_clear_node(struct tree_map_node *node)
{
	for (int i = 0; i < node->n; ++i) {
		tree_map_clear_node(node->slots[i]);
	}

	free(node);
}
#endif


/*
 * tree_map_get_from_node -- (internal) searches for a value in the node
 */
static uint64_t
tree_map_get_from_node(struct tree_map_node *node, uint64_t key)
{
	for (int i = 0; i <= node->n; ++i)
		if (i == node->n || node->items[i].key > key)
			return tree_map_get_from_node(
				node->slots[i], key);
		else if (node->items[i].key == key)
			return node->items[i].value;

	return 0;
}

/*
 * tree_map_get -- searches for a value of the key
 */
uint64_t
tree_map_get(struct tree_map *map, uint64_t key)
{
	return tree_map_get_from_node(map->root, key);
}

/*
 * tree_map_foreach_node -- (internal) recursively traverses tree
 */
static int
tree_map_foreach_node(struct tree_map_node *p,
	int (*cb)(uint64_t key, uint64_t, void *arg), void *arg)
{
	if (p== NULL)
		return 0;

	for (int i = 0; i <= p->n; ++i) {
		if (tree_map_foreach_node(p->slots[i], cb, arg) != 0)
			return 1;

		printf("%u\n",(unsigned int)p->items[i].key);
		if (i != p->n && p->items[i].key != 0) {
			//printf("%u\n",(unsigned int)D_RO(p)->items[i].key);
			if (cb(p->items[i].key, p->items[i].value,
					arg) != 0)
				return 1;
		}
	}
	return 0;
}

/*
 * tree_map_foreach -- initiates recursive traversal
 */
int
tree_map_foreach(struct tree_map *map,
	int (*cb)(uint64_t key, uint64_t value, void *arg), void *arg)
{
	return tree_map_foreach_node(map->root, cb, arg);
}


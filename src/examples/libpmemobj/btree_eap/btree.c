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
 * btree.c -- implementation of persistent binary search tree
 */
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libpmemobj.h>

POBJ_LAYOUT_BEGIN(btree);
POBJ_LAYOUT_ROOT(btree, struct btree);
POBJ_LAYOUT_TOID(btree, struct btree_node);
POBJ_LAYOUT_END(btree);

#define	POOLSIZE 1024*1024*1024
#define ITEM_COUNT 50000
#define	KEYLEN 64
#define	VALUELEN 64

unsigned int nr_inserts;
unsigned int nr_find_failure;
unsigned int nr_find_success;

struct btree_node {
	int64_t key;
	TOID(struct btree_node) slots[2];
	struct btree_node *directp;
	char value[];

};

struct btree {
	TOID(struct btree_node) root;
};

struct btree_node_arg {
	size_t size;
	int64_t key;
	const char *value;
};

/*
 * btree_node_construct -- constructor of btree node
 */
void
btree_node_construct(PMEMobjpool *pop, void *ptr, void *arg)
{
	struct btree_node *node = ptr;
	struct btree_node_arg *a = arg;

	node->key = a->key;
	strcpy(node->value, a->value);
	node->slots[0] = TOID_NULL(struct btree_node);
	node->slots[1] = TOID_NULL(struct btree_node);

	pmemobj_persist(pop, node, a->size);
}

/*
 * btree_node_construct -- constructor of btree node
 */
TOID(struct btree_node)
btree_node_construct1(PMEMobjpool *pop, void *arg)
{

	struct btree_node_arg *a = arg;

	TOID(struct btree_node) node = TX_NEW(struct btree_node);

	struct btree_node *node1= D_RW(node);

	node1->directp = node1;
	node1->key = a->key;
	strcpy(node1->value, a->value);
	node1->slots[0] = TOID_NULL(struct btree_node);
	node1->slots[1] = TOID_NULL(struct btree_node);
	return node;

}



/*
 * btree_insert -- inserts new element into the tree
 */
void
btree_insert(PMEMobjpool *pop, int64_t key, const char *value)
{
	TOID(struct btree) btree = POBJ_ROOT(pop, struct btree);
	TOID(struct btree_node) *dst = &D_RW(btree)->root;
	TOID(struct btree_node) node;
	int i=0;

	while (!TOID_IS_NULL(*dst)) {

		if(key > D_RO(*dst)->key) i = 1;
				else i=0;

		dst = &D_RW(*dst)->slots[i];
	}

	struct btree_node_arg args = {
			.size = sizeof (struct btree_node) + strlen(value) + 1,
			.key = key,
			.value = value
	};

	//POBJ_ALLOC(pop, dst, struct btree_node, args.size,
		//	btree_node_construct, &args);
		node = btree_node_construct1(pop, &args);
		*dst = node;

}

/*
 * btree_find -- searches for key in the tree
 */
const char *
btree_find(PMEMobjpool *pop, int64_t key)
{
	TOID(struct btree) btree = POBJ_ROOT(pop, struct btree);
	TOID(struct btree_node) node = D_RO(btree)->root;
	int i=0;

	while (!TOID_IS_NULL(node)) {
		if (D_RO(node)->key == key){
			return D_RO(node)->value;
		}
		else {

			if(key > D_RO(node)->key) i = 1;
			else i=0;

			node = D_RO(node)->slots[i];
		}
	}

	return NULL;
}

/*
 * btree_node_print -- prints content of the btree node
 */
void
btree_node_print(const TOID(struct btree_node) node)
{
	printf("%ld %s\n", D_RO(node)->key, D_RO(node)->value);
}

/*
 * btree_foreach -- invoke callback for every node
 */
void
btree_foreach(PMEMobjpool *pop, const TOID(struct btree_node) node,
		void(*cb)(const TOID(struct btree_node) node))
{
	if (TOID_IS_NULL(node))
		return;

	btree_foreach(pop, D_RO(node)->slots[0], cb);

	cb(node);

	btree_foreach(pop, D_RO(node)->slots[1], cb);
}

/*
 * btree_print -- initiates foreach node print
 */
void
btree_print(PMEMobjpool *pop)
{
	TOID(struct btree) btree = POBJ_ROOT(pop, struct btree);

	btree_foreach(pop, D_RO(btree)->root, btree_node_print);
}


void gen_random(char *s, const int len) {
	static const char alphanum[] =
			"0123456789"
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			"abcdefghijklmnopqrstuvwxyz";

	for (int i = 0; i < len; ++i) {
		s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
	}

	s[len] = 0;
}


void operation(PMEMobjpool *pop, uint64_t key_64,
		char *value, int op)
{
	switch (op) {
	case 'p':
		btree_print(pop);
		break;
	case 'i':
		//gen_random(value, VALUELEN);
		if(value){
			TX_BEGIN(pop) {
			btree_insert(pop, key_64, (const char *)value);
			nr_inserts++;
			}TX_END
		}
		break;
	case 'f':
		if ((value = (char *)btree_find(pop, key_64)) != NULL){
			//printf("%s\n", value);
			nr_find_success++;
		}else{
			nr_find_failure++;
		}
		break;
	default:
		printf("invalid operation\n");
		break;
	}

}

int main(int argc, char *argv[])
{
	char value[VALUELEN];
	uint64_t key_64;

	if (argc < 2) {
		printf("usage: %s file-name [p|i|f] [key] [value] \n", argv[0]);
		return 1;
	}

	const char *path = argv[1];

	PMEMobjpool *pop;

	if (access(path, F_OK) != 0) {
		if ((pop = pmemobj_create(path, POBJ_LAYOUT_NAME(btree),
				POOLSIZE, 0666)) == NULL) {
			fprintf(stderr,"failed to create pool %s\n", path);
			return 1;
		}
	} else {
		if ((pop = pmemobj_open(path,
				POBJ_LAYOUT_NAME(btree))) == NULL) {
			fprintf(stderr,"failed to open pool%s\n", path);
			return 1;
		}
	}


	int i=0;
	/*****************************************************************************/
	/* Insertion */
	for (i = 0; i < ITEM_COUNT; i++)
	{
		bzero(value, VALUELEN);
		key_64 = i;
		operation(pop,key_64, value,'i');
	}
	/*****************************************************************************/
	/* Find/Search */
	for (i = 0; i < ITEM_COUNT; i++)
	{
		key_64 = i;
		//operation(pop,key_64, value,'f');
		//fprintf(stdout,"key %lu value %s \n", key_64, value);

	}
	/*****************************************************************************/

	fprintf(stderr,"nr_inserts %u, nr_find_success %u,  nr_find_failures %u\n",
			nr_inserts, nr_find_success, nr_find_failure);
	pmemobj_close(pop);

	return 0;
}

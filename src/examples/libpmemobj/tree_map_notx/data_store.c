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
 * data_store.c -- tree_map example usage
 */

#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include "tree_map.h"

//#define TEST_RESTART

POBJ_LAYOUT_BEGIN(data_store);
POBJ_LAYOUT_ROOT(data_store, struct store_root);
POBJ_LAYOUT_TOID(data_store, struct store_item);
POBJ_LAYOUT_END(data_store);

#define	MAX_INSERTS 6
#define ELEMENTSZ 512

static uint64_t nkeys;
static uint64_t keys[MAX_INSERTS];

struct store_item {
	uint64_t item_data;
};

struct store_root {
	struct tree_map *map;
};

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



/*
 * get_keys -- inserts the keys of the items by key order (sorted, descending)
 */
int
get_keys(uint64_t key, uint64_t value, void *arg)
{
	keys[nkeys++] = key;

	return 0;
}

/*
 * dec_keys -- decrements the keys count for every item
 */
int
dec_keys(uint64_t key, uint64_t value, void *arg)
{
	nkeys--;
	return 0;
}

int main(int argc, const char *argv[]) {
	if (argc < 2) {
		printf("usage: %s file-name\n", argv[0]);
		return 1;
	}

	const char *path = argv[1];

	PMEMobjpool *pop;
	srand(time(NULL));

	if (access(path, F_OK) != 0) {
		if ((pop = pmemobj_create(path, POBJ_LAYOUT_NAME(data_store),
			PMEMOBJ_MIN_POOL, 0666)) == NULL) {
			fprintf(stderr,"failed to create pool %s\n", path);
			return 1;
		}
	//	return 0;
	} else {
		if ((pop = pmemobj_open(path,
				POBJ_LAYOUT_NAME(data_store))) == NULL) {
			fprintf(stderr,"failed to open pool %s\n", path);
			return 1;
		}
#ifdef TEST_RESTART
		TOID(struct store_root) root = POBJ_ROOT(pop, struct store_root);
		/* count the items */
		if (!TOID_IS_NULL(D_RO(root)->map)){
			tree_map_foreach(D_RO(root)->map, get_keys, NULL);
			exit(0);
		}
#endif
	}

	struct store_root *root =
			(struct store_root*)malloc(sizeof(struct store_root));


#if 0
	if (!TOID_IS_NULL(D_RO(root)->map)) /* delete the map if it exists */
		tree_map_delete(pop, &D_RW(root)->map);
#endif

		int i =0;
	/* insert random items in a transaction */
		tree_map_new(pop,&(root->map));

		for (i = 0; i < MAX_INSERTS; ++i) {
			//TX_BEGIN(pop) {
			//fprintf(stdout,"calling transactions %u\n",i);
			/* new_store_item is transactional! */
			tree_map_insert(pop, root->map, i+1,i+1);
			//}TX_END
		}
			printf("calling transactions finish %u\n", i);

#if 1
	/* count the items */
	//if(D_RO(root)->map)
	tree_map_foreach(root->map, get_keys, NULL);
#endif


	int ret = 0;
	printf("nkeys %lu\n", nkeys);

	/* remove the items without outer transaction */
	for (int i = 0; i < nkeys; ++i) {

		printf("trying to remove %ld\n", keys[i]);
		ret = tree_map_remove(pop, root->map, keys[i]);
		printf("remove return %u\n",ret);
		//assert(!OID_IS_NULL(item));
		//assert(OID_INSTANCEOF(item, struct store_item));
	}

#ifndef TEST_RESTART
#if 0
	uint64_t old_nkeys = nkeys;

	/* tree should be empty */
	tree_map_foreach(D_RO(root)->map, dec_keys, NULL);
	assert(old_nkeys == nkeys);
#endif
#endif //TEST_RESTART

	pmemobj_close(pop);

	return 0;
}

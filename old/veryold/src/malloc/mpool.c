#include "zc.h"
//#define set_used_ring set_ring
typedef struct {
	unsigned short int element_unused_list_id:16;
	ZRING ring;
	ZRBTREE_NODE rbnode;
	unsigned short int element_unused_sum;
	char *element_list;
} ZMPOOL_SET;

typedef struct {
	unsigned short int e_id;
} ZMPOOL_USI;

static int zmpool_set_cmp(ZRBTREE_NODE * rn1, ZRBTREE_NODE * rn2)
{
	ZMPOOL_SET *set1, *set2;

	set1 = ZCONTAINER_OF(rn1, ZMPOOL_SET, rbnode);
	set2 = ZCONTAINER_OF(rn2, ZMPOOL_SET, rbnode);

	return (set1->element_list - set2->element_list);
}

static ZMPOOL_SET *zmpool_set_create(ZMPOOL * zmp)
{
	int i;
	char *data;
	ZMPOOL_SET *set;
	ZMPOOL_USI *usi;

	data = (char *)zmalloc(sizeof(ZMPOOL_SET) + zmp->element_size * zmp->element_count_per_set);
	set = (ZMPOOL_SET *) data;
	memset(set, 0, sizeof(ZMPOOL_SET));
	set->element_list = data + sizeof(ZMPOOL_SET);
	for (i = 0; i < zmp->element_count_per_set; i++) {
		usi = (ZMPOOL_USI *) (set->element_list + zmp->element_size * i);
		usi->e_id = (unsigned short int)(i + 1);
	}
	set->element_unused_list_id = 0;
	set->element_unused_sum = zmp->element_count_per_set;

	return set;
}

ZMPOOL *zmpool_create(int element_size, int element_count_per_set, int element_unused_limit)
{
	ZMPOOL *zmp;

	if (element_size < sizeof(unsigned short int)) {
		zlog_fatal("zmpool_create: element_size must > %d ", (int)(sizeof(unsigned short int)));
	}
	zmp = (ZMPOOL *) zmalloc(sizeof(ZMPOOL));
	memset(zmp, 0, sizeof(ZMPOOL));
	zrbtree_init(&(zmp->rbtree), zmpool_set_cmp);
	zring_init(&(zmp->set_ring));
	zring_init(&(zmp->set_used_ring));
	zring_init(&(zmp->set_unused_ring));

	zmp->element_size = element_size;
	if (element_count_per_set == 0) {
		if (element_size < 512) {
			element_count_per_set = 4096 / element_size;
		} else {
			element_count_per_set = 8;
		}
	}
	zmp->element_count_per_set = element_count_per_set;

	if (element_unused_limit < zmp->element_count_per_set * 1.3 + 1) {
		element_unused_limit = zmp->element_count_per_set * 1.5 + 1;
	}
	zmp->element_unused_limit = element_unused_limit;

	return zmp;
}

void *zmpool_alloc_one(ZMPOOL * zmp)
{
	ZMPOOL_SET *set;
	ZRING *rg;
	ZMPOOL_USI *usi;
	unsigned short int e_id;
	char *rp;

	rg = zmp->set_ring.next;
	set = ZCONTAINER_OF(rg, ZMPOOL_SET, ring);
	if ((rg == &(zmp->set_ring)) || (set->element_unused_list_id == zmp->element_count_per_set)) {
		if (zmp->set_unused_ring.next == &(zmp->set_unused_ring)) {
			set = zmpool_set_create(zmp);
			zrbtree_attach(&(zmp->rbtree), &(set->rbnode));
			zring_prepend(&(zmp->set_unused_ring), &(set->ring));
			zmp->element_unused_sum += zmp->element_count_per_set;
			zmp->set_sum++;
		}
		rg = zmp->set_unused_ring.next;
		set = ZCONTAINER_OF(rg, ZMPOOL_SET, ring);
		zring_detach(rg);
		zring_prepend(&(zmp->set_ring), rg);
	}
	zmp->element_unused_sum--;

	rg = zmp->set_ring.next;
	set = ZCONTAINER_OF(rg, ZMPOOL_SET, ring);
	e_id = set->element_unused_list_id;
	rp = (char *)(set->element_list + e_id * zmp->element_size);
	usi = (ZMPOOL_USI *) rp;
	set->element_unused_list_id = usi->e_id;
	set->element_unused_sum--;

	if (!(set->element_unused_sum)) {
		zring_detach(rg);
		zring_append(&(zmp->set_used_ring), rg);
	}

	return ((void *)rp);
}

void zmpool_free_one(ZMPOOL * zmp, void *rp)
{
	ZMPOOL_SET *set, rs;
	ZRBTREE_NODE *rn;
	ZRING *rg;
	ZMPOOL_USI *usi;

	rs.element_list = (char *)rp;
	rn = zrbtree_near_prev(&(zmp->rbtree), &(rs.rbnode));
	set = ZCONTAINER_OF(rn, ZMPOOL_SET, rbnode);

	usi = (ZMPOOL_USI *) rp;
	usi->e_id = set->element_unused_list_id;
	set->element_unused_list_id = (unsigned short int)((((char *)rp) - set->element_list) / zmp->element_size);
	set->element_unused_sum++;

	zmp->element_unused_sum++;

	rg = &(set->ring);
	if ((set->element_unused_sum == zmp->element_count_per_set)) {
		zring_detach(rg);
		zring_prepend(&(zmp->set_unused_ring), rg);
	} else {
		zring_detach(rg);
		zring_append(&(zmp->set_ring), rg);
	}

	if ((zmp->element_unused_sum > zmp->element_unused_limit) && (&(zmp->set_unused_ring) != zmp->set_unused_ring.next)) {
		rg = zmp->set_unused_ring.next;
		set = ZCONTAINER_OF(rg, ZMPOOL_SET, ring);
		zrbtree_detach(&(zmp->rbtree), &(set->rbnode));
		zring_detach(rg);
		zmp->element_unused_sum -= zmp->element_count_per_set;
		zmp->set_sum -= 1;
		zfree(set);
	}
}

void zmpool_free(ZMPOOL * zmp)
{
	ZRBTREE_NODE *rn;
	ZMPOOL_SET *set;

	while ((rn = zrbtree_first(&(zmp->rbtree)))) {
		set = ZCONTAINER_OF(rn, ZMPOOL_SET, rbnode);
		zrbtree_detach(&(zmp->rbtree), rn);
		zfree(set);
	}

	zfree(zmp);
}

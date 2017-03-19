#include "zc.h"

static int zidict_rbtree_cmp(ZRBTREE_NODE * n1, ZRBTREE_NODE * n2)
{
	ZIDICT_NODE *dn1, *dn2;

	dn1 = ZCONTAINER_OF(n1, ZIDICT_NODE, rbnode);
	dn2 = ZCONTAINER_OF(n2, ZIDICT_NODE, rbnode);

	return (dn1->key - dn2->key);
}

ZIDICT *zidict_create(void)
{
	ZIDICT *zd;
	zd = (ZIDICT *) zmalloc(sizeof(ZIDICT));
	zd->len = 0;
	zrbtree_init(&(zd->rbtree), zidict_rbtree_cmp);

	return zd;
}

ZIDICT_NODE *zidict_attach(ZIDICT * zd, ZIDICT_NODE * node)
{
	ZIDICT_NODE *cmp_n;
	ZRBTREE_NODE *rn, *cmp_rn, *ret_rn;
	int cmp_result;

	rn = &(node->rbnode);

	ret_rn = 0;
	ZRBTREE_ATTACH_PART1(&(zd->rbtree), rn, cmp_rn);

	cmp_n = ZCONTAINER_OF(cmp_rn, ZIDICT_NODE, rbnode);
	cmp_result = node->key - cmp_n->key;

	ZRBTREE_ATTACH_PART2(&(zd->rbtree), rn, cmp_result, ret_rn);

	if (rn != ret_rn) {
		return ZCONTAINER_OF(ret_rn, ZIDICT_NODE, rbnode);
	}

	zd->len++;
	return node;
}

ZIDICT_NODE *zidict_detach(ZIDICT * zd, ZIDICT_NODE * node)
{
	ZRBTREE_DETACH(&(zd->rbtree), &(node->rbnode));
	zd->len--;

	return node;
}

ZIDICT_NODE *zidict_enter(ZIDICT * zd, int key, void *value, ZIDICT_NODE ** ret_node)
{
	ZIDICT_NODE n, *nn;

	n.key = key;
	nn = zidict_attach(zd, &n);

	if (nn != &n) {
		if (ret_node) {
			*ret_node = nn;
		}
		return 0;
	}
	nn = zidict_node_create();
	nn->key = key;
	nn->value = value;

	zrbtree_replace_node(&(zd->rbtree), &(n.rbnode), &(nn->rbnode));

	return nn;

}

ZIDICT_NODE *zidict_lookup(ZIDICT * zd, int key, char **value)
{
	ZIDICT_NODE *cmp_n;
	ZRBTREE_NODE *cmp_rn /* and return_rn */ ;
	int cmp_result;

	ZRBTREE_LOOKUP_PART1(&(zd->rbtree), cmp_rn);

	cmp_n = ZCONTAINER_OF(cmp_rn, ZIDICT_NODE, rbnode);
	cmp_result = key - cmp_n->key;

	ZRBTREE_LOOKUP_PART2(&(zd->rbtree), cmp_result, cmp_rn);
	if (cmp_rn == 0) {
		return 0;
	}
	cmp_n = ZCONTAINER_OF(cmp_rn, ZIDICT_NODE, rbnode);
	if (value) {
		*value = cmp_n->value;
	}

	return cmp_n;
}

ZIDICT_NODE *zidict_remove(ZIDICT * zd, int key, char **old_value)
{
	ZIDICT_NODE *n;

	n = zidict_lookup(zd, key, old_value);
	if (!n) {
		return 0;
	}
	ZRBTREE_DETACH(&(zd->rbtree), &(n->rbnode));
	zidict_node_free(n);

	return n;
}

ZIDICT_NODE *zidict_first(ZIDICT * zd)
{
	ZRBTREE_NODE *rn = zrbtree_first(&(zd->rbtree));
	if (rn) {
		return ZCONTAINER_OF(rn, ZIDICT_NODE, rbnode);
	}
	return 0;
}

ZIDICT_NODE *zidict_last(ZIDICT * zd)
{
	ZRBTREE_NODE *rn = zrbtree_last(&(zd->rbtree));
	if (rn) {
		return ZCONTAINER_OF(rn, ZIDICT_NODE, rbnode);
	}
	return 0;
}

ZIDICT_NODE *zidict_prev(ZIDICT_NODE * node)
{
	ZRBTREE_NODE *rn = zrbtree_prev(&(node->rbnode));
	if (rn) {
		return ZCONTAINER_OF(rn, ZIDICT_NODE, rbnode);
	}
	return 0;
}

ZIDICT_NODE *zidict_next(ZIDICT_NODE * node)
{
	ZRBTREE_NODE *rn = zrbtree_next(&(node->rbnode));
	if (rn) {
		return ZCONTAINER_OF(rn, ZIDICT_NODE, rbnode);
	}
	return 0;
}

void zidict_walk(ZIDICT * zd, void (*walk_fn) (ZIDICT_NODE *, void *), void *ctx)
{
	ZRBTREE_NODE *rnode;
	ZIDICT_NODE *inode;

	if (!walk_fn) {
		return;
	}
	ZRBTREE_WALK_BEGIN(&(zd->rbtree), rnode) {
		inode = ZCONTAINER_OF(rnode, ZIDICT_NODE, rbnode);
		(*walk_fn) (inode, ctx);
	}
	ZRBTREE_WALK_END;
}

void zidict_free(ZIDICT * zd, void (*free_fn) (ZIDICT_NODE *, void *), void *ctx)
{
	ZIDICT_NODE *n;

	while ((n = zidict_first(zd))) {
		zidict_detach(zd, n);
		if (free_fn) {
			(*free_fn) (n, ctx);
		}
		zfree(n);
	}
	zfree(zd);
}

void zidict_free_STR(ZIDICT * zd)
{
	ZIDICT_NODE *n;

	while ((n = zidict_first(zd))) {
		zidict_detach(zd, n);
		if (n->value) {
			zfree(n->value);
		}
		zfree(n);
	}
	zfree(zd);
}

void zidict_free2(ZIDICT * zd, void (*free_fn) (ZIDICT_NODE *, void *), void *ctx)
{
	ZIDICT_NODE *n;

	while ((n = zidict_first(zd))) {
		zidict_detach(zd, n);
		if (free_fn) {
			(*free_fn) (n, ctx);
		}
	}
	zfree(zd);
}

void zidict_free2_STR(ZIDICT * zd)
{
	ZIDICT_NODE *n;

	while ((n = zidict_first(zd))) {
		zidict_detach(zd, n);
		if (n->value) {
			zfree(n->value);
		}
	}
	zfree(zd);
}

int zidict_keys(ZIDICT * zd, int *key_list, int size)
{
	ZRBTREE_NODE *rnode;
	ZIDICT_NODE *node;
	int i;

	i = 0;
	ZRBTREE_WALK_BEGIN(&(zd->rbtree), rnode) {
		node = ZCONTAINER_OF(rnode, ZIDICT_NODE, rbnode);
		if (i == size) {
			goto end;
		}
		i++;
		*key_list = ZIDICT_KEY(node);
		key_list++;
	}
	ZRBTREE_WALK_END;

      end:

	return i;
}

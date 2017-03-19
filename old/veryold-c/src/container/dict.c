#include "zc.h"

static int zdict_rbtree_cmp(ZRBTREE_NODE * n1, ZRBTREE_NODE * n2)
{
	ZDICT_NODE *dn1, *dn2;

	dn1 = ZCONTAINER_OF(n1, ZDICT_NODE, rbnode);
	dn2 = ZCONTAINER_OF(n2, ZDICT_NODE, rbnode);

	return strcmp(dn1->key, dn2->key);
}

ZDICT *zdict_create(void)
{
	ZDICT *zd;
	zd = (ZDICT *) zmalloc(sizeof(ZDICT));
	zd->len = 0;
	zrbtree_init(&(zd->rbtree), zdict_rbtree_cmp);

	return zd;
}

ZDICT_NODE *zdict_attach(ZDICT * zd, ZDICT_NODE * node)
{
	ZDICT_NODE *cmp_n;
	ZRBTREE_NODE *rn, *cmp_rn /* and return_rn */ ;
	int cmp_result;

	rn = &(node->rbnode);

	ZRBTREE_ATTACH_PART1(&(zd->rbtree), rn, cmp_rn);

	cmp_n = ZCONTAINER_OF(cmp_rn, ZDICT_NODE, rbnode);
	cmp_result = strcmp(node->key, cmp_n->key);

	ZRBTREE_ATTACH_PART2(&(zd->rbtree), rn, cmp_result, cmp_rn);

	if (rn != cmp_rn) {
		return ZCONTAINER_OF(cmp_rn, ZDICT_NODE, rbnode);
	}

	zd->len++;
	return node;
}

ZDICT_NODE *zdict_detach(ZDICT * zd, ZDICT_NODE * node)
{
	ZRBTREE_DETACH(&(zd->rbtree), &(node->rbnode));
	zd->len--;

	return node;
}

ZDICT_NODE *zdict_enter(ZDICT * zd, char *key, void *value, ZDICT_NODE ** ret_node)
{
	ZDICT_NODE n, *nn;

	n.key = key;
	nn = zdict_attach(zd, &n);

	if (nn != &n) {
		if (ret_node) {
			*ret_node = nn;
		}
		return 0;
	}
	nn = zdict_node_create();
	nn->key = zstr_strdup(key);
	nn->value = value;

	zrbtree_replace_node(&(zd->rbtree), &(n.rbnode), &(nn->rbnode));

	return nn;

}

ZDICT_NODE *zdict_lookup(ZDICT * zd, char *key, char **value)
{
	ZDICT_NODE *cmp_n;
	ZRBTREE_NODE *cmp_rn /* and return_rn */ ;
	int cmp_result;

	ZRBTREE_LOOKUP_PART1(&(zd->rbtree), cmp_rn);

	cmp_n = ZCONTAINER_OF(cmp_rn, ZDICT_NODE, rbnode);
	cmp_result = strcmp(key, cmp_n->key);

	ZRBTREE_LOOKUP_PART2(&(zd->rbtree), cmp_result, cmp_rn);
	if (cmp_rn == 0) {
		return 0;
	}
	cmp_n = ZCONTAINER_OF(cmp_rn, ZDICT_NODE, rbnode);
	if (value) {
		*value = cmp_n->value;
	}

	return cmp_n;
}

ZDICT_NODE *zdict_remove(ZDICT * zd, char *key, char **old_value)
{
	ZDICT_NODE *n;

	n = zdict_lookup(zd, key, old_value);
	if (!n) {
		return 0;
	}
	ZRBTREE_DETACH(&(zd->rbtree), &(n->rbnode));
	zfree(n->key);
	zdict_node_free(n);

	return n;
}

ZDICT_NODE *zdict_first(ZDICT * zd)
{
	ZRBTREE_NODE *rn = zrbtree_first(&(zd->rbtree));
	if (rn) {
		return ZCONTAINER_OF(rn, ZDICT_NODE, rbnode);
	}
	return 0;
}

ZDICT_NODE *zdict_last(ZDICT * zd)
{
	ZRBTREE_NODE *rn = zrbtree_last(&(zd->rbtree));
	if (rn) {
		return ZCONTAINER_OF(rn, ZDICT_NODE, rbnode);
	}
	return 0;
}

ZDICT_NODE *zdict_prev(ZDICT_NODE * node)
{
	ZRBTREE_NODE *rn = zrbtree_prev(&(node->rbnode));
	if (rn) {
		return ZCONTAINER_OF(rn, ZDICT_NODE, rbnode);
	}
	return 0;
}

ZDICT_NODE *zdict_next(ZDICT_NODE * node)
{
	ZRBTREE_NODE *rn = zrbtree_next(&(node->rbnode));
	if (rn) {
		return ZCONTAINER_OF(rn, ZDICT_NODE, rbnode);
	}
	return 0;
}

ZDICT_NODE *zdict_enter_STR(ZDICT * zd, char *key, char *str)
{
	ZDICT_NODE *ndn, *dn;
	char *new_str;

	new_str = zstr_strdup(str);
	if ((ndn = zdict_enter(zd, key, new_str, &dn))) {
		return ndn;
	}
	zfree(ZDICT_VALUE(dn));
	ZDICT_UPDATE_VALUE(dn, new_str);

	return dn;
}

ZDICT_NODE *zdict_remove_STR(ZDICT * zd, char *key)
{
	ZDICT_NODE *n = 0;
	char *value;

	if ((n = zdict_remove(zd, key, &value))) {
		zstr_free(value);
	}

	return n;
}

void zdict_walk(ZDICT * zd, void (*walk_fn) (ZDICT_NODE *, void *), void *ctx)
{
	ZRBTREE_NODE *rnode;
	ZDICT_NODE *node;

	if (!walk_fn) {
		return;
	}
	ZRBTREE_WALK_BEGIN(&(zd->rbtree), rnode) {
		node = ZCONTAINER_OF(rnode, ZDICT_NODE, rbnode);
		(*walk_fn) (node, ctx);
	}
	ZRBTREE_WALK_END;
}

void zdict_free(ZDICT * zd, void (*free_fn) (ZDICT_NODE *, void *), void *ctx)
{
	ZDICT_NODE *n;

	while ((n = zdict_first(zd))) {
		zdict_detach(zd, n);
		if (free_fn) {
			(*free_fn) (n, ctx);
		}
		zstr_free(n->key);
		zfree(n);
	}
	zfree(zd);
}

void zdict_free_STR(ZDICT * zd)
{
	ZDICT_NODE *n;

	while ((n = zdict_first(zd))) {
		zdict_detach(zd, n);
		if (n->value) {
			zstr_free(n->value);
		}
		zstr_free(n->key);
		zfree(n);
	}
	zfree(zd);
}

void zdict_free2(ZDICT * zd, void (*free_fn) (ZDICT_NODE *, void *), void *ctx)
{
	ZDICT_NODE *n;

	while ((n = zdict_first(zd))) {
		zdict_detach(zd, n);
		if (free_fn) {
			(*free_fn) (n, ctx);
		}
	}
	zfree(zd);
}

void zdict_free2_STR(ZDICT * zd)
{
	ZDICT_NODE *n;

	while ((n = zdict_first(zd))) {
		zdict_detach(zd, n);
		if (n->value) {
			zfree(n->value);
		}
	}
	zfree(zd);
}

int zdict_keys(ZDICT * zd, char **key_list, int size)
{
	ZRBTREE_NODE *rnode;
	ZDICT_NODE *node;
	int i;

	i = 0;
	ZRBTREE_WALK_BEGIN(&(zd->rbtree), rnode) {
		node = ZCONTAINER_OF(rnode, ZDICT_NODE, rbnode);
		if (i == size) {
			goto end;
		}
		i++;
		*key_list = ZDICT_KEY(node);
		key_list++;
	}
	ZRBTREE_WALK_END;

      end:

	return i;
}

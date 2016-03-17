#include "zc.h"
#include "test_lib.h"

typedef struct MYKV MYKV;
typedef struct MYKV_NODE MYKV_NODE;
struct MYKV {
	ZRBTREE *rbtree;	/* should use struct-block instead of struct-pointer */
};
struct MYKV_NODE {
	char *key;
	void *value;
	ZRBTREE_NODE rbnode;
};

int cmp_fn(ZRBTREE_NODE * n1, ZRBTREE_NODE * n2)
{
	MYKV_NODE *mn1, *mn2;

	mn1 = ZCONTAINER_OF(n1, MYKV_NODE, rbnode);
	mn2 = ZCONTAINER_OF(n2, MYKV_NODE, rbnode);

	return strcmp(mn1->key, mn2->key);
}

MYKV *mykv_create(int size)
{
	MYKV *kv;
	ZRBTREE *rbtree;

	kv = (MYKV *) zmalloc(sizeof(MYKV));
	rbtree = (ZRBTREE *) zmalloc(sizeof(ZRBTREE));
	zrbtree_init(rbtree, cmp_fn);
	kv->rbtree = rbtree;
	return kv;
}

MYKV_NODE *mykv_insert(MYKV * kv, char *k, void *v)
{
	MYKV_NODE *n;
	ZRBTREE_NODE *rn, *cmp_rn /* and return_rn */ ;

	n = (MYKV_NODE *) zmalloc(sizeof(MYKV_NODE));
	n->key = zstr_strdup(k);
	n->value = v;

	rn = &(n->rbnode);

	cmp_rn = zrbtree_attach(kv->rbtree, rn);
	if (cmp_rn != rn) {
		zfree(n->key);
		zfree(n);
		return ZCONTAINER_OF(cmp_rn, MYKV_NODE, rbnode);
	}

	return n;
}

MYKV_NODE *mykv_find(MYKV * kv, char *k, void **value)
{
	MYKV_NODE n, *cmp_n;
	ZRBTREE_NODE *rn, *cmp_rn /* and return_rn */ ;

	n.key = k;
	rn = &(n.rbnode);

	cmp_rn = zrbtree_lookup(kv->rbtree, rn);
	if (!cmp_rn) {
		return 0;
	}
	cmp_n = ZCONTAINER_OF(cmp_rn, MYKV_NODE, rbnode);
	if (value) {
		*value = cmp_n->value;
	}

	return cmp_n;
}

MYKV_NODE *mykv_near_prev(MYKV * kv, char *k, void **value)
{
	MYKV_NODE n, *cmp_n;
	ZRBTREE_NODE *rn, *cmp_rn /* and return_rn */ ;

	n.key = k;
	rn = &(n.rbnode);

	cmp_rn = zrbtree_near_prev(kv->rbtree, rn);
	if (!cmp_rn) {
		return 0;
	}
	cmp_n = ZCONTAINER_OF(cmp_rn, MYKV_NODE, rbnode);
	if (value) {
		*value = cmp_n->value;
	}

	return cmp_n;
}

MYKV_NODE *mykv_delete(MYKV * kv, char *k, void **value)
{
	MYKV_NODE *n;

	n = mykv_find(kv, k, value);
	if (!n) {
		return 0;
	}
	ZRBTREE_DETACH(kv->rbtree, &(n->rbnode));
	zfree(n);

	return n;
}

void free_value(void *v)
{
	ZARGV *av = (ZARGV *) v;
	if (av) {
		zargv_free(av);
	}
}

void rb_walk(ZRBTREE_NODE * rn, void *fv)
{
	typedef void (*FREE_FN) (void *);
	MYKV_NODE *mn;

	FREE_FN fff = (FREE_FN) fv;
	mn = ZCONTAINER_OF(rn, MYKV_NODE, rbnode);

	if (fv) {
		(*fff) (mn->value);
	}
	zfree(mn);
}

void mykv_free(MYKV * kv, void (*free_fn) (void *))
{
	zrbtree_fini(kv->rbtree, rb_walk, free_fn);
}

int main(int argc, char **argv)
{
	MYKV *rbs;
	ZRBTREE_NODE *rn;
	ZARGV *lav, *av;
	int i;

	rbs = mykv_create(1024 * 1024);

	zlog_info("load /etc/passwd to rbtree which'key is user name");
	test_load_passwd(0, &lav);

	for (i = 0; i < lav->argc; i++) {
		av = zargv_split(lav->argv[i], ":");
		if (ZARGV_LEN(av) == 0) {
			zargv_free(av);
			continue;
		}
		mykv_insert(rbs, av->argv[0], av);
	}

	zargv_free(lav);

	zlog_info("find rbtree_node which'key is 'mail'");
	if (mykv_find(rbs, "mail", (void **)&av)) {
		zlog_info("find out:");
		for (i = 0; i < av->argc; i++) {
			zlog_info("\targv[%d]=%s", i, av->argv[i]);
		}
	}
	zlog_info("find one rbtree_node which'key is smaller than 'mail2'");
	if (mykv_near_prev(rbs, "mail2", (void **)&av)) {
		zlog_info("find out:");
		for (i = 0; i < av->argc; i++) {
			zlog_info("\targv[%d]=%s", i, av->argv[i]);
		}
	}

	zlog_info("test ZRBTREE_WALK_MACRO");
	i = 0;
	ZRBTREE_WALK_FORWARD_BEGIN(rbs->rbtree, rn) {
		if (i++ % 5 == 0) {
			/* Must not to be display all */
			zlog_info("key: %s", ZCONTAINER_OF(rn, MYKV_NODE, rbnode)->key);
		}
	}
	ZRBTREE_WALK_FORWARD_END;
	mykv_free(rbs, free_value);
	return (0);
}

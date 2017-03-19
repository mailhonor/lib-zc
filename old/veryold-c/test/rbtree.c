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

MYKV *mykv_create(int size)
{
	MYKV *kv;
	ZRBTREE *rbtree;

	kv = (MYKV *) zmalloc(sizeof(MYKV));
	rbtree = (ZRBTREE *) zmalloc(sizeof(ZRBTREE));
	ZRBTREE_INIT(rbtree, 0);
	kv->rbtree = rbtree;
	return kv;
}

MYKV_NODE *mykv_insert(MYKV * kv, char *k, void *v)
{
	MYKV_NODE *n, *cmp_n;
	ZRBTREE_NODE *rn, *cmp_rn /* and return_rn */ ;
	int cmp_result;

	n = (MYKV_NODE *) zmalloc(sizeof(MYKV_NODE));
	n->key = zstr_strdup(k);
	n->value = v;

	rn = &(n->rbnode);

	ZRBTREE_ATTACH_PART1(kv->rbtree, rn, cmp_rn);

	cmp_n = ZCONTAINER_OF(cmp_rn, MYKV_NODE, rbnode);
	cmp_result = strcmp(n->key, cmp_n->key);

	ZRBTREE_ATTACH_PART2(kv->rbtree, rn, cmp_result, cmp_rn);

	if (rn != cmp_rn) {
		/*
		 * Already exist.
		 * rn is equal cmp_rn on the context.
		 * We donot deal this condition in the testing.
		 */
		zfree(n->key);
		zfree(n);
		return ZCONTAINER_OF(cmp_rn, MYKV_NODE, rbnode);
	}
	return n;
}

MYKV_NODE *mykv_find(MYKV * kv, char *k, void **value)
{
	MYKV_NODE *cmp_n;
	ZRBTREE_NODE *cmp_rn /* and return_rn */ ;
	int cmp_result;

	ZRBTREE_LOOKUP_PART1(kv->rbtree, cmp_rn);

	cmp_n = ZCONTAINER_OF(cmp_rn, MYKV_NODE, rbnode);
	cmp_result = strcmp(k, cmp_n->key);

	ZRBTREE_LOOKUP_PART2(kv->rbtree, cmp_result, cmp_rn);
	if (cmp_rn == 0) {
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

void mykv_free(MYKV * kv, void (*free_fn) (void *))
{
}

int walk_i = 0;
void walk_fn(ZRBTREE_NODE * n, void *ctx)
{
	if (walk_i++ % 5 == 1) {
		/* Must not to be display all */
		zlog_info("key: %s", ZCONTAINER_OF(n, MYKV_NODE, rbnode)->key);
	}
}

void free_value(void *v)
{
	ZARGV *av = (ZARGV *) v;
	if (av) {
		zargv_free(av);
	}
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

	zlog_info("test ZRBTREE_WALK_MACRO");
	i = 0;
	ZRBTREE_WALK_FORWARD_BEGIN(rbs->rbtree, rn) {
		if (i++ % 5 == 0) {
			/* Must not to be display all */
			zlog_info("key: %s", ZCONTAINER_OF(rn, MYKV_NODE, rbnode)->key);
		}
	}
	ZRBTREE_WALK_FORWARD_END;
	zlog_info("test walk ...............");
	zrbtree_walk(rbs->rbtree, walk_fn, 0);
	mykv_free(rbs, free_value);
	return (0);
}

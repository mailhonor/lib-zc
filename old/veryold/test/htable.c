#include "zc.h"
#include "test_lib.h"

typedef struct MYKV MYKV;
typedef struct MYKV_NODE MYKV_NODE;
struct MYKV {
	ZHTABLE *ht;
};
struct MYKV_NODE {
	char *key;
	void *value;
	ZHTABLE_NODE hn;
};

int mykv_cmp(ZHTABLE_NODE * node1, ZHTABLE_NODE * node2)
{
	MYKV_NODE *mn1, *mn2;
	mn1 = ZCONTAINER_OF(node1, MYKV_NODE, hn);
	mn2 = ZCONTAINER_OF(node2, MYKV_NODE, hn);
	return (strcmp(mn1->key, mn2->key));
}

int mykv_hash(ZHTABLE_NODE * node, int size)
{
	MYKV_NODE *mn;
	unsigned long hash;
	const char *s;

	mn = ZCONTAINER_OF(node, MYKV_NODE, hn);
	s = mn->key;
	ZHTABLE_BUF_HASH(hash, 0, s, *s);

	return (hash % size);
}

MYKV *mykv_create(int size)
{
	MYKV *kv;
	ZHTABLE *ht;

	kv = (MYKV *) zmalloc(sizeof(MYKV));
	ht = (ZHTABLE *) zmalloc(sizeof(ZHTABLE));
	zhtable_init(ht, size, mykv_cmp, mykv_hash);
	kv->ht = ht;
	return kv;
}

MYKV_NODE *mykv_insert(MYKV * kv, char *k, void *v)
{
	MYKV_NODE *n;

	n = (MYKV_NODE *) zmalloc(sizeof(MYKV_NODE));
	n->key = zstr_strdup(k);
	n->value = v;

	zhtable_attach(kv->ht, &(n->hn));

	return n;
}

MYKV_NODE *mykv_find(MYKV * kv, char *k, void **value)
{
	MYKV_NODE n, *np;
	ZHTABLE_NODE *hn;

	n.key = k;
	hn = zhtable_lookup(kv->ht, &n.hn);
	if (hn == 0) {
		return 0;
	}
	np = ZCONTAINER_OF(hn, MYKV_NODE, hn);
	if (value) {
		*value = np->value;
	}

	return np;
}

MYKV_NODE *mykv_delete(MYKV * kv, char *k, void **value)
{
	MYKV_NODE n, *np;
	ZHTABLE_NODE *hn;

	n.key = k;
	hn = zhtable_remove(kv->ht, &(n.hn));
	if (hn == 0) {
		return 0;
	}
	np = ZCONTAINER_OF(hn, MYKV_NODE, hn);
	if (value) {
		*value = np->value;
	}

	return np;
}

static void _mykv_free_node(ZHTABLE_NODE * hn, void *ctx)
{
	typedef void (*FREE_FN) (void *);

	MYKV_NODE *mn;
	FREE_FN free_fn = (FREE_FN) ctx;

	mn = ZCONTAINER_OF(hn, MYKV_NODE, hn);
	free_fn(mn->value);
	zfree(mn);
}

void mykv_free(MYKV * kv, void (*free_fn) (void *))
{
	zhtable_fini(kv->ht, _mykv_free_node, free_fn);
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
	MYKV *hs;
	ZHTABLE_NODE *hn;
	ZARGV *lav, *av;
	int i;

	hs = mykv_create(1024 * 1024);

	zlog_info("load /etc/passwd to htable which'key is user name");
	test_load_passwd(0, &lav);

	for (i = 0; i < lav->argc; i++) {
		av = zargv_split(lav->argv[i], ":");
		if (ZARGV_LEN(av) == 0) {
			zargv_free(av);
			continue;
		}
		mykv_insert(hs, av->argv[0], av);
	}

	zargv_free(lav);

	zlog_info("find htable_node which'key is 'mail'");
	if (mykv_find(hs, "mail", (void **)&av)) {
		zlog_info("find out:");
		for (i = 0; i < av->argc; i++) {
			zlog_info("\targv[%d]=%s", i, av->argv[i]);
		}
	}

	zlog_info("test ZHTABLE_WALK_MACRO");
	i = 0;
	ZHTABLE_WALK_BEGIN(hs->ht, hn) {
		if (i++ % 5 == 0) {
			/* Must not to be display all */
			zlog_info("key: %s", ZCONTAINER_OF(hn, MYKV_NODE, hn)->key);
		}
	}
	ZHTABLE_WALK_END;
	mykv_free(hs, free_value);
	return (0);
}

#include "zc.h"
#include "test_lib.h"

void free_value(ZIDICT_NODE * node, void *ctx)
{
}

void free_value2(ZIDICT_NODE * node, void *ctx)
{
}

int walk_i = 0;
void walk_fn(ZIDICT_NODE * node, void *ctx)
{
	if (walk_i++ == 100) {
		zlog_info("key: %d", node->key);
	}
}

int main(int argc, char **argv)
{
	int test_len = 1024 * 1024;
	ZIDICT *dict;
	ZIDICT_NODE *node;
	int i, m, *mp;

	zlog_info("testing ...");
	dict = zidict_create();
	test_usetime("test1");
	for (i = 0; i < test_len; i++) {
		m = i % 12345;
		zidict_enter(dict, i, ZINT_TO_VOID_PTR(m), 0);
	}
	if (zidict_lookup(dict, 123781, (char **)&mp)) {
		zlog_info("find out 123781: value: %d", ZCHAR_PTR_TO_INT(mp));
	}

	zidict_free(dict, free_value, 0);
	test_usetime("test1 end");

	zlog_info("testing2 ...");
	dict = zidict_create();
	test_usetime("test2");
	ZIDICT_NODE *node_list = (ZIDICT_NODE *) zcalloc(sizeof(ZIDICT_NODE), test_len);
	for (i = 0; i < test_len; i++) {
		m = i % 12345;
		node = node_list + i;
		node->key = i;
		node->value = ZINT_TO_VOID_PTR(m);
		zidict_attach(dict, node);
	}
	if (zidict_lookup(dict, 123781, (char **)&mp)) {
		zlog_info("find out 123781: value: %d", ZCHAR_PTR_TO_INT(mp));
	}

	zidict_walk(dict, walk_fn, 0);
	zidict_free2(dict, free_value2, 0);
	zfree(node_list);
	test_usetime("test2 end");

	return (0);
}

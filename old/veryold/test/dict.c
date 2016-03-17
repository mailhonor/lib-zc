#include "zc.h"
#include "test_lib.h"

void free_value(ZDICT_NODE * node, void *ctx)
{
	ZARGV *av = (ZARGV *) node->value;
	if (av) {
		zargv_free(av);
	}
}

int walk_i = 0;
void walk_fn(ZDICT_NODE * node, void *ctx)
{
	if (walk_i % 5 == 1)
		zlog_info("%5d, %s", walk_i, node->key);
	walk_i++;
}

int main(int argc, char **argv)
{
	ZDICT *dict;
	ZARGV *lav, *av;
	int i;

	dict = zdict_create();

	zlog_info("load /etc/passwd to zdict which'key is user name");
	test_load_passwd(0, &lav);

	for (i = 0; i < lav->argc; i++) {
		av = zargv_split(lav->argv[i], ":");
		if (ZARGV_LEN(av) == 0) {
			zargv_free(av);
			continue;
		}
		zdict_enter(dict, av->argv[0], av, 0);
	}

	zargv_free(lav);

	zlog_info("find zdict_node which'key is 'mail'");
	if (zdict_lookup(dict, "mail", (char **)&av)) {
		zlog_info("find out:");
		for (i = 0; i < av->argc; i++) {
			zlog_info("\targv[%d]=%s", i, av->argv[i]);
		}
	}

	zdict_walk(dict, walk_fn, 0);
	zlog_info("first key: %s", zdict_first(dict)->key);
	zdict_free(dict, free_value, 0);
	return (0);
}

#include "zc.h"
#include "test_lib.h"

void _show_kv(ZDICT * zc, int level)
{
	char *name, *value;

	ZDICT_CONFIG_WALK_BEGIN(zc, name, value) {
		if (level == 2) {
			printf("\t");
		}
		printf("%s = %s\n", name, value);
	}
	ZDICT_CONFIG_WALK_END;
}

int main(int argc, char **argv)
{

	char *fn;
	ZDICT *zc;
	ZDICT *child;
	char *name;

	test_init(argc, argv);

	if (argc == 1) {
		fn = "data/config.conf";
	} else {
		fn = argv[1];
	}

	zc = zdict_create();
	zdict_config_load_file(zc, fn);

	_show_kv(zc, 1);
	ZDICT_CONFIG_WALK_CHILD_BEGIN(zc, name, child) {
		printf("[%s]\n", name);
		_show_kv(child, 2);
	}
	ZDICT_CONFIG_WALK_CHILD_END;

	if (zdict_config_get_child(zc, "black_ip")) {
		printf("find\n");
	} else {
		printf("not find\n");
	}

	zdict_config_free(zc);

	return 0;
}

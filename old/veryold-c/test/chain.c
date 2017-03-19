#include "zc.h"

void walk_fn(ZCHAIN_NODE * n)
{
	printf("%s\n", ((ZARGV *) (n->value))->argv[0]);
}

void free_fn(void *v, void *ctx)
{
	zargv_free((ZARGV *) v);
}

int main(int argc, char **argv)
{
	FILE *fp;
	ZCHAIN *zc;
	ZCHAIN_NODE *n;
	char *p;
	ZARGV *av;

	zc = zchain_create();
	fp = fopen("/etc/passwd", "r");
	while ((p = (char *)malloc(1024), fgets(p, 1024, fp))) {
		av = zargv_split(p, ":");
		zchain_push(zc, (char *)av);
	}
	zlog_debug("now test zchain_walk");
	zchain_walk(zc, walk_fn);
	zlog_debug("now test ZCHAIN_WALK_BEGIN");
	ZCHAIN_WALK_BEGIN(zc, n) {
		zlog_debug("%s", ((ZARGV *) (n->value))->argv[0]);
	}
	ZCHAIN_WALK_END;
	zchain_free(zc, free_fn, 0);

	return (0);
}

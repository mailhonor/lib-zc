#include "zc.h"
#include "test_lib.h"

int main(int argc, char **argv)
{
	char *p;
	int size;
	ZSTACK_BUF(zb, 102400);
	ZSTACK_BUF(zb2, 102400);

	ZSTACK_SDLIST(sdl, 1024);
	ZSTACK_SDLIST(sdl2, 1024);
	ZARGV *av;

	test_load_passwd(0, &av);

	ZARGV_WALK_BEGIN(av, p) {
		zsdlist_add(sdl, p, strlen(p));
	} ZARGV_WALK_END;

	zsdlist_escape(sdl, zb);
	zsdlist_parse_sizedata(sdl2, ZBUF_DATA(zb), ZBUF_LEN(zb));
	zsdlist_terminate(sdl2);

	ZSDLIST_WALK_BEGIN(sdl2, p, size) {
		printf("size:%5d, p=%s\n", size, p);
	} ZSDLIST_WALK_END;

	return 0;
}

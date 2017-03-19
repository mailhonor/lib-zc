#include "zc.h"
#include "test_lib.h"

int main(int argc, char **argv)
{
	ZBUF *zb;
	ZARGV *plist;
	int i;
	char *p;

	zb = zbuf_create(1);
	test_load_passwd(0, &plist);
	for (i = 0; i < ZARGV_LEN(plist); i++) {
		p = plist->argv[i];
		zbuf_sprintf(zb, "readline: %s ", p);
		ZBUF_PUT(zb, 'A');
		ZBUF_PUT(zb, ' ');
		zbuf_strncat(zb, "NOOOOOOOOOOOOOOOOOOOOOO", 3);
		zbuf_sprintf(zb, ": %5d ", i);
		ZBUF_PUT(zb, '\n');
	}
	printf("%s", ZBUF_DATA(zb));

	return (0);
}

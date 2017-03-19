#include "zc.h"

int zsio_get_delimiter(ZSIO * fp, ZBUF * zb, char delimiter)
{
	int i, c;

	for (i = 0;; i++) {
		c = ZSIO_GETCHAR(fp);
		if (c < 0) {
			return i;
		}
		ZBUF_PUT(zb, c);
		if (c == (int)delimiter) {
			return (i + 1);
		}
	}

	return i;
}

int zsio_get_n(ZSIO * fp, ZBUF * zb, int len)
{
	int i, c;

	for (i = 0; i < len; i++) {
		c = ZSIO_GETCHAR(fp);
		if (c < 0) {
			return i;
		}
		ZBUF_PUT(zb, c);
	}

	return i;
}

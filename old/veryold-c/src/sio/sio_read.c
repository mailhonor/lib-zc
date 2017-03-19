#include "zc.h"

int zsio_getchar(ZSIO * fp)
{
	return ZSIO_GETCHAR(fp);
}

int zsio_read(ZSIO * fp, void *buf, int len)
{
	ZBUF *rbuf;
	char *p;
	int i, c, ret;

	rbuf = &(fp->rbuf);
	if (rbuf->len == 0) {
		ret = zsio_read_ready(&(fp->rbuf));
		if (ret < 0) {
			return ret;
		}
	}
	p = (char *)buf;
	for (i = 0; i < len && rbuf->len > 0; i++) {
		c = ZSIO_GETCHAR(fp);
		if (c < 0) {
			return i;
		}
		*p++ = c;
	}

	return i;
}

int zsio_read_n(ZSIO * fp, void *buf, int len)
{
	char *p;
	int i, c;

	if (len < 1) {
		return ZSIO_ERROR;
	}

	p = (char *)buf;
	for (i = 0; i < len; i++) {
		c = ZSIO_GETCHAR(fp);
		if (c < 0) {
			return c;
		}
		*p++ = c;
	}

	return len;
}

int zsio_read_delimiter(ZSIO * fp, void *buf, int len, char delimiter)
{
	char *p;
	int i, c;

	if (len < 1) {
		return ZSIO_ERROR;
	}

	p = (char *)buf;
	for (i = 0; i < len; i++) {
		c = ZSIO_GETCHAR(fp);
		if (c < 0) {
			return c;
		}
		*p++ = c;
		if (c == (int)delimiter) {
			return (i + 1);
		}
	}

	return len;
}

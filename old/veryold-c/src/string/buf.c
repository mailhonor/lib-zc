#include "zc.h"

int zbuf_get(ZBUF * zb)
{
	if (!zb->ready) {
		return ZBUF_EOF;
	}
	return (zb->ready(zb) ? ZBUF_EOF : ZBUF_GET(zb));
}

int zbuf_put(ZBUF * zb, int ch)
{
	if (zb->len < zb->size) {
		return ZBUF_PUT(zb, ch);
	}
	if (!zb->ready) {
		return ZBUF_EOF;
	}
	return (zb->ready(zb) ? ZBUF_EOF : ZBUF_PUT(zb, ch));
}

int zbuf_read(ZBUF * zb, void *buf, int len)
{
	int count;
	char *cbuf;

	cbuf = (char *)buf;
	for (count = 0; count < len; count++) {
		if ((cbuf[count] = ZBUF_GET(zb)) < 0) {
			break;
		}
	}

	return (count);
}

int zbuf_write(ZBUF * zb, void *buf, int len)
{
	int count;
	char *cbuf;

	cbuf = (char *)buf;
	for (count = 0; count < len; count++) {
		if (ZBUF_PUT(zb, cbuf[count]) < 0) {
			break;
		}
	}

	return (count);
}

ZBUF *zbuf_create_buf(void)
{
	ZBUF *zb;

	zb = (ZBUF *) zcalloc(1, sizeof(ZBUF));
	return zb;
}

void zbuf_free_buf(ZBUF * zb)
{
	zfree(zb);
}

void zbuf_reset(ZBUF * zb)
{
	ZBUF_RESET(zb);
}

void zbuf_terminate(ZBUF * zb)
{
	ZBUF_TERMINATE(zb);
}

void zbuf_truncate(ZBUF * zb, int len)
{
	ZBUF_TRUNCATE(zb, len);
}

#include "zc.h"

int zbuf_string_space_extend(ZBUF * zb, int incr)
{
	if (incr < zb->size) {
		incr = zb->size;
	}
	zb->size += incr;
	zb->data = (char *)zrealloc(zb->data, zb->size + 1);
	zb->ptr = zb->data + zb->len;

	return 0;
}

int zbuf_string_write_ready(ZBUF * zb)
{
	return zbuf_string_space_extend(zb, 0);
}

int zbuf_string_space_ready(ZBUF * zb, int want)
{
	int incr;

	incr = want - (zb->size - zb->len);
	if (incr > 0) {
		if (!zb->ready) {
			return -1;
		}
		return zbuf_string_space_extend(zb, incr);
	}

	return 0;
}

ZBUF *zbuf_create(int size)
{
	ZBUF *zb;

	zb = (ZBUF *) zcalloc(1, sizeof(ZBUF));
	zb->flags = 0;
	if (size < 13) {
		size = 13;
	}
	zb->size = size;
	zb->len = 0;
	zb->data = (char *)zmalloc(size + 1);
	zb->ptr = zb->data;
	zb->ready = zbuf_string_write_ready;
	zb->space = zbuf_string_space_ready;

	return zb;
}

void zbuf_free(ZBUF * zb)
{
	zfree(zb->data);
	zfree(zb);
}

int zbuf_strncpy(ZBUF * zb, char *src, int len)
{
	ZBUF_RESET(zb);
	if (len < 1) {
		return 0;
	}
	while (len-- && *src) {
		ZBUF_PUT(zb, *src);
		src++;
	}
	ZBUF_TERMINATE(zb);

	return (zb->len);
}

int zbuf_strcpy(ZBUF * zb, char *src)
{
	ZBUF_RESET(zb);
	while (*src) {
		ZBUF_PUT(zb, *src);
		src++;
	}
	ZBUF_TERMINATE(zb);

	return (zb->len);
}

int zbuf_strncat(ZBUF * zb, char *src, int len)
{
	int old;

	old = zb->len;
	while (len-- && *src) {
		ZBUF_PUT(zb, *src);
		src++;
	}
	ZBUF_TERMINATE(zb);

	return (zb->len - old);
}

int zbuf_strcat(ZBUF * zb, char *src)
{
	int old;

	old = zb->len;
	while (*src) {
		ZBUF_PUT(zb, *src);
		src++;
	}
	ZBUF_TERMINATE(zb);

	return (zb->len - old);
}

int zbuf_memcpy(ZBUF * zb, char *src, int len)
{
	ZBUF_RESET(zb);
	if (len > 1024) {
		zb->len = len;
		ZBUF_SPACE(zb, len);
		memcpy(zb->data, src, len);
	} else {
		while (len--) {
			ZBUF_PUT(zb, *src);
			src++;
		}
	}
	ZBUF_TERMINATE(zb);

	return (zb->len);
}

int zbuf_memcat(ZBUF * zb, char *src, int len)
{
	int old;

	old = zb->len;
	if (len > 1024) {
		ZBUF_SPACE(zb, len);
		zb->len += len;
		memcpy(zb->data, src, len);
	} else {
		while (len--) {
			ZBUF_PUT(zb, *src);
			src++;
		}
	}
	ZBUF_TERMINATE(zb);

	return (zb->len - old);
}

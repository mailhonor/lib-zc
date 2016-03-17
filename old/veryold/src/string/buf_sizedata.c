#include "zc.h"

void zbuf_sizedata_escape(ZBUF * zb, void *data, int len)
{
	int ch, left = len;

	if (len < 0) {
		return;
	}
	do {
		ch = left & 0177;
		left >>= 7;
		if (!left) {
			ch |= 0200;
		}
		ZBUF_PUT(zb, ch);
	} while (left);
	if (len > 0) {
		zbuf_memcat(zb, data, len);
	}
}

void zbuf_sizedata_escape_int(ZBUF * zb, int i)
{
	char buf[32];
	int len;

	len = sprintf(buf, "%d", i);

	zbuf_sizedata_escape(zb, buf, len);
}

void zbuf_sizedata_escape_long(ZBUF * zb, long i)
{
	char buf[64];
	int len;

	len = sprintf(buf, "%lu", i);

	zbuf_sizedata_escape(zb, buf, len);
}

void zbuf_sizedata_escape_dict(ZBUF * zb, ZDICT * zd)
{
	ZDICT_NODE *n;
	char *k, *v;

	ZDICT_WALK_BEGIN(zd, n) {
		k = (char *)ZDICT_KEY(n);
		v = (char *)ZDICT_VALUE(n);
		zbuf_sizedata_escape(zb, k, strlen(k));
		zbuf_sizedata_escape(zb, v, strlen(v));
	}
	ZDICT_WALK_END;
}

void zbuf_sizedata_escape_pp(ZBUF * zb, char **pp, int len)
{
	int i;
	char *p;

	for (i = 0; i < len; i++) {
		p = *pp++;
		zbuf_sizedata_escape(zb, p, p ? strlen(p) : 0);
	}
}

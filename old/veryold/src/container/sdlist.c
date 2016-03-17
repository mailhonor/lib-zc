#include "zc.h"

ZSDLIST *zsdlist_create(int size)
{
	ZSDLIST *sdl;

	sdl = (ZSDLIST *) zcalloc(1, sizeof(ZSDLIST));
	sdl->ST = 0;

	sdl->list = (ZSDATA *) malloc(sizeof(ZSDATA) * size);
	sdl->size = size;
	sdl->len = 0;

	return sdl;
}

void zsdlist_free(ZSDLIST * sdl)
{
	zfree(sdl->list);
	zfree(sdl);
}

int zsdlist_add(ZSDLIST * sdl, void *data, int size)
{
	ZSDATA *sd;

	if (sdl->len >= sdl->size) {
		if (sdl->ST) {
			return -1;
		}
		sdl->size *= 2;
		sdl->list = (ZSDATA *) zrealloc(sdl->list, sizeof(ZSDATA) * sdl->size);
	}

	sd = sdl->list + sdl->len++;
	sd->data = data;
	sd->size = size;

	return 0;
}

void zsdlist_terminate(ZSDLIST * sdl)
{
	int i;
	ZSDATA *sd;

	for (i = 0; i < sdl->len; i++) {
		sd = sdl->list + i;
		((char *)(sd->data))[sd->size] = 0;
	}
}

void zsdlist_reset(ZSDLIST * sdl)
{
	sdl->len = 0;
}

int zsdlist_parse_sizedata(ZSDLIST * sdl, void *data, int size)
{
	int ret, count = 0;
	unsigned char *buf = (unsigned char *)data;
	int left = size;
	ZSDATA sd;

	while (left > 0) {
		ret = zsdata_parse(&sd, buf, left);
		if (ret < 0) {
			return -1;
		}
		zsdlist_add(sdl, sd.data, sd.size);
		buf += ret;
		left -= ret;
		count++;
	}

	return count;
}

int zsdlist_escape(ZSDLIST * sdl, ZBUF * zb)
{
	int i;

	for (i = 0; i < sdl->len; i++) {
		zsdata_escape(sdl->list + i, zb);
	}

	return ZBUF_LEN(zb);
}

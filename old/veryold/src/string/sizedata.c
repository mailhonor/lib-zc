#include "zc.h"

int zsdata_parse(ZSDATA * sd, void *data, int size)
{
	int i = 0;
	unsigned char *buf = (unsigned char *)data;
	int ch, len = 0, shift = 0;
	while (1) {
		ch = ((i++ == size) ? -1 : *buf++);
		if (ch == -1) {
			return -1;
		}
		len |= ((ch & 0177) << shift);
		if (ch & 0200) {
			break;
		}
		shift += 7;
	}
	if (i + len > size) {
		return -1;
	}
	sd->size = len;
	sd->data = buf;
	return i + len;
}

void zsdata_escape(ZSDATA * sd, ZBUF * zb)
{
	zbuf_sizedata_escape(zb, sd->data, sd->size);
}

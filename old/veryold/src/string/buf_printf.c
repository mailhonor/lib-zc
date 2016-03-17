#include "zc.h"

int zbuf_vprintf(ZBUF * zb, char *format, va_list ap)
{
	char buf[1024000], *src;
	int i, c;

	i = zstr_vsnprintf(buf, 1024000, format, ap);
	src = buf;

	while (i--) {
		c = *src;
		ZBUF_PUT(zb, c);
		src++;
	}

	return i;
}

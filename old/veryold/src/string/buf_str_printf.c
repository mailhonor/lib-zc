#include "zc.h"

int zbuf_sprintf(ZBUF * zb, char *fmt, ...)
{
	va_list ap;
	int i;

	va_start(ap, fmt);
	i = zbuf_vsprintf(zb, fmt, ap);
	va_end(ap);

	return i;
}

int zbuf_vsprintf(ZBUF * zb, char *format, va_list ap)
{
	int i;

	i = zbuf_vprintf(zb, format, ap);

	ZBUF_TERMINATE(zb);

	return i;
}

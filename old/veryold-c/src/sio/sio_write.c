#include "zc.h"

int zsio_putchar(ZSIO * fp, int inch)
{
	return ZSIO_PUTCHAR(fp, inch);
}

int zsio_write_n(ZSIO * fp, void *buf, int len)
{
	int left, pc, c;
	char *p;

	left = len;
	p = (char *)buf;
	for (left = len; left; left--) {
		c = *p;
		pc = ZSIO_PUTCHAR(fp, c);
		if (pc < 0) {
			return (pc);
		}
		p++;
	}

	return (len);
}

int zsio_vfprintf(ZSIO * fp, char *format, va_list ap)
{
	int i;

	i = zbuf_vprintf(&(fp->wbuf), format, ap);

	return i;
}

int zsio_fprintf(ZSIO * fp, char *format, ...)
{
	va_list ap;
	int i;

	va_start(ap, format);
	i = zsio_vfprintf(fp, format, ap);
	va_end(ap);

	return i;
}

int zsio_fputs(char *s, ZSIO * fp)
{
	int i, c;

	i = 0;
	while ((c = (*s))) {
		c = ZSIO_PUTCHAR(fp, c);
		if (c < 0) {
			return c;
		}
		i++;
	}

	return i;
}

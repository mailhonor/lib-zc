#include "zc.h"

int zstr_snprintf(char *str, size_t size, const char *format, ...)
{
	va_list ap;
	int i;

	va_start(ap, format);
	i = vsnprintf(str, size, format, ap);
	va_end(ap);
	if (i >= size) {
		i = size - 1;
	}

	return i;
}

int zstr_vsnprintf(char *str, size_t size, const char *format, va_list ap)
{
	int i;

	i = vsnprintf(str, size, format, ap);
	if (i >= size) {
		i = size - 1;
	}

	return i;
}

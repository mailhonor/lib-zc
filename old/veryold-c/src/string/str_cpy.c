#include "zc.h"

int zstr_strncpy(char *dest, char *src, size_t len)
{
	int ret;

	strncpy(dest, (const char *)src, len);
	ret = strlen(dest);
	dest[ret] = 0;

	return ret;
}

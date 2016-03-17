#include "zc.h"

/* */
void zstr_strtok_create(ZSTRTOK * k, char *sstr)
{
	k->sstr = sstr;
	k->len = 0;
	k->str = NULL;
}

ZSTRTOK *zstr_strtok(ZSTRTOK * k, const char *delim)
{
	char *r;

	r = k->sstr;
	r += strspn(k->sstr, delim);
	if (*r == 0)
		return (NULL);
	k->len = strcspn(r, delim);
	if (k->len == 0)
		return (NULL);
	k->sstr = r + k->len;
	k->str = r;

	return (k);
}

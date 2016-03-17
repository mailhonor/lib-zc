#include "zc.h"

#define ___SKIP(start, var, cond) for (var = start; *var && (cond); var++);
#define ___DELETE(ch) ((isascii(ch) && isspace(ch)) || iscntrl(ch))
#define ___TRIM(s) { char *p; for (p = (s) + strlen(s); p > (s) && ___DELETE(p[-1]); p--); *p = 0; }

char *zstr_trim_left(char *str)
{
	char *np;

	___SKIP(str, np, ___DELETE(*np));

	return np;
}

char *zstr_trim_right(char *str)
{

	___TRIM(str);

	return str;
}

char *zstr_trim(char *str)
{
	char *np;

	___SKIP(str, np, ___DELETE(*np));
	___TRIM(np);

	return np;
}

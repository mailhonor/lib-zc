#include "zc.h"

/* string case convert.
 * only support Enlish locale.
 */

char *zstr_tolower(char *str)
{
	char *scan = str;

	while (*scan) {
		*scan = ZCHAR_TOLOWER(*scan);
		scan++;
	}

	return (str);
}

char *zstr_toupper(char *str)
{
	char *scan = str;

	while (*scan) {
		*scan = ZCHAR_TOUPPER(*scan);
		scan++;
	}

	return (str);
}

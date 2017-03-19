#include "zc.h"

int zstr_to_bool(char *s, int def)
{
	if (!strcmp(s, "1") || !strcasecmp(s, "y") || !strcasecmp(s, "yes") || !strcasecmp(s, "true"))
		return 1;
	if (!strcmp(s, "0") || !strcasecmp(s, "n") || !strcasecmp(s, "no") || !strcasecmp(s, "false"))
		return 0;

	return def;
}

int zstr_to_second(char *s)
{
	char unit, junk;
	int intval;

	switch (sscanf(s, "%d%c%c", &intval, &unit, &junk)) {
	case 1:
		unit = 's';
	case 2:
		if (intval < 0)
			return 0;
		switch (zchar_tolower(unit)) {
		case 'w':
			return (intval * (7 * 24 * 3600));
		case 'd':
			return (intval * (24 * 3600));
		case 'h':
			return (intval * 3600);
		case 'm':
			return (intval * 60);
		case 's':
		default:
			return (intval);
		}
	}

	return 0;
}

int zstr_to_size(char *s)
{
	char unit, junk;
	int intval;

	switch (sscanf(s, "%d%c%c", &intval, &unit, &junk)) {
	case 1:
		unit = 'b';
	case 2:
		if (intval < 0)
			return 0;
		switch (tolower(unit)) {
		case 'g':
			return (intval * (1024 * 1024 * 1024));
		case 'm':
			return (intval * (1024 * 1024));
		case 'k':
			return (intval * 1024);
		case 'b':
		default:
			return (intval);
		}
	}

	return 0;
}

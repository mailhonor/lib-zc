#include "zc.h"

char *zdict_get_str(ZDICT * zd, char *name, char *def)
{
	char *value;

	if (zdict_lookup(zd, name, &value)) {
		return value;
	}

	return def;
}

int zdict_get_bool(ZDICT * zd, char *name, int def)
{
	char *value;

	value = zdict_get_str(zd, name, 0);

	if (value == 0 || *value == 0) {
		return def;
	}
	return zstr_to_bool(value, def);
}

int zdict_get_int(ZDICT * zd, char *name, int def)
{
	char *value;

	value = zdict_get_str(zd, name, 0);

	if (value == 0 || *value == 0) {
		return def;
	}
	return atoi(value);
}

int zdict_get_time(ZDICT * zd, char *name, int def)
{
	char *value;

	value = zdict_get_str(zd, name, 0);

	if (value == 0 || *value == 0) {
		return def;
	}
	return zstr_to_second(value);
}

int zdict_get_size(ZDICT * zd, char *name, int def)
{
	char *value;

	value = zdict_get_str(zd, name, 0);

	if (value == 0 || *value == 0) {
		return def;
	}
	return zstr_to_size(value);
}

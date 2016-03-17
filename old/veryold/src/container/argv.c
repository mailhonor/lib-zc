#include "zc.h"

#define ZARGV_SPACE_LEFT(a) ((a)->size - (a)->argc - 1)

ZARGV *zargv_create(int size)
{
	ZARGV *argvp;
	int sane_size;

	argvp = (ZARGV *) zmalloc(sizeof(ZARGV));
	sane_size = (size < 13 ? 13 : size);
	argvp->argv = (char **)zmalloc((sane_size + 1) * sizeof(char *));
	argvp->size = sane_size;
	argvp->argc = 0;
	argvp->argv[0] = 0;

	return (argvp);
}

ZARGV *zargv_free(ZARGV * argvp)
{
	char **cpp;

	for (cpp = argvp->argv; cpp < argvp->argv + argvp->argc; cpp++) {
		if (*cpp) {
			zfree(*cpp);
		}
	}
	zfree(argvp->argv);
	zfree(argvp);
	return (0);
}

static void zargv_extend(ZARGV * argvp)
{
	int new_size;

	new_size = argvp->size * 2;
	argvp->argv = (char **)zrealloc((char *)argvp->argv, (new_size + 1) * sizeof(char *));
	argvp->size = new_size;
}

void zargv_add(ZARGV * argvp, char *ns)
{
	if (ZARGV_SPACE_LEFT(argvp) <= 0)
		zargv_extend(argvp);
	argvp->argv[argvp->argc++] = zstr_strdup(ns ? ns : "");
	argvp->argv[argvp->argc] = 0;
}

void zargv_addn(ZARGV * argvp, char *ns, int nlen)
{
	if (ZARGV_SPACE_LEFT(argvp) <= 0)
		zargv_extend(argvp);
	argvp->argv[argvp->argc++] = zstr_strndup(ns ? ns : "", nlen);
	argvp->argv[argvp->argc] = 0;
}

void zargv_truncate(ZARGV * argvp, int len)
{
	char **cpp;

	if (len < argvp->argc) {
		for (cpp = argvp->argv + len; cpp < argvp->argv + argvp->argc; cpp++)
			zfree(*cpp);
		argvp->argc = len;
		argvp->argv[argvp->argc] = 0;
	}
}

ZARGV *zargv_split(const char *string, const char *delim)
{
	ZARGV *argvp;
	ZSTRTOK stok;

	argvp = zargv_create(1);
	zstr_strtok_create(&stok, (char *)string);
	while (zstr_strtok(&stok, delim)) {
		zargv_addn(argvp, stok.str, stok.len);
	}

	return (argvp);
}

ZARGV *zargv_split_append(ZARGV * argvp, const char *string, const char *delim)
{
	ZSTRTOK stok;

	zstr_strtok_create(&stok, (char *)string);
	while (zstr_strtok(&stok, delim))
		zargv_addn(argvp, stok.str, stok.len);

	return (argvp);
}

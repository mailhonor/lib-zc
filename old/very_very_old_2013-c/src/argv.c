#include "zyc.h"

#define ZARGV_SPACE_LEFT(a) ((a)->len - (a)->argc - 1)

ZARGV *zargv_free(ZARGV * argvp, void(*free_fn)(void *, void *), char *ptr)
{
	char **cpp;

	for (cpp = argvp->argv; cpp < argvp->argv + argvp->argc; cpp++){
		if(free_fn) (*free_fn)(*cpp, ptr);
		else z_free(*cpp);
	}
	z_free(argvp->argv);
	z_free(argvp);
	return (0);
}

ZARGV *zargv_create(int len)
{
	ZARGV *argvp;
	int sane_len;

	argvp = (ZARGV *) z_malloc(sizeof (*argvp));
	argvp->len = 0;
	sane_len = (len < 16 ? 16 : len);
	argvp->argv = (char **) z_malloc((sane_len + 1) * sizeof (char *));
	argvp->len = sane_len;
	argvp->argc = 0;
	argvp->argv[0] = 0;
	return (argvp);
}

static void zargv_extend(ZARGV * argvp)
{
	ssize_t new_len;

	new_len = argvp->len * 2;
	argvp->argv = (char **) z_realloc((char *) argvp->argv, (new_len + 1) * sizeof (char *));
	argvp->len = new_len;
}

void zargv_add(ZARGV *argvp, char *ns){
	if (ZARGV_SPACE_LEFT(argvp) <= 0)
		zargv_extend(argvp);
	argvp->argv[argvp->argc++] =z_strdup(ns?ns:"");
	argvp->argv[argvp->argc] = 0;
}

void zargv_adds(ZARGV * argvp, ...)
{
	char *arg;
	va_list ap;

	va_start(ap, argvp);
	while ((arg = va_arg(ap, char *)) != 0) {
		zargv_add(argvp, arg);
	}
	va_end(ap);
	argvp->argv[argvp->argc] = 0;
}

void zargv_addn(ZARGV *argvp, char *ns, int len){
	if (ZARGV_SPACE_LEFT(argvp) <= 0)
		zargv_extend(argvp);
	argvp->argv[argvp->argc++] =z_strndup(ns?ns:"", len);
	argvp->argv[argvp->argc] = 0;
}

void zargv_truncate(ZARGV * argvp, ssize_t len)
{
	char **cpp;
	if (len < argvp->argc) {
		for (cpp = argvp->argv + len; cpp < argvp->argv + argvp->argc; cpp++)
			z_free(*cpp);
		argvp->argc = len;
		argvp->argv[argvp->argc] = 0;
	}
}

ZARGV *zargv_split(const char *string, const char *delim)
{
	ZARGV *argvp = zargv_create(1);
	ZSTRTOK stok;
	
	z_strtok_create(&stok, (char *)string);
	while (z_strtok(&stok, delim)){
		zargv_addn(argvp, stok.str, stok.len);
	}
	return (argvp);
}

ZARGV *zargv_split_append(ZARGV * argvp, const char *string, const char *delim)
{
	ZSTRTOK stok;

	z_strtok_create(&stok, (char *)string);
	while (z_strtok(&stok, delim))
		zargv_addn(argvp, stok.str, stok.len);
	return (argvp);
}

void zargv_add_node(ZARGV * argvp, char *node){
	if (ZARGV_SPACE_LEFT(argvp) <= 0)
		zargv_extend(argvp);
	argvp->argv[argvp->argc++] = node;
	argvp->argv[argvp->argc] = 0;
}

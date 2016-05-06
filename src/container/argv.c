/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-09-29
 * ================================
 */

#include "libzc.h"

#define ___SPACE_LEFT(a) ((a)->size - (a)->argc - 1)

zargv_t *zargv_create(int size)
{
    return zargv_create_mpool(size, 0);
}

zargv_t *zargv_create_mpool(int size, zmpool_t * mpool)
{
    zargv_t *argvp;
    int sane_size;

    argvp = (zargv_t *) zmpool_malloc(mpool, sizeof(zargv_t));
    argvp->mpool = mpool;
    sane_size = (size < 13 ? 13 : size);
    argvp->argv = (char **)zmpool_malloc(mpool, (sane_size + 1) * sizeof(char *));
    argvp->size = sane_size;
    argvp->argc = 0;
    argvp->argv[0] = 0;

    return (argvp);
}

void zargv_free(zargv_t * argvp)
{
    char **cpp;

    for (cpp = argvp->argv; cpp < argvp->argv + argvp->argc; cpp++) {
        zmpool_free(argvp->mpool, *cpp);
    }
    zmpool_free(argvp->mpool, argvp->argv);
    zmpool_free(argvp->mpool, argvp);
}

static void zargv_extend(zargv_t * argvp)
{
    int new_size;

    new_size = argvp->size * 2;
#if 1
    /* Within mpool mode, realloc maybe is invalid or not recommended */
    void *old;
    old = argvp->argv;
    argvp->argv = (char **)zmpool_malloc(argvp->mpool, (new_size + 1) * sizeof(char *));
    memcpy(argvp->argv, old, argvp->size * sizeof(char *));
    zmpool_free(argvp->mpool, old);
#else
    argvp->argv = (char **)zrealloc((char *)argvp->argv, (new_size + 1) * sizeof(char *));
#endif
    argvp->size = new_size;
}

void zargv_add(zargv_t * argvp, char *ns)
{
    if (___SPACE_LEFT(argvp) <= 0)
        zargv_extend(argvp);
    argvp->argv[argvp->argc++] = zmpool_strdup(argvp->mpool, ns);
    argvp->argv[argvp->argc] = 0;
}

void zargv_addn(zargv_t * argvp, char *ns, int nlen)
{
    if (___SPACE_LEFT(argvp) <= 0)
        zargv_extend(argvp);
    argvp->argv[argvp->argc++] = zmpool_strndup(argvp->mpool, ns, nlen);
    argvp->argv[argvp->argc] = 0;
}

void zargv_truncate(zargv_t * argvp, int len)
{
    char **cpp;

    if (len < argvp->argc) {
        for (cpp = argvp->argv + len; cpp < argvp->argv + argvp->argc; cpp++)
            zmpool_free(argvp->mpool, *cpp);
        argvp->argc = len;
        argvp->argv[argvp->argc] = 0;
    }
}

void zargv_rest(zargv_t * argvp)
{
    zargv_truncate(argvp, 0);
}

zargv_t *zargv_split_append(zargv_t * argvp, char *string, char *delim)
{
    zstrtok_t stok;

    zstrtok_init(&stok, (char *)string);
    while (zstrtok(&stok, delim))
        zargv_addn(argvp, stok.str, stok.len);

    return (argvp);
}

void zargv_show(zargv_t * argvp)
{
    char *p;

    ZARGV_WALK_BEGIN(argvp, p) {
        printf("%s\n", p);
    }
    ZARGV_WALK_END;
}

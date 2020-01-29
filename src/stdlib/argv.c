/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2015-09-29
 * ================================
 */

#include "zc.h"

typedef struct zargv_mpool_t zargv_mpool_t;
struct zargv_mpool_t {
    zargv_t v;
    zmpool_t *mpool;
};

void zargv_init(zargv_t *argvp, int size);
void zargv_fini(zargv_t *argvp);

zargv_t *zargv_create(int size)
{
    zargv_t *argvp = (zargv_t *) zmalloc(sizeof(zargv_t));
    zargv_init(argvp, size);
    return (argvp);
}

void zargv_init(zargv_t *argvp, int size)
{
    int sane_size = (size < 0 ? 13 : size);
    argvp->argv = (char **)zmalloc((sane_size + 1) * sizeof(char *));
    argvp->size = sane_size;
    argvp->argc = 0;
    argvp->argv[0] = 0;

    argvp->mpool_used = 0;
}

void zargv_init_mpool(zargv_t *argvp, int size, zmpool_t *mpool)
{
    int sane_size = (size < 0 ? 13 : size);
    argvp->argv = (char **)zmpool_malloc(mpool, (sane_size + 1) * sizeof(char *));
    argvp->size = sane_size;
    argvp->argc = 0;
    argvp->argv[0] = 0;

    argvp->mpool_used = 1;
    zargv_mpool_t *ama = (zargv_mpool_t *)argvp;
    ama->mpool = mpool;
}

void zargv_fini(zargv_t *argvp)
{
    zargv_mpool_t *ama = (zargv_mpool_t *)argvp;
    char **cpp;

    for (cpp = argvp->argv; cpp < argvp->argv + argvp->argc; cpp++) {
        if (argvp->mpool_used) {
            zmpool_free(ama->mpool, *cpp);
        } else {
            zfree(*cpp);
        }
    }
    if (argvp->mpool_used) {
        zmpool_free(ama->mpool, argvp->argv);
    } else {
        zfree(argvp->argv);
    }
}


void zargv_free(zargv_t * argvp)
{
    if (argvp) {
        zargv_fini(argvp);
        zfree(argvp);
    }
}

static void zargv_extend(zargv_t * argvp)
{
    int new_size = argvp->size * 2;
    if (new_size < 1) {
        new_size = 2;
    }
    if (argvp->mpool_used) {
        zargv_mpool_t *ama = (zargv_mpool_t *)argvp;
        char **npp = (char **)zmpool_malloc(ama->mpool, (new_size + 1) * sizeof(char *));
        if (argvp->argc) {
            memcpy(npp, argvp->argv, argvp->argc * sizeof(char *));
        }
        zmpool_free(ama->mpool, argvp->argv);
        argvp->argv = npp;
    } else {
        argvp->argv = (char **)zrealloc(argvp->argv, (new_size + 1) * sizeof(char *));
    }
    argvp->size = new_size;
}

void zargv_add(zargv_t * argvp, const char *ns)
{
    if (argvp->argc >= argvp->size) {
        zargv_extend(argvp);
    }
    if (argvp->mpool_used) {
        zargv_mpool_t *ama = (zargv_mpool_t *)argvp;
        argvp->argv[argvp->argc++] = zmpool_strdup(ama->mpool, ns);
    } else {
        argvp->argv[argvp->argc++] = zstrdup(ns);
    }
    argvp->argv[argvp->argc] = 0;
}

void zargv_addn(zargv_t * argvp, const char *ns, int nlen)
{
    if (argvp->argc >= argvp->size) {
        zargv_extend(argvp);
    }
    if (argvp->mpool_used) {
        zargv_mpool_t *ama = (zargv_mpool_t *)argvp;
        argvp->argv[argvp->argc++] = zmpool_strndup(ama->mpool, ns, nlen);
    } else {
        argvp->argv[argvp->argc++] = zstrndup(ns, nlen);
    }
    argvp->argv[argvp->argc] = 0;
}

void zargv_truncate(zargv_t * argvp, int len)
{
    zargv_mpool_t *ama = (zargv_mpool_t *)argvp;
    char **cpp;

    if (len < argvp->argc) {
        for (cpp = argvp->argv + len; cpp < argvp->argv + argvp->argc; cpp++) {
            if (argvp->mpool_used) {
                zmpool_free(ama->mpool, *cpp);
            } else {
                zfree(*cpp);
            }
        }
        argvp->argc = len;
        argvp->argv[argvp->argc] = 0;
    }
}

zargv_t *zargv_split_append(zargv_t * argvp, const char *string, const char *delim)
{
    zstrtok_t stok;
    if (argvp == 0) {
        argvp = zargv_create(-1);
    }

    zstrtok_init(&stok, (char *)string);
    while (zstrtok(&stok, delim)) {
        zargv_addn(argvp, stok.str, stok.len);
    }

    return (argvp);
}

void zargv_debug_show(zargv_t * argvp)
{
    ZARGV_WALK_BEGIN(argvp, p) {
        printf("%s\n", p);
    }
    ZARGV_WALK_END;
}

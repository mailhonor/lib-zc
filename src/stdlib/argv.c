/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-09-29
 * ================================
 */

#include "zc.h"

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
}

void zargv_fini(zargv_t *argvp)
{
    char **cpp;

    for (cpp = argvp->argv; cpp < argvp->argv + argvp->argc; cpp++) {
        zfree(*cpp);
    }
    zfree(argvp->argv);
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
    argvp->argv = (char **)zrealloc(argvp->argv, (new_size + 1) * sizeof(char *));
    argvp->size = new_size;
}

void zargv_add(zargv_t * argvp, const char *ns)
{
    if (argvp->argc >= argvp->size) {
        zargv_extend(argvp);
    }
    argvp->argv[argvp->argc++] = zstrdup(ns);
    argvp->argv[argvp->argc] = 0;
}

void zargv_addn(zargv_t * argvp, const char *ns, int nlen)
{
    if (argvp->argc >= argvp->size) {
        zargv_extend(argvp);
    }
    argvp->argv[argvp->argc++] = zstrndup(ns, nlen);
    argvp->argv[argvp->argc] = 0;
}

void zargv_truncate(zargv_t * argvp, int len)
{
    char **cpp;
    if (len < argvp->argc) {
        for (cpp = argvp->argv + len; cpp < argvp->argv + argvp->argc; cpp++) {
            zfree(*cpp);
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
        zdebug_show("%s", p);
    }
    ZARGV_WALK_END;
}

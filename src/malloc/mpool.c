/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2015-12-30
 * ================================
 */

#include "zc.h"

static void *_pool_malloc(zmpool_t * mp, int len)
{
    return zmalloc(len);
}

static void *_pool_calloc(zmpool_t * mp, int nmemb, int len)
{
    return zcalloc(nmemb, len);
}

static void *_pool_realloc(zmpool_t * mp, const void *ptr, int len)
{
    return zrealloc(ptr, len);
}

static void _pool_free(zmpool_t * mp, const void *ptr)
{
    return zfree(ptr);
}

static void _pool_reset(zmpool_t * mp)
{
    zfatal("mpool: system memory can not reset");
}

static void _pool_free_pool(zmpool_t * mp)
{
}

static zmpool_method_t _method = {
    _pool_malloc,
    _pool_calloc,
    _pool_realloc,
    _pool_free,
    _pool_reset,
    _pool_free_pool
};

static zmpool_t zvar_system_mpool_buf = {
    &_method
};

zmpool_t *zvar_system_mpool = &zvar_system_mpool_buf;

/* ################################################################## */
void zmpool_free_pool(zmpool_t * mp)
{
    mp->method->free_pool(mp);
}

void zmpool_reset(zmpool_t * mp)
{
    mp->method->reset(mp);
}

/* ################################################################## */

void *zmpool_strdup(zmpool_t * mp, const char *ptr)
{
    int len;
    char *r;

    len = strlen(ptr);
    r = (char *)zmpool_malloc(mp, len + 1);
    if (len > 0) {
        memcpy(r, ptr, len);
    }
    r[len] = 0;

    return r;
}

void *zmpool_strndup(zmpool_t * mp, const char *ptr, int n)
{
    int nlen;
    char *r;

    if (n < 0) {
        n = 0;
    }
    if ((!ptr) || (!n)) {
        r = (char *)zmpool_malloc(mp, n + 1);
        r[0] = 0;
        return r;
    }
    const char *p = ptr;
    for(nlen=0;*p && (nlen<n);p++,nlen++) {
    }
    r = (char *)zmpool_malloc(mp, nlen + 1);
    if (nlen > 0) {
        memcpy(r, ptr, nlen);
    }
    r[nlen] = 0;

    return r;
}

void *zmpool_memdup(zmpool_t * mp, const void *ptr, int n)
{
    char *r;

    if (n < 0) {
        n = 0;
    }
    if ((!ptr) || (!n)) {
        r = (char *)zmpool_malloc(mp, 1);
        r[0] = 0;
        return r;
    }

    r = (char *)zmpool_malloc(mp, n);
    memcpy(r, ptr, n);

    return r;
}

void *zmpool_memdupnull(zmpool_t * mp, const void *ptr, int n)
{
    char *r;

    if (n < 0) {
        n = 0;
    }
    if ((!ptr) || (!n)) {
        r = (char *)zmpool_malloc(mp, 1);
        r[0] = 0;
        return r;
    }

    r = (char *)zmpool_malloc(mp, n+1);
    memcpy(r, ptr, n);
    r[n] = 0;

    return r;
}

/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-30
 * ================================
 */

#include "libzc.h"

void zmpool_free_pool(zmpool_t *mp)
{
    if (mp)
    {
        mp->free_pool(mp);
    }
}

void zmpool_reset(zmpool_t * mp)
{
    if (mp)
    {
        mp->reset(mp);
    }
}

/* ################################################################## */
void *zmpool_malloc(zmpool_t * mp, int len)
{
    if (!mp)
    {
        return zmalloc(len);
    }
    return mp->malloc(mp, len);
}

void *zmpool_realloc(zmpool_t * mp, void *ptr, int len)
{
    if (!mp)
    {
        return zrealloc(ptr, len);
    }
    return mp->realloc(mp, ptr, len);
}

void zmpool_free(zmpool_t * mp, void *ptr)
{
    if (!mp)
    {
        return zfree(ptr);
    }
    return mp->free(mp, ptr);
}

/* ################################################################## */
void *zmpool_calloc(zmpool_t * mp, int nmemb, int size)
{
    void *r;

    if (!mp)
    {
        return zcalloc(nmemb, size);
    }

    r = zmpool_malloc(mp, nmemb * size);
    memset(r, 0, nmemb * size);

    return r;
}

void *zmpool_strdup(zmpool_t * mp, char *ptr)
{
    int len;
    char *r;

    if (!mp)
    {
        return zstrdup(ptr);
    }

    if (!ptr)
    {
        ptr = "";
    }
    len = strlen(ptr);
    r = zmpool_malloc(mp, len + 1);
    if (len > 0)
    {
        memcpy(r, ptr, len);
    }
    r[len] = 0;

    return r;
}

void *zmpool_strndup(zmpool_t * mp, char *ptr, int n)
{
    int len;
    char *r;

    if (!mp)
    {
        return zstrndup(ptr, n);
    }

    if (!ptr)
    {
        ptr = "";
    }
    len = strlen(ptr);
    if (n > len)
    {
        n = len;
    }
    r = zmpool_malloc(mp, n + 1);
    if (n > 0)
    {
        memcpy(r, ptr, n);
    }
    r[n] = 0;

    return r;
}

void *zmpool_memdup(zmpool_t * mp, void *ptr, int n)
{
    char *r;

    if (!mp)
    {
        return zmemdup(ptr, n);
    }

    if (!ptr)
    {
        ptr = "";
    }

    r = zmpool_malloc(mp, n + 1);
    if (n > 0)
    {
        memcpy(r, ptr, n);
    }
    r[n] = 0;

    return r;
}

/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-10-12
 * ================================
 */

#include "zcc/zcc_stdlib.h"
#include <stdlib.h>

zcc_namespace_begin;

void *malloc(int64_t len)
{
    void *r;

    if (len < 0)
    {
        len = 0;
    }
    if ((r = ::malloc(len)) == 0)
    {
        if (len == 0)
        {
            return 0;
        }
        zcc_fatal("malloc: insufficient memory for %zd bytes", len);
    }

    return r;
}

void *calloc(int64_t nmemb, int64_t size)
{
    void *r;

    if (nmemb < 0)
    {
        nmemb = 0;
    }
    if (size < 0)
    {
        size = 0;
    }
    if ((r = ::calloc(nmemb, size)) == 0)
    {
        if (nmemb * size == 0)
        {
            return 0;
        }
        zcc_fatal("zcalloc: insufficient memory for %zdx%zd bytes", nmemb, size);
    }

    return r;
}

void *realloc(const void *ptr, int64_t len)
{
    void *r;
    if (len < 0)
    {
        len = 0;
    }
    if ((r = ::realloc((void *)ptr, len)) == 0)
    {
        if (len == 0)
        {
            return 0;
        }
        zcc_fatal("zrealloc: insufficient memory for %zd bytes", len);
    }

    return r;
}

void free(const void *ptr)
{
    if (ptr && (ptr != var_blank_buffer))
    {
        ::free((void *)ptr);
    }
}

char *strdup(const char *ptr)
{
    char *r;

    r = ::strdup(ptr ? ptr : "");
    if (r == NULL)
    {
        zcc_fatal("strdup: insufficient memory");
    }

    return r;
}

char *strndup(const char *ptr, int64_t n)
{
    char *r;

    if (!ptr)
    {
        ptr = "";
    }
    if (n < 0)
    {
        n = 0;
    }

    int64_t nlen = 0;
    const char *p = ptr;
    for (; *p && (nlen < n); p++, nlen++)
    {
    }
    r = (char *)memdupnull(ptr, nlen);
    if (r == NULL)
    {
        zcc_fatal("zstrndup: insufficient memory for %zd bytes", n);
    }

    return r;
}

void *memdup(const void *ptr, int64_t n)
{
    char *r;

    if (!ptr)
    {
        r = (char *)malloc(1);
        r[0] = 0;
        return r;
    }
    if (n < 0)
    {
        n = 0;
    }
    if ((r = (char *)malloc(n)) == 0)
    {
        zcc_fatal("zmalloc: insufficient memory for %zd bytes", n);
    }
    if (n > 0)
    {
        memcpy(r, ptr, n);
    }

    return r;
}

void *memdupnull(const void *ptr, int64_t n)
{
    char *r;

    if (!ptr)
    {
        r = (char *)::malloc(1);
        r[0] = 0;
        return r;
    }
    if (n < 0)
    {
        n = 0;
    }
    if ((r = (char *)::malloc(n + 1)) == 0)
    {
        zcc_fatal("zmalloc: insufficient memory for %zd bytes", (n + 1));
    }
    if (n > 0)
    {
        ::memcpy(r, ptr, n);
    }
    r[n] = 0;

    return r;
}

zcc_namespace_end;

/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-10-12
 * ================================
 */

#include "zc.h"

static char zblank_buffer_buffer[2] = {0, 0};
char *zblank_buffer = zblank_buffer_buffer;

void *zmalloc(int len)
{
    void *r;

    if (len < 0) {
        len = 0;
    }
    if ((r = malloc(len)) == 0) {
        if (len == 0) {
            return 0;
        }
        zfatal("FATAL zmalloc: insufficient memory for %d bytes: %m", len);
    }

    return r;
}

void *zcalloc(int nmemb, int size)
{
    void *r;
    
    if (nmemb < 0) {
        nmemb = 0;
    }
    if (size < 0) {
        size = 0;
    }
    if ((r = calloc(nmemb, size)) == 0) {
        if (nmemb * size == 0) {
            return 0;
        }
        zfatal("FATAL zcalloc: insufficient memory for %dx%d bytes: %m", nmemb, size);
    }

    return r;
}

void *zrealloc(const void *ptr, int len)
{
    void *r;
    if (len < 0) {
        len = 0;
    }
    if ((r = realloc((void *)ptr, len)) == 0) {
        if (len == 0) {
            return 0;
        }
        zfatal("FATAL zrealloc: insufficient memory for %d bytes: %m", len);
    }

    return r;
}

void zfree(const void *ptr)
{
    if (ptr && (ptr != zblank_buffer)) {
        free((void *)ptr);
    }
}

char *zstrdup(const char *ptr)
{
    char *r;

    r = strdup(ptr?ptr:"");
    if (r == NULL) {
        zfatal("FATAL zstrdup: insufficient memory : %m");
    }

    return r;
}

char *zstrndup(const char *ptr, int n)
{
    char *r;

    if (!ptr) {
        ptr = "";
    }
    if (n < 0) {
        n = 0;
    }

    int nlen = 0;
    const char *p = ptr;
    for(;*p && (nlen<n);p++,nlen++) {
    }
    r = strndup(ptr, nlen);
    if (r == NULL) {
        zfatal("FATAL zstrndup: insufficient memory for %d bytes: %m", n);
    }

    return r;
}

char *zmemdup(const void *ptr, int n)
{
    char *r;

    if (!ptr) {
        r = (char *)malloc(1);
        r[0] = 0;
        return r;
    }
    if (n < 0) {
        n = 0;
    }
    if ((r = (char *)malloc(n)) == 0) {
        zfatal("FATAL zmalloc: insufficient memory for %d bytes: %m", n);
    }
    if (n>0) {
        memcpy(r, ptr, n);
    }

    return r;
}


char *zmemdupnull(const void *ptr, int n)
{
    char *r;

    if (!ptr) {
        r = (char *)malloc(1);
        r[0] = 0;
        return r;
    }
    if (n<0) {
        n = 0;
    }
    if ((r = (char *)malloc(n+1)) == 0) {
        zfatal("FATAL zmalloc: insufficient memory for %d bytes: %m", (n+1));
    }
    if (n>0) {
        memcpy(r, ptr, n);
    }
    r[n] = 0;

    return r;
}

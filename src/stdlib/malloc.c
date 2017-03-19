/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2015-10-12
 * ================================
 */

#include "zc.h"

char zblank_buffer[1] = {'\0'};

void *zmalloc(int len)
{
    void *r;

    if (len < 1) {
        return zblank_buffer;
    }
    if ((r = malloc(len)) == 0) {
        zfatal("zmalloc: insufficient memory for %ld bytes: %m", (long)len);
    }
    ((char *)r)[0] = 0;

    return r;
}

void *zcalloc(int nmemb, int size)
{
    void *r;

    if ((!nmemb) || (!size)) {
        return zblank_buffer;
    }
    if ((r = calloc(nmemb, size)) == 0) {
        zfatal("zcalloc: insufficient memory for %ldx%ld bytes: %m", (long)nmemb, size);
    }

    return r;
}

void *zrealloc(const void *ptr, int len)
{
    void *r;

    if (len < 1) {
        zfree(ptr);
        return zblank_buffer;
    }

    if ((r = realloc((void *)ptr, len)) == 0) {
        zfatal("zrealloc: insufficient memory for %ld bytes: %m", (long)len);
    }

    return r;
}

void zfree(const void *ptr)
{
    if ((ptr) &&(ptr!=zblank_buffer)) {
        free((void *)ptr);
    }
}

char *zstrdup(const char *ptr)
{
    char *r;

    if ((!ptr) || (!*ptr)) {
        return zblank_buffer;
    }
    r = strdup(ptr);
    if (r == NULL) {
        zfatal("zstrdup: insufficient memory : %m");
    }

    return r;
}

char *zstrndup(const char *ptr, int n)
{
    char *r;

    if ((!ptr) || (!*ptr) || (n<1)) {
        return zblank_buffer;
    }

    r = strndup(ptr, n);
    if (r == NULL) {
        zfatal("zstrndup: insufficient memory for %ld bytes: %m", (long)n);
    }

    return r;
}

char *zmemdup(const void *ptr, int n)
{
    char *r;

    if ((!ptr) || (n<1) ) {
        return zblank_buffer;
    }

    if ((r = (char *)malloc(n)) == 0) {
        zfatal("zmalloc: insufficient memory for %ld bytes: %m", (long)n);
    }

    memcpy(r, ptr, n);

    return r;
}


char *zmemdupnull(const void *ptr, int n)
{
    char *r;

    if ((!ptr) || (n<1) ) {
        return zblank_buffer;
    }

    if ((r = (char *)malloc(n+1)) == 0) {
        zfatal("zmalloc: insufficient memory for %ld bytes: %m", (long)(n+1));
    }
    memcpy(r, ptr, n);
    r[n] = 0;

    return r;
}

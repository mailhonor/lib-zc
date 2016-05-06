/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-09-28
 * ================================
 */

#include "libzc.h"

void *zmalloc(int len)
{
    void *r;

    if (len < 1) {
        len = 1;
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

    if ((r = calloc(nmemb, size)) == 0) {
        zfatal("zcalloc: insufficient memory for %ldx%ld bytes: %m", (long)nmemb, size);
    }

    return r;
}

void *zrealloc(void *ptr, int len)
{
    void *r;

    if (len < 1) {
        len = 1;
    }

    if ((r = realloc(ptr, len)) == 0) {
        zfatal("zrealloc: insufficient memory for %ld bytes: %m", (long)len);
    }

    return r;
}

void zfree(void *ptr)
{
    if (ptr) {
        free(ptr);
    }
}

char *zstrdup(char *ptr)
{
    char *r;

    if (!ptr) {
        ptr = "";
    }
    r = strdup(ptr);
    if (r == NULL) {
        zfatal("zstrdup: insufficient memory : %m");
    }

    return r;
}

char *zstrndup(char *ptr, int n)
{
    char *r;

    if (!ptr) {
        ptr = "";
    }

    if (!n) {
        return zmalloc(0);
    }

    r = strndup(ptr, n);
    if (r == NULL) {
        zfatal("zstrndup: insufficient memory for %ld bytes: %m", (long)n);
    }

    return r;
}

char *zmemdup(void *ptr, int n)
{
    char *r;

    if (!ptr) {
        ptr = "";
    }

    r = zmalloc(n + 1);
    if (r == NULL) {
        zfatal("zmemdup: insufficient memory for %ld bytes: %m", (long)n);
    }
    if (n > 0) {
        memcpy(r, ptr, n);
    }
    r[n] = 0;

    return r;
}

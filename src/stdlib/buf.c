/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-10-12
 * ================================
 */

#include "libzc.h"

zbuf_t *zbuf_create(int size)
{
    zbuf_t *bf;

    bf = (zbuf_t *) zcalloc(1, sizeof(zbuf_t));
    if (size < 13) {
        size = 13;
    }
    bf->size = size;
    bf->len = 0;
    bf->data = (char *)zmalloc(size + 1);

    return bf;
}

void zbuf_free(zbuf_t * bf)
{
    zfree(bf->data);
    zfree(bf);
}

int zbuf_need_space(zbuf_t * bf, int need)
{
    int left, incr;

    if (bf->static_mode) {
        return -1;
    }

    left = bf->size - bf->len;
    incr = need - left;
    if (incr < 0) {
        return left;
    }
    if (incr < bf->size) {
        incr = bf->size;
    }
    bf->size += incr;
    bf->data = (char *)zrealloc(bf->data, bf->size + 1);
    return (left + incr);
}

int zbuf_put_do(zbuf_t * bf, int ch)
{
    if (bf->len == bf->size) {
        zbuf_need_space(bf, 1);
    }
    return ZBUF_PUT(bf, ch);
}

void zbuf_reset(zbuf_t * bf)
{
    ZBUF_RESET(bf);
}

void zbuf_terminate(zbuf_t * bf)
{
    ZBUF_TERMINATE(bf);
}

void zbuf_truncate(zbuf_t * bf, int new_len)
{
    ZBUF_TRUNCATE(bf, new_len);
}

int zbuf_strncpy(zbuf_t * bf, const char *src, int len)
{
    ZBUF_RESET(bf);
    if (len < 1) {
        return ZBUF_LEN(bf);
    }
    while (len-- && *src) {
        ZBUF_PUT(bf, *src);
        src++;
    }
    ZBUF_TERMINATE(bf);

    return (bf->len);
}

int zbuf_strcpy(zbuf_t * bf, const char *src)
{
    ZBUF_RESET(bf);
    while (*src) {
        ZBUF_PUT(bf, *src);
        src++;
    }
    ZBUF_TERMINATE(bf);

    return (bf->len);
}

int zbuf_strncat(zbuf_t * bf, const char *src, int len)
{
    if (len < 1) {
        return ZBUF_LEN(bf);
    }
    while (len-- && *src) {
        ZBUF_PUT(bf, *src);
        src++;
    }
    ZBUF_TERMINATE(bf);

    return (bf->len);
}

int zbuf_strcat(zbuf_t * bf, const char *src)
{
    while (*src) {
        ZBUF_PUT(bf, *src);
        src++;
    }
    ZBUF_TERMINATE(bf);

    return (bf->len);
}

int zbuf_memcpy(zbuf_t * bf, const void *src_raw, int len)
{
    char *src = (char *)src_raw;

    ZBUF_RESET(bf);
    if (len < 1) {
        return ZBUF_LEN(bf);
    }
    while (len--) {
        ZBUF_PUT(bf, *src);
        src++;
    }
    ZBUF_TERMINATE(bf);

    return (bf->len);
}

int zbuf_memcat(zbuf_t * bf, const void *src_raw, int len)
{
    char *src = (char *)src_raw;

    if (len < 1) {
        ZBUF_TERMINATE(bf);
        return ZBUF_LEN(bf);
    }
    while (len--) {
        ZBUF_PUT(bf, *src);
        src++;
    }
    ZBUF_TERMINATE(bf);

    return (bf->len);
}

/* ################################################################## */
/* printf */

int zbuf_printf_1024(zbuf_t * bf, const char *format, ...)
{
    va_list ap;
    char buf[1024+1];
    int len;

    va_start(ap, format);
    len = zvsnprintf(buf, 1024, format, ap);
    va_end(ap);
    return zbuf_memcat(bf, buf, len);
}


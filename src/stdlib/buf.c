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

int zbuf_strncpy(zbuf_t * bf, char *src, int len)
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

int zbuf_strcpy(zbuf_t * bf, char *src)
{
    ZBUF_RESET(bf);
    while (*src) {
        ZBUF_PUT(bf, *src);
        src++;
    }
    ZBUF_TERMINATE(bf);

    return (bf->len);
}

int zbuf_strncat(zbuf_t * bf, char *src, int len)
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

int zbuf_strcat(zbuf_t * bf, char *src)
{
    while (*src) {
        ZBUF_PUT(bf, *src);
        src++;
    }
    ZBUF_TERMINATE(bf);

    return (bf->len);
}

int zbuf_memcpy(zbuf_t * bf, void *src_raw, int len)
{
    char *src = (char *)src_raw;
    ZBUF_RESET(bf);
    if (len < 1) {
        return ZBUF_LEN(bf);
    }
    if (len > 1024) {
        bf->len = len;
        zbuf_need_space(bf, len);
        memcpy(bf->data, src, len);
    } else {
        while (len--) {
            ZBUF_PUT(bf, *src);
            src++;
        }
    }
    ZBUF_TERMINATE(bf);

    return (bf->len);
}

int zbuf_memcat(zbuf_t * bf, void *src_raw, int len)
{
    char *src = (char *)src_raw;
    if (len < 1) {
        return ZBUF_LEN(bf);
    }
    if (len > 1024) {
        zbuf_need_space(bf, len);
        memcpy(bf->data + bf->len, src, len);
        bf->len += len;
    } else {
        while (len--) {
            ZBUF_PUT(bf, *src);
            src++;
        }
    }
    ZBUF_TERMINATE(bf);

    return (bf->len);
}

int zbuf_add_int(zbuf_t * bf, int num)
{
    char buf[32];

    sprintf(buf, "%d", num);
    return zbuf_strcat(bf, buf);
}

/* ################################################################## */
/* printf */

int zbuf_simple_sprintf(zbuf_t * bf, char *format, ...)
{
    va_list ap;
    char buf[128];

    va_start(ap, format);
    snprintf(buf, 127, format, ap);
    va_end(ap);
    return zbuf_strcat(bf, buf);
}

/* 下面的 sprintf 相关函数不推荐使用.
 * 请注意 data 的长度限制为1024000~1M
 */
int zbuf_vprintf(zbuf_t * bf, char *format, va_list ap)
{
    char buf[1024001], *src;
    int c;

    vsnprintf(buf, 1024000, format, ap);
    src = buf;

    while (1) {
        c = *src++;
        if (!c) {
            break;
        }
        ZBUF_PUT(bf, c);
    }
    ZBUF_TERMINATE(bf);

    return bf->len;
}

int zbuf_sprintf(zbuf_t * bf, char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    zbuf_vprintf(bf, format, ap);
    va_end(ap);
    ZBUF_TERMINATE(bf);

    return bf->len;
}

int zbuf_vsprintf(zbuf_t * bf, char *format, va_list ap)
{
    zbuf_vprintf(bf, format, ap);
    ZBUF_TERMINATE(bf);

    return bf->len;
}

/* ################################################################## */
/* sizedata */

void zbuf_sizedata_escape(zbuf_t * bf, void *data, int len)
{
    int ch, left = len;

    if (len < 0) {
        return;
    }
    do {
        ch = left & 0177;
        left >>= 7;
        if (!left) {
            ch |= 0200;
        }
        ZBUF_PUT(bf, ch);
    }
    while (left);
    if (len > 0) {
        zbuf_memcat(bf, data, len);
    }
}

void zbuf_sizedata_escape_int(zbuf_t * bf, int i)
{
    char buf[32];
    int len;

    len = sprintf(buf, "%d", i);

    zbuf_sizedata_escape(bf, buf, len);
}

void zbuf_sizedata_escape_long(zbuf_t * bf, long i)
{
    char buf[64];
    int len;

    len = sprintf(buf, "%lu", i);

    zbuf_sizedata_escape(bf, buf, len);
}

void zbuf_sizedata_escape_dict(zbuf_t * bf, zdict_t * zd)
{
    zdict_node_t *n;
    char *k, *v;

    ZDICT_WALK_BEGIN(zd, n) {
        k = (char *)zdict_key(n);
        v = (char *)zdict_value(n);
        zbuf_sizedata_escape(bf, k, strlen(k));
        zbuf_sizedata_escape(bf, v, strlen(v));
    }
    ZDICT_WALK_END;
}

void zbuf_sizedata_escape_pp(zbuf_t * bf, char **pp, int len)
{
    int i;
    char *p;

    for (i = 0; i < len; i++) {
        p = *pp++;
        zbuf_sizedata_escape(bf, p, p ? strlen(p) : 0);
    }
}

/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2015-10-12
 * ================================
 */

#include "zc.h"

zbuf_t *zbuf_create(int size)
{
    zbuf_t *bf;
    bf = (zbuf_t *) zmalloc(sizeof(zbuf_t));
    zbuf_init(bf, size);
    return bf;
}

void zbuf_init(zbuf_t *bf, int size)
{
    memset(bf, 0, sizeof(zbuf_t));
    if (size < 0) {
        size = 13;
    } else if (size == 0) {
        size = 1;
    }
    bf->size = size;
    bf->len = 0;
    bf->data = (char *)zmalloc(size + 1);
    bf->data[0] = 0;
}

void zbuf_fini(zbuf_t *bf)
{
    if (bf) {
        if (!bf->static_mode) {
            zfree(bf->data);
        }
    }
}

void zbuf_free(zbuf_t *bf)
{
    if (bf) {
        zbuf_fini(bf);
        zfree(bf);
    }
}

int zbuf_need_space(zbuf_t *bf, int need)
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

int zbuf_put_do(zbuf_t *bf, int ch)
{
    if (bf->len == bf->size) {
        zbuf_need_space(bf, 1);
    }
    return ZBUF_PUT(bf, ch);
}

void zbuf_strncpy(zbuf_t *bf, const char *src, int len)
{
    zbuf_reset(bf);
    if (len > 0) {
        while (len-- && *src) {
            ZBUF_PUT(bf, *src);
            src++;
        }
    }
    zbuf_terminate(bf);
}

void zbuf_strcpy(zbuf_t *bf, const char *src)
{
    zbuf_reset(bf);
#if 0
    while (*src) {
        ZBUF_PUT(bf, *src);
        src++;
    }
#else
    int len = strlen(src?src:"");
    int left = zbuf_need_space(bf, len + 1);
    if (left > len) {
        if (len > 0) {
            memcpy(zbuf_data(bf) + zbuf_len(bf), src, len);
            bf->len += len;
        }
    } else {
        while (len--) {
            ZBUF_PUT(bf, *src);
            src++;
        }
    }
#endif
    zbuf_terminate(bf);
}

void zbuf_strncat(zbuf_t *bf, const char *src, int len)
{
    if (len > 0) {
        while (len-- && *src) {
            ZBUF_PUT(bf, *src);
            src++;
        }
    }
    zbuf_terminate(bf);
}

void zbuf_strcat(zbuf_t *bf, const char *src)
{
#if 0
    while (*src) {
        ZBUF_PUT(bf, *src);
        src++;
    }
#else
    int len = strlen(src?src:"");
    if (len > 0) {
        int left = zbuf_need_space(bf, len + 1);
        if (left > len) {
            if (len > 0) {
                memcpy(zbuf_data(bf) + zbuf_len(bf), src, len);
                bf->len += len;
            }
        } else {
            while (len--) {
                ZBUF_PUT(bf, *src);
                src++;
            }
        }
    }
#endif
    zbuf_terminate(bf);
}

void zbuf_memcpy(zbuf_t *bf, const void *src_raw, int len)
{
    char *src = (char *)src_raw;

    zbuf_reset(bf);
    if (len > 0) {
#if 0
        while (len--) {
            ZBUF_PUT(bf, *src);
            src++;
        }
#else
        int left = zbuf_need_space(bf, len + 1);
        if (left > len) {
            if (len > 0) {
                memcpy(zbuf_data(bf) + zbuf_len(bf), src, len);
                bf->len += len;
            }
        } else {
            while (len--) {
                ZBUF_PUT(bf, *src);
                src++;
            }
        }
#endif
    }
    zbuf_terminate(bf);
}

void zbuf_memcat(zbuf_t *bf, const void *src_raw, int len)
{
    char *src = (char *)src_raw;

    if (len > 0) {
#if 0
        while (len--) {
            ZBUF_PUT(bf, *src);
            src++;
        }
#else
        int left = zbuf_need_space(bf, len + 1);
        if (left > len) {
            memcpy(zbuf_data(bf) + zbuf_len(bf), src, len);
            bf->len += len;
        } else {
            while (len--) {
                ZBUF_PUT(bf, *src);
                src++;
            }
        }
#endif
    }
    zbuf_terminate(bf);
}

/* ################################################################## */
/* printf */

void zbuf_printf_1024(zbuf_t *bf, const char *format, ...)
{
    va_list ap;
    char buf[1024+1];
    int len;

    va_start(ap, format);
    len = vsnprintf(buf, 1024, format, ap);
    len = ((len<1024)?len:(1024-1));
    va_end(ap);

    zbuf_memcat(bf, buf, len);
}

void zbuf_vprintf_1024(zbuf_t *bf, const char *format, va_list ap)
{
    char buf[1024+1];
    int len;
    len = vsnprintf(buf, 1024, format, ap);
    len = ((len<1024)?len:(1024-1));
    zbuf_memcat(bf, buf, len);
}

void zbuf_trim_right_rn(zbuf_t *bf)
{
    unsigned char *data = (unsigned char *)(bf->data);
    while (bf->len > 0) {
        if(data[bf->len -1] == '\n') {
            bf->len --;
            continue;
        }
        if(data[bf->len -1] == '\r') {
            bf->len --;
            continue;
        }
        break;
    }
    zbuf_terminate(bf);
}


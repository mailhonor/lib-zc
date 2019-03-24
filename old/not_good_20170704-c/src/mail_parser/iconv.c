/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-09
 * ================================
 */

#include "zc.h"

static inline void ___clear_null(zbuf_t *dest)
{
    char ch, *p = (char *)ZBUF_DATA(dest);
    size_t ri = 0, i, len = ZBUF_LEN(dest);
    for (i=0;i<len;i++) {
        ch = p[i];
        if (ch == '\0') {
            continue;
        }
        p[ri++] = ch;
    }
    zbuf_truncate(dest, ri);
    zbuf_terminate(dest);
}

static inline void ___clear_null2(const char *data, size_t size, zbuf_t *dest)
{
    char ch, *p = (char *)data;
    size_t i;

    zbuf_reset(dest);
    for (i=0;i<size;i++) {
        ch = p[i];
        if (ch == '\0') {
            continue;
        }
        ZBUF_PUT(dest, ch);
    }
    zbuf_terminate(dest);
}

ssize_t zmime_iconv(const char *src_charset, const char *data, size_t size, zbuf_t *dest)
{
    char f_charset_buf[64];
    const char *f_charset;
    size_t ret;
    int detact = 0;

    f_charset = src_charset;
    if (ZEMPTY(f_charset)) {
        detact = 1;
        if (zcharset_detect_cjk(data, size, f_charset_buf)) {
            f_charset = f_charset_buf;
        } else  {
            f_charset = "GB18030";
        }
    }
    f_charset = zcharset_correct_charset(f_charset);

    zbuf_reset(dest);
    ret = zcharset_iconv_zbuf(f_charset, data, size
            , "UTF-8", dest
            , 0
            , -1, 0);

    if (ret > 0 ) {
        ___clear_null(dest);
        return ZBUF_LEN(dest);
    }

    if(detact) {
        ___clear_null2(data, size, dest);
        return ZBUF_LEN(dest);
    } else {
        if (zcharset_detect_cjk(data, size, f_charset_buf)) {
            f_charset = f_charset_buf;
        } else  {
            f_charset = "GB18030";
        }
    }

    zbuf_reset(dest);
    ret = zcharset_iconv_zbuf(f_charset, data, size
            , "UTF-8", dest
            , 0
            , -1, 0);
    if (ret > 0 ) {
        ___clear_null(dest);
        return ZBUF_LEN(dest);
    }

    ___clear_null2(data, size, dest);
    return ZBUF_LEN(dest);
}

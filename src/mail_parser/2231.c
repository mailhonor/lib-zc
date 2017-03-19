
/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-15
 * ================================
 */

#include "zc.h"

void zmime_header_line_2231_get_utf8(const char *src_charset_def, const char *in_src, size_t in_len, int with_charset, zbuf_t *dest)
{
    int i, len, ret, start_len;
    char ch, *pend = (char *)in_src + in_len, *ps, *p, *start, *charset;
    char charset_buf[32];

    zbuf_reset(dest);

    if (with_charset) {
        p = memchr(in_src, '\'', in_len);
        if (!p) {
            goto err;
        }
        len = p - in_src + 1;
        if (len > 31) {
            len = 31;
        }
        memcpy(charset_buf, in_src, len);
        charset_buf[len] = 0;

        ps = p + 1;
        p = memchr(ps, '\'', pend - ps);
        if (!p) {
            goto err;
        }
        p++;

        {
            char *p;
            p = strchr(charset_buf, '*');
            if (p) {
                *p = 0;
            }
        }
        charset = charset_buf;
        start = p;
        start_len = pend - p;
    } else {
        charset = (char *)src_charset_def;
        start = (char *)in_src;
        start_len = in_len;
    }
    ZSTACK_BUF(zb, ZMAIL_HEADER_LINE_MAX_LENGTH);
    for(i=0;i<start_len;i++) {
        ch = start[i];
        if (ch!='%') {
            ZBUF_PUT(zb, ch);
            continue;
        }
        if (i+1 > start_len) {
            break;
        }
        ch = (zhex_to_dec_list[(int)(start[i+1])]<<4) | (zhex_to_dec_list[(int)(start[i+2])]);
        ZBUF_PUT(zb, ch);
        i += 2;
    }
    ZBUF_TERMINATE(zb);
    ret = zmime_iconv(charset, ZBUF_DATA(zb), ZBUF_LEN(zb), dest);
    if (ret < 1) {
        return;
    }
    return;
  err:
    zbuf_memcpy(dest, in_src, in_len);
}

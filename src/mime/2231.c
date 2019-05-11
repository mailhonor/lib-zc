/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2015-12-09
 * ================================
 */

/* rfc 2231 */

#include "zc.h"
#include "mime.h"

void zmime_header_line_get_utf8_2231(const char *src_charset_def, const char *in_line, int in_len, zbuf_t *result, int with_charset_flag)
{
    if (in_len == -1) {
        in_len = strlen(in_line);
    }
    if (in_len < 1) {
        return;
    }
    int i, len, start_len, ch;
    char *in_src, *pend, *ps, *p, *start, *charset;
    ZSTACK_BUF(charset_buf, zvar_charset_name_max_size+1);
   
    in_src = (char *)(void *)in_line;
    pend = in_src + in_len;
    zbuf_reset(result);
    if (with_charset_flag) {
        p = (char *)memchr(in_src, '\'', in_len);
        if (!p) {
            goto err;
        }
        len = p - in_src + 1;
        zbuf_memcpy(charset_buf, in_src, len);

        ps = p + 1;
        p = (char *)memchr(ps, '\'', pend - ps);
        if (!p) {
            goto err;
        }
        p++;

        {
            char *p;
            p = strchr(zbuf_data(charset_buf), '*');
            if (p) {
                *p = 0;
            }
        }
        charset = zbuf_data(charset_buf);
        start = p;
        start_len = pend - p;
    } else {
        charset = (char *)(void *)src_charset_def;
        start = (char *)(void *)in_line;
        start_len = in_len;
    }

    zbuf_t *tmps = zbuf_create(in_len);
    zbuf_reset(tmps);
    for(i=0;i<start_len;i++) {
        ch = start[i];
        if (ch!='%') {
            ZBUF_PUT(tmps, ch);
            continue;
        }
        if (i+1 > start_len) {
            break;
        }
        ch = (zchar_xdigitval_vector[(int)(start[i+1])]<<4) | (zchar_xdigitval_vector[(int)(start[i+2])]);
        ZBUF_PUT(tmps, ch);
        i += 2;
    }
    zmime_iconv(charset, zbuf_data(tmps), zbuf_len(tmps), result);
    zbuf_free(tmps);
    return;
err:
    zbuf_memcpy(result, in_src, in_len);
}

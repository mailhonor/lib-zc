/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2015-12-09
 * ================================
 */

#include "zc.h"
#include "mime.h"

void zmime_iconv(const char *from_charset, const char *data, int size, zbuf_t *result)
{
    if (size < 1) {
        return;
    }

    char f_charset_buf[zvar_charset_name_max_size+1];
    const char *f_charset;
    int detected = 0;

    zbuf_reset(result);
    f_charset = from_charset;
    if (ZEMPTY(f_charset)) {
        detected = 1;
        if (zcharset_detect_cjk(data, size, f_charset_buf)) {
            f_charset = f_charset_buf;
        } else  {
            f_charset = "GB18030";
        }
    } else {
        f_charset = zcharset_correct_charset(f_charset);
    }

    if (zcharset_convert(f_charset, data, size,
                "UTF-8", result, 0,
                -1, 0) > 0) {
        zmail_clear_null_inner(zbuf_data(result), zbuf_len(result));
        return;
    }

    if(detected) {
        zbuf_memcpy(result, data, size);
        zmail_clear_null_inner(zbuf_data(result), zbuf_len(result));
        return;
    }

    if (zcharset_detect_cjk(data, size, f_charset_buf)) {
        f_charset = f_charset_buf;
    } else  {
        f_charset = "GB18030";
    }
    zbuf_reset(result);
    if (zcharset_convert(f_charset, data, size,
                "UTF-8", result, 0,
                -1, 0) > 0) {
        zmail_clear_null_inner(zbuf_data(result), zbuf_len(result));
        return;
    }

    zbuf_memcpy(result, data, size);
    zmail_clear_null_inner(zbuf_data(result), zbuf_len(result));
    return;
}

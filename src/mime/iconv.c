/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-12-09
 * ================================
 */

#include "./mime.h"

extern char *zcharset_detect_1252(const char *data, int size, char *charset_result);

static int _mime_iconv_1252(const char *data, int size, zbuf_t *result)
{
    zbuf_reset(result);
    int omit_invalid_bytes_count = 0;

    const char *charset = "windows-1252";
    char f_charset_buf[zvar_charset_name_max_size + 1];
    charset = zcharset_detect_1252(data, size, f_charset_buf);
    if (!charset)
    {
        charset = "windows-1252";
    }

    if (zcharset_convert(charset, data, size,
                         "UTF-8", result, 0,
                         10, &omit_invalid_bytes_count) < 0)
    {
        return 0;
    }

    if (omit_invalid_bytes_count > 0)
    {
        return 0;
    }
    return 1;
}

void zmime_iconv(const char *from_charset, const char *data, int size, zbuf_t *result)
{
    if (size < 1)
    {
        return;
    }

    if (from_charset)
    {
        if ((!strcasecmp(from_charset, "iso-8859-1")) || (!strcasecmp(from_charset, "windows-1252")))
        {
            if (_mime_iconv_1252(data, size, result))
            {
                return;
            }
            from_charset = 0;
        }
    }

    const char *default_charset = "WINDOWS-1252";
    if (from_charset && (from_charset[0] == '?'))
    {
        default_charset = from_charset + 1;
        from_charset = 0;
    }

    char f_charset_buf[zvar_charset_name_max_size + 1];
    const char *f_charset;
    int detected = 0;

    zbuf_reset(result);
    f_charset = from_charset;
    if (ZEMPTY(f_charset))
    {
        detected = 1;
        if (zcharset_detect_cjk(data, size, f_charset_buf))
        {
            f_charset = f_charset_buf;
        }
        else
        {
            f_charset = default_charset;
        }
        // FILE *fp = fopen("dd", "w+");
        // fwrite(data, 1, size, fp);
        // fclose(fp);
    }
    else
    {
        f_charset = zcharset_correct_charset(f_charset);
    }

    if (zcharset_convert(f_charset, data, size,
                         "UTF-8", result, 0,
                         -1, 0) > 0)
    {
        zmail_clear_null_inner(zbuf_data(result), zbuf_len(result));
        return;
    }

    if (detected)
    {
        zbuf_memcpy(result, data, size);
        zmail_clear_null_inner(zbuf_data(result), zbuf_len(result));
        return;
    }

    if (zcharset_detect_cjk(data, size, f_charset_buf))
    {
        f_charset = f_charset_buf;
    }
    else
    {
        f_charset = default_charset;
    }
    zbuf_reset(result);
    if (zcharset_convert(f_charset, data, size,
                         "UTF-8", result, 0,
                         -1, 0) > 0)
    {
        zmail_clear_null_inner(zbuf_data(result), zbuf_len(result));
        return;
    }

    zbuf_memcpy(result, data, size);
    zmail_clear_null_inner(zbuf_data(result), zbuf_len(result));
    return;
}

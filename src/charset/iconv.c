/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-12-08
 * ================================
 */

#include "../../cpp_src/cpp_dev.h"
#ifndef ___ZC_ZCC_MODE___
#include "zc.h"
#include <iconv.h>
#include <errno.h>
int zvar_charset_debug = 0;
int zvar_charset_uconv_mode = 0;
#else
#endif

#define mydebug(fmt, args...) \
    if (zvar_charset_debug)   \
    {                         \
        zinfo(fmt, ##args);   \
    }

#define ZCHARSET_ICONV_ERROR_OPEN (-2016)

typedef struct charset_iconv_t charset_iconv_t;
struct charset_iconv_t
{
    char *to_charset;
    char *from_charset;
    unsigned char charset_regular : 1;
    int in_converted_len;
    int omit_invalid_bytes;
    int omit_invalid_bytes_count;
    iconv_t ic;
};

char *zcharset_correct_charset(const char *charset)
{
    char tmpcharset[32];
    int charsetlen = strlen(charset);
    if (charsetlen > 30)
    {
        return (char *)charset;
    }
    strcpy(tmpcharset, charset);
    zstr_tolower(tmpcharset);

    typedef struct
    {
        const char *from;
        int flen;
        int maxlen;
        const char *to;
    } correct_t;

    correct_t vector[] = {
        {"gb2312", 6, -1, "GB18030"},
        {"gbk", 3, -1, "GB18030"},
        {"ks_c_5601", 9, 9, "ISO-2022-KR"},
        {"ks_c_5861", 9, 9, "EUC-KR"},
        {"unicode-1-1-utf-7", 17, -1, "UTF-7"},
#ifdef _WIN32
        {"utf7", 4, -1, "UTF-7"},
        {"utf8", 4, -1, "UTF-8"},
#endif // _WIN32
        {0, 0, 0, 0}};
    for (correct_t *vi = vector; vi->from; vi++)
    {
        if (tmpcharset[0] != vi->from[0])
        {
            continue;
        }
        if (vi->maxlen == -1)
        {
            if (charsetlen != vi->flen)
            {
                continue;
            }
            if (strcmp(tmpcharset, vi->from))
            {
                continue;
            }
            return (char *)(vi->to);
        }
        else
        {
            if (charsetlen < vi->maxlen)
            {
                continue;
            }
            if (strncmp(tmpcharset, vi->from, vi->maxlen))
            {
                continue;
            }
            return (char *)(vi->to);
        }
    }
    return (char *)charset;
}

static inline int charset_iconv_base(charset_iconv_t *ic, char *_in_str, int _in_len, char *_out_s, int _out_l)
{
    char *in_str = _in_str;
    size_t in_len = (size_t)(_in_len);
    int in_converted_len = 0;

    char *out_str = _out_s;
    size_t out_len = (size_t)(_out_l);
    int out_converted_len = 0;

    int ic_ret;
    int errno2;
    char *in_str_o, *out_tmp;
    size_t out_len_tmp;
    int t_ilen, t_olen;

    ic->in_converted_len = 0;

    if (_in_len < 1)
    {
        return 0;
    }
    if (ic->omit_invalid_bytes_count > ic->omit_invalid_bytes)
    {
        return 0;
    }

    /* correct charset */
    if (!(ic->charset_regular))
    {
        if (zempty(ic->from_charset) || zempty(ic->to_charset))
        {
            return -1;
        }
        ic->charset_regular = 1;
        ic->to_charset = zcharset_correct_charset(ic->to_charset);
        ic->from_charset = zcharset_correct_charset(ic->from_charset);
    }

    /* */
    if (ic->ic == 0)
    {
        ic->ic = iconv_open(ic->to_charset, ic->from_charset);
    }

    if (ic->ic == (iconv_t)-1)
    {
        return ZCHARSET_ICONV_ERROR_OPEN;
    }

    /* do ic */
    while (in_len > 0)
    {
        in_str_o = in_str;
        out_tmp = out_str;
        out_len_tmp = out_len;

        ic_ret = iconv(ic->ic, &in_str, &in_len, &out_tmp, &out_len_tmp);
        t_ilen = in_str - in_str_o;
        in_converted_len += t_ilen;

        t_olen = out_tmp - out_str;
        out_str = out_tmp;
        out_len = out_len_tmp;
        out_converted_len += t_olen;

        if (ic_ret != (int)-1)
        {
            ic_ret = iconv(ic->ic, NULL, NULL, &out_tmp, &out_len_tmp);
            t_olen = out_tmp - out_str;
            out_str = out_tmp;
            out_len = out_len_tmp;
            out_converted_len += t_olen;
            break;
        }
        errno2 = zget_errno();
        if (errno2 == E2BIG)
        {
            break;
        }
        else if (errno2 == EILSEQ || errno2 == EINVAL)
        {
            in_str++;
            in_len--;
            ic->omit_invalid_bytes_count++;
            if (ic->omit_invalid_bytes_count > ic->omit_invalid_bytes)
            {
                break;
            }
            in_converted_len += 1;
            continue;
        }
        else
        {
            break;
        }
    }

    ic->in_converted_len = in_converted_len;

    return out_converted_len;
}

#ifndef ___ZC_ZCC_MODE___
int zcharset_iconv(const char *from_charset, const char *src, int src_len, const char *to_charset, zbuf_t *dest, int *src_converted_len, int omit_invalid_bytes_limit, int *omit_invalid_bytes_count)
#else
int charset_iconv(const char *from_charset, const char *src, int src_len, const char *to_charset, std::string &dest, int *src_converted_len, int omit_invalid_bytes_limit, int *omit_invalid_bytes_count)
#endif
{
    if (zvar_charset_uconv_mode)
    {
#ifndef ___ZC_ZCC_MODE___
        zfatal("run zcharset_convert_use_uconv first");
#else
        zfatal("run zcc::charset_convert_use_uconv instead of zcharset_convert_use_uconv first");
#endif
    }
    charset_iconv_t ic_buf, *ic = &ic_buf;
    char buf[4910];
    int len;
    char *in_str;
    int in_len;
    int out_converted_len = 0;
    char *str_running;
    int len_running;
    zbuf_reset_cpp(dest);

    memset(ic, 0, sizeof(charset_iconv_t));
    ic->from_charset = (char *)(from_charset);
    ic->to_charset = (char *)(to_charset);
    if (omit_invalid_bytes_limit < 0)
    {
        ic->omit_invalid_bytes = (256 * 256 * 256 * 127 - 1);
    }
    else
    {
        ic->omit_invalid_bytes = omit_invalid_bytes_limit;
    }

    in_str = (char *)(src);
    in_len = src_len;
    if (in_len < 0)
    {
        in_len = strlen(src);
    }

    while (in_len > 0)
    {
        str_running = buf;
        len_running = 4096;
        len = charset_iconv_base(ic, in_str, in_len, str_running, len_running);
        if (len < 0)
        {
            out_converted_len = -1;
            break;
        }
        in_str += ic->in_converted_len;
        in_len -= ic->in_converted_len;

        if (len == 0)
        {
            break;
        }
        out_converted_len += len;
        zbuf_memcat_cpp(dest, buf, len);
    }

    if ((ic->ic) && (ic->ic != (iconv_t)-1))
    {
        iconv_close(ic->ic);
    }

    if (src_converted_len)
    {
        *src_converted_len = in_str - (char *)(src);
    }
    if (omit_invalid_bytes_count)
    {
        *omit_invalid_bytes_count = ic->omit_invalid_bytes_count;
    }

    return out_converted_len;
}
/*
 * iconv static lib, missing libiconv and GCONV_PATH mismatched.
 * 1, download latest libiconv;
 * 2, ./configure --enable-static=PKGS
 * 3, make
 * 4, ls lib/.libs/iconv.o lib/.libs/localcharset.o lib/.libs/relocatable.o
 */

#ifndef ___ZC_ZCC_MODE___
int (*zcharset_convert)(const char *from_charset, const char *src, int src_len, const char *to_charset, zbuf_t *result, int *src_converted_len, int omit_invalid_bytes_limit, int *omit_invalid_bytes_count) = zcharset_iconv;
#else
int (*charset_convert)(const char *from_charset, const char *src, int src_len, const char *to_charset, std::string &result, int *src_converted_len, int omit_invalid_bytes_limit, int *omit_invalid_bytes_count) = charset_iconv;
#endif

#ifndef ___ZC_ZCC_MODE___
static void _clear_null_inner(zbuf_t *bf)
{
    char *p = zbuf_data(bf);
    int size = zbuf_len(bf), i;
    for (i = 0; i < size; i++)
    {
        if (p[i] == '\0')
        {
            p[i] = ' ';
        }
    }
}
#else  // ___ZC_ZCC_MODE___
static void _clear_null_inner(std::string &bf)
{
    const char *p = bf.c_str();
    size_t size = bf.size(), i;
    for (i = 0; i < size; i++)
    {
        if (p[i] == '\0')
        {
            bf[i] = ' ';
        }
    }
}
#endif // ___ZC_ZCC_MODE___

#ifdef __cplusplus
zcc_namespace_c_begin;
#endif // __cplusplus
extern char *zcharset_detect_1252(const char *data, int size, char *charset_result);
#ifdef __cplusplus
zcc_namespace_c_end;
#endif // __cplusplus

#ifndef ___ZC_ZCC_MODE___
static int _mime_iconv_1252(const char *data, int size, zbuf_t *result)
#else
static int _mime_iconv_1252(const char *data, int size, std::string &result)
#endif
{
    zbuf_reset_cpp(result);
    int omit_invalid_bytes_count = 0;

    const char *charset = "windows-1252";
    char f_charset_buf[zvar_charset_name_max_size + 1];
    charset = zcharset_detect_1252(data, size, f_charset_buf);
    if (!charset)
    {
        charset = "windows-1252";
    }

    if (zcharset_convert_cpp(charset, data, size,
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

#ifndef ___ZC_ZCC_MODE___
void zcharset_convert_to_utf8(const char *from_charset, const char *data, int size, zbuf_t *result)
#else
void charset_convert_to_utf8(const char *from_charset, const char *data, int size, std::string &result)
#endif
{
    if (size < 0)
    {
        size = strlen(data);
    }
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

    zbuf_reset_cpp(result);
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
    }
    else
    {
        f_charset = zcharset_correct_charset(f_charset);
    }

    if (zcharset_convert_cpp(f_charset, data, size,
                             "UTF-8", result, 0,
                             -1, 0) > 0)
    {
        _clear_null_inner(result);
        return;
    }

    if (detected)
    {
        zbuf_memcpy_cpp(result, data, size);
        _clear_null_inner(result);
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
    zbuf_reset_cpp(result);
    if (zcharset_convert_cpp(f_charset, data, size,
                             "UTF-8", result, 0,
                             -1, 0) > 0)
    {
        _clear_null_inner(result);
        return;
    }

    zbuf_memcpy_cpp(result, data, size);
    _clear_null_inner(result);
    return;
}

/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2015-12-08
 * ================================
 */

#include "zc.h"
#include <iconv.h>
#include <errno.h>

zbool_t zvar_charset_debug = 0;

#define ZCHARSET_ICONV_ERROR_OPEN       (-2016)

typedef struct charset_iconv_t charset_iconv_t;
struct charset_iconv_t {
    char *to_charset;
    char *from_charset;
    unsigned char charset_regular:1;
    int in_converted_len;
    int omit_invalid_bytes;
    int omit_invalid_bytes_count;
    iconv_t ic;
};

char *zcharset_correct_charset(const char *charset)
{

    if (ZSTR_CASE_EQ(charset, "gb2312")) {
        charset = "GB18030";
#if 0
    } else if (ZSTR_CASE_EQ(charset, "CHINESEBIG5_CHARSET")) {
        charset = "BIG5";
    } else if (ZSTR_CASE_EQ(charset, "GB2312_CHARSET")) {
        charset = "GB18030";
#endif
    } else if (ZSTR_CASE_EQ(charset, "GBK")) {
        charset = "GB18030";
    } else if (ZSTR_N_CASE_EQ(charset, "KS_C_5601", 9)) {
        charset = "ISO-2022-KR";
    } else if (ZSTR_N_CASE_EQ(charset, "KS_C_5861", 9)) {
        charset = "EUC-KR";
    } else if (ZSTR_CASE_EQ(charset, "unicode-1-1-utf-7")) {
        charset = "UTF-7";
    }

    return (char *)charset;
}

static inline int charset_iconv_base(charset_iconv_t * ic, char *_in_str, int _in_len, char *_out_s, int _out_l)
{
    char *in_str = _in_str;
    size_t in_len = (size_t) (_in_len);
    int in_converted_len = 0;

    char *out_str = _out_s;
    size_t out_len = (size_t) (_out_l);
    int out_converted_len = 0;

    int ic_ret;
    int errno2;
    char *in_str_o, *out_tmp;
    size_t out_len_tmp;
    int t_ilen, t_olen;

    ic->in_converted_len = 0;

    if (_in_len < 1) {
        return 0;
    }
    if (ic->omit_invalid_bytes_count > ic->omit_invalid_bytes) {
        return 0;
    }

    /* correct charset */
    if (!(ic->charset_regular)) {
        if (zempty(ic->from_charset) || zempty(ic->to_charset)) {
            return -1;
        }
        ic->charset_regular = 1;
        ic->to_charset = zcharset_correct_charset(ic->to_charset);
        ic->from_charset = zcharset_correct_charset(ic->from_charset);
    }

    /* */
    if (ic->ic == 0) {
        ic->ic = iconv_open(ic->to_charset, ic->from_charset);
    }
    if (ic->ic == (iconv_t) - 1) {
        return ZCHARSET_ICONV_ERROR_OPEN;
    }

    /* do ic */
    while (in_len > 0) {
        in_str_o = in_str;
        out_tmp = out_str;
        out_len_tmp = out_len;

        ic_ret = iconv(ic->ic, &in_str, &in_len, &out_tmp, &out_len_tmp);
        errno2 = errno;
        t_ilen = in_str - in_str_o;
        in_converted_len += t_ilen;

        t_olen = out_tmp - out_str;
        out_str = out_tmp;
        out_len = out_len_tmp;
        out_converted_len += t_olen;

        if (ic_ret != (int) - 1) {
            ic_ret = iconv(ic->ic, NULL, NULL, &out_tmp, &out_len_tmp);
            t_olen = out_tmp - out_str;
            out_str = out_tmp;
            out_len = out_len_tmp;
            out_converted_len += t_olen;

            break;
        }
        if (errno2 == E2BIG) {
            break;
        } else if (errno2 == EILSEQ || errno2 == EINVAL) {
            in_str++;
            in_len--;
            ic->omit_invalid_bytes_count++;
            if (ic->omit_invalid_bytes_count > ic->omit_invalid_bytes) {
                break;
            }
            in_converted_len += 1;
            continue;
        } else {
            break;
        }
    }

    ic->in_converted_len = in_converted_len;

    return out_converted_len;
}

int zcharset_iconv(const char *from_charset, const char *src, int src_len, const char *to_charset, zbuf_t *dest, int *src_converted_len, int omit_invalid_bytes_limit, int *omit_invalid_bytes_count)
{
    charset_iconv_t ic_buf, *ic = &ic_buf;
    char buf[4910];
    int len;
    char *in_str;
    int in_len;
    int out_converted_len = 0;
    char *str_running;
    int len_running;
    zbuf_reset(dest);

    memset(ic, 0, sizeof(charset_iconv_t));
    ic->from_charset = (char *)(from_charset);
    ic->to_charset = (char *)(to_charset);
    if (omit_invalid_bytes_limit < 0) {
        ic->omit_invalid_bytes = (256 * 256 * 256 * 127 - 1);
    } else {
        ic->omit_invalid_bytes = omit_invalid_bytes_limit;
    }

    in_str = (char *)(src);
    in_len = src_len;
    if (in_len < 0) {
        in_len = strlen(src);
    }

    while (in_len > 0) {
        str_running = buf;
        len_running = 4096;
        len = charset_iconv_base(ic, in_str, in_len, str_running, len_running);
        if (len < 0) {
            out_converted_len = -1;
            break;
        }
        in_str += ic->in_converted_len;
        in_len -= ic->in_converted_len;

        if (len == 0) {
            break;
        }
        out_converted_len += len;
        zbuf_memcat(dest, buf, len);
    }

    if ((ic->ic) && (ic->ic != (iconv_t) - 1)) {
        iconv_close(ic->ic);
    }

    if (src_converted_len) {
        *src_converted_len = in_str - (char *)(src);
    }
    if (omit_invalid_bytes_count) {
        *omit_invalid_bytes_count = ic->omit_invalid_bytes_count;
    }

    return out_converted_len;
}
int (*zcharset_convert)(const char *from_charset, const char *src, int src_len, const char *to_charset, zbuf_t *result, int *src_converted_len, int omit_invalid_bytes_limit, int *omit_invalid_bytes_count) = zcharset_iconv;

/*
 * iconv static lib, missing libiconv and GCONV_PATH mismatched.
 * 1, download latest libiconv; 
 * 2, ./configure --enable-static=PKGS 
 * 3, make 
 * 4, ls lib/.libs/iconv.o lib/.libs/localcharset.o lib/.libs/relocatable.o 
 */

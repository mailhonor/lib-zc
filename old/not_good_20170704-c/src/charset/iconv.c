/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-08
 * ================================
 */

#include "zc.h"
#include <iconv.h>

#define ZCHARSET_ICONV_ERROR_OPEN       (-2016)

typedef struct zcharset_iconv_t zcharset_iconv_t;
struct zcharset_iconv_t {
    char *to_charset;
    char *from_charset;
    unsigned char charset_regular:1;
    int in_converted_len;
    int omit_invalid_bytes;
    int omit_invalid_bytes_count;
    iconv_t ic;
};

int zvar_charset_iconv = 0;
char *zcharset_correct_charset(const char *charset)
{

    if (ZSTR_CASE_EQ(charset, "gb2312")) {
        charset = "GB18030";
    } else if (ZSTR_CASE_EQ(charset, "CHINESEBIG5_CHARSET")) {
        charset = "BIG5";
    } else if (ZSTR_CASE_EQ(charset, "GB2312_CHARSET")) {
        charset = "GB18030";
    } else if (ZSTR_CASE_EQ(charset, "GBK")) {
        charset = "GB18030";
    } else if (ZSTR_N_CASE_EQ(charset, "ks_c_5601", 9)) {
        charset = "ISO-2022-KR";
    } else if (ZSTR_N_CASE_EQ(charset, "KS_C_5861", 9)) {
        charset = "EUC-KR";
    } else if (ZSTR_CASE_EQ(charset, "unicode-1-1-utf-7")) {
        charset = "UTF-7";
    }

    return (char *)charset;
}

static inline int zcharset_iconv_base(zcharset_iconv_t * ic, char *_in_str, int _in_len, char *_out_s, int _out_l)
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

int zcharset_iconv(
        const char *from_charset, const char *src, int src_len
        , const char *to_charset,  char *dest, int dest_len
        , int *src_converted_len
        , int omit_invalid_bytes_limit, int *omit_invalid_bytes_count
        )
{
    zcharset_iconv_t ic_buf, *ic = &ic_buf;
    char buf[4910];
    int len;
    char *in_str;
    int in_len;
    int out_converted_len = 0;
    char *str_running;
    int len_running;

    memset(ic, 0, sizeof(zcharset_iconv_t));
    ic->from_charset = (char *)(from_charset);
    ic->to_charset = (char *)(to_charset);
    if (omit_invalid_bytes_limit < 0) {
        ic->omit_invalid_bytes = (256 * 256 * 256 * 127 - 1);
    } else {
        ic->omit_invalid_bytes = omit_invalid_bytes_limit;
    }

    in_str = (char *)(src);
    in_len = src_len;

    while (in_len > 0) {
        if (dest_len > 0) {
            str_running = dest;
            len_running = dest_len;
        } else  {
            str_running = buf;
            len_running = 4096;
        }
        len = zcharset_iconv_base(ic, in_str, in_len, str_running, len_running);
        if (len < 0) {
            out_converted_len = -1;
            break;
        }
        in_str += ic->in_converted_len;
        in_len -= ic->in_converted_len;

        if (len == 0) {
            break;
        }
        if (dest_len > 0) {
            dest[len] = 0;
            out_converted_len = len;
            break;
        }
        out_converted_len += len;

        if (dest_len == Z_DF_ZBUF) {
            zbuf_memcat((zbuf_t *)dest, buf, len);
        } else {
            zfatal("zcharset_iconv: Z_DF type unknown");
        }
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

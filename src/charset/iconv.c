/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-08
 * ================================
 */

#include "libzc.h"

#define ZICONV_DEFAULT_CHARSET  "GB18030"

char *zcharset_correct_charset(char *charset, char *default_charset)
{
    if (!default_charset)
    {
        default_charset = ZICONV_DEFAULT_CHARSET;
    }

    if (ZSTR_CASE_EQ(charset, "gb2312"))
    {
        charset = "GB18030";
    }
    else if (ZSTR_CASE_EQ(charset, "CHINESEBIG5_CHARSET"))
    {
        charset = "BIG5";
    }
    else if (ZSTR_CASE_EQ(charset, "GB2312_CHARSET"))
    {
        charset = default_charset;
    }
    else if (ZSTR_N_CASE_EQ(charset, "ks_c_5601", 9))
    {
        charset = "ISO-2022-KR";
    }
    else if (ZSTR_N_CASE_EQ(charset, "KS_C_5861", 9))
    {
        charset = "EUC-KR";
    }
    else if (ZSTR_CASE_EQ(charset, "unicode-1-1-utf-7"))
    {
        charset = default_charset;
    }

    return charset;
}

static inline int zcharset_iconv_base(zcharset_iconv_t * ic)
{
    char *in_str = ic->in_str;
    size_t in_len = (size_t) (ic->in_len);
    int in_converted_len = 0;

    char *out_str = ic->out_str_runing;
    size_t out_len = (size_t) (ic->out_len_runing);
    int out_converted_len = 0;

    int ic_ret;
    int errno2;
    char *in_str_o, *out_tmp;
    size_t out_len_tmp;
    int t_ilen, t_olen;

    ic->in_converted_len = 0;

    if (ic->in_len < 1)
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
        if (!(ic->from_charset) || !(ic->to_charset))
        {
            return -1;
        }
        ic->charset_regular = 1;
        ic->to_charset = zcharset_correct_charset(ic->to_charset, ic->default_charset);
        ic->from_charset = zcharset_correct_charset(ic->from_charset, ic->default_charset);
    }

    /* */
    if (ic->ic == 0)
    {
        ic->ic = iconv_open(ic->to_charset, ic->from_charset);
    }
    if (ic->ic == (iconv_t) - 1)
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
        errno2 = errno;
        t_ilen = in_str - in_str_o;
        in_converted_len += t_ilen;

        t_olen = out_tmp - out_str;
        out_str = out_tmp;
        out_len = out_len_tmp;
        out_converted_len += t_olen;

        if (ic_ret != (size_t) - 1)
        {
            ic_ret = iconv(ic->ic, NULL, NULL, &out_tmp, &out_len_tmp);
            t_olen = out_tmp - out_str;
            out_str = out_tmp;
            out_len = out_len_tmp;
            out_converted_len += t_olen;

            break;
        }
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

void zcharset_iconv_init(zcharset_iconv_t * ic)
{
    memset(ic, 0, sizeof(zcharset_iconv_t));
    ic->omit_invalid_bytes = (256*256*256*127 - 1);
}

void zcharset_iconv_fini(zcharset_iconv_t * ic)
{
    if ((ic->ic) && (ic->ic != (iconv_t) - 1))
    {
        iconv_close(ic->ic);
    }
    ic->ic = 0;
}

int zcharset_iconv(zcharset_iconv_t * ic)
{
    char buf[4910];
    int len;
    char *in_str;
    int in_len;
    int out_converted_len = 0;
    
    in_str = ic->in_str;
    in_len = ic->in_len;
    while (in_len > 0)
    {
        if(ic->filter_type > 0)
        {
            ic->out_str_runing = (char *)(ic->filter);
            ic->out_len_runing = ic->filter_type;
        }
        else
        {

            ic->in_str = in_str;
            ic->in_len = in_len;
            ic->out_str_runing = buf;
            ic->out_len_runing = 4096;
        }
        len = zcharset_iconv_base(ic);
        if (len < 0)
        {
            return len;
        }
        if(ic->filter_type > 0)
        {
            return len;
        }
        if (len == 0)
        {
            return out_converted_len;
        }
        out_converted_len += len;

        in_str += ic->in_converted_len;
        in_len -= ic->in_converted_len;
        zdata_filter_write(ic->filter, ic->filter_type, buf, len);
    }

    return out_converted_len;
}

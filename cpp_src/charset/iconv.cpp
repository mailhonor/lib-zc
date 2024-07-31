/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-10-12
 * ================================
 */

#include "zcc/zcc_charset.h"
#include "zcc/zcc_errno.h"
#include <iconv.h>

zcc_namespace_begin;
zcc_general_namespace_begin(charset);

#define CHARSET_ICONV_ERROR_OPEN (-2016)

struct charset_iconv_t
{
    const char *to_charset{nullptr};
    const char *from_charset{nullptr};
    bool charset_regular{false};
    int in_converted_len{-1};
    int omit_invalid_bytes{-1};
    int omit_invalid_bytes_count{0};
    iconv_t ic{nullptr};
};

static inline int charset_iconv_base(charset_iconv_t &ic, char *_in_str, int _in_len, char *_out_s, int _out_l)
{
    char *in_str = _in_str;
    uint64_t in_len = (uint64_t)(_in_len);
    int in_converted_len = 0;

    char *out_str = _out_s;
    uint64_t out_len = (uint64_t)(_out_l);
    int out_converted_len = 0;

    int ic_ret;
    int errno2;
    char *in_str_o, *out_tmp;
    uint64_t out_len_tmp;
    int t_ilen, t_olen;

    ic.in_converted_len = 0;

    if (_in_len < 1)
    {
        return 0;
    }
    if (ic.omit_invalid_bytes_count > ic.omit_invalid_bytes)
    {
        return 0;
    }

    /* correct charset */
    if (!(ic.charset_regular))
    {
        if (zcc::empty(ic.from_charset) || zcc::empty(ic.to_charset))
        {
            return -1;
        }
        ic.charset_regular = true;
        ic.to_charset = correct_name(ic.to_charset);
        ic.from_charset = correct_name(ic.from_charset);
    }

    /* */
    if (ic.ic == 0)
    {
        ic.ic = iconv_open(ic.to_charset, ic.from_charset);
    }

    if (ic.ic == (iconv_t)-1)
    {
        return CHARSET_ICONV_ERROR_OPEN;
    }

    /* do ic */
    while (in_len > 0)
    {
        in_str_o = in_str;
        out_tmp = out_str;
        out_len_tmp = out_len;

        ic_ret = iconv(ic.ic, &in_str, &in_len, &out_tmp, &out_len_tmp);
        t_ilen = in_str - in_str_o;
        in_converted_len += t_ilen;

        t_olen = out_tmp - out_str;
        out_str = out_tmp;
        out_len = out_len_tmp;
        out_converted_len += t_olen;

        if (ic_ret != (int)-1)
        {
            ic_ret = iconv(ic.ic, NULL, NULL, &out_tmp, &out_len_tmp);
            t_olen = out_tmp - out_str;
            out_str = out_tmp;
            out_len = out_len_tmp;
            out_converted_len += t_olen;
            break;
        }
        errno2 = get_errno();
        if (errno2 == ZCC_E2BIG)
        {
            break;
        }
        else if (errno2 == ZCC_EILSEQ || errno2 == ZCC_EINVAL)
        {
            // if (in_str != _in_str)
            // {
            //     break;
            // }
            in_str++;
            in_len--;
            ic.omit_invalid_bytes_count++;
            if (ic.omit_invalid_bytes_count > ic.omit_invalid_bytes)
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

    ic.in_converted_len = in_converted_len;

    return out_converted_len;
}

int iconv_convert(const char *from_charset, const char *src, int src_len, const char *to_charset, std::string &dest, int *src_converted_len, int omit_invalid_bytes_limit, int *omit_invalid_bytes_count)
{
    charset_iconv_t ic;
    char buf[4910];
    int len;
    char *in_str;
    int in_len;
    int out_converted_len = 0;
    char *str_running;
    int len_running;
    dest.clear();

    ic.from_charset = (char *)(from_charset);
    ic.to_charset = (char *)(to_charset);
    if (omit_invalid_bytes_limit < 0)
    {
        ic.omit_invalid_bytes = (256 * 256 * 256 * 127 - 1);
    }
    else
    {
        ic.omit_invalid_bytes = omit_invalid_bytes_limit;
    }

    in_str = (char *)(src);
    in_len = src_len;
    if (in_len < 0)
    {
        in_len = std::strlen(src);
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
        in_str += ic.in_converted_len;
        in_len -= ic.in_converted_len;

        if (len == 0)
        {
            break;
        }
        out_converted_len += len;
        dest.append(buf, len);
    }

    if ((ic.ic) && (ic.ic != (iconv_t)ZCC_VOID_PTR_ONE))
    {
        iconv_close(ic.ic);
    }

    if (src_converted_len)
    {
        *src_converted_len = in_str - (char *)(src);
    }
    if (omit_invalid_bytes_count)
    {
        *omit_invalid_bytes_count = ic.omit_invalid_bytes_count;
    }

    return out_converted_len;
}
int (*convert)(const char *from_charset, const char *src, int src_len, const char *to_charset, std::string &dest, int *src_converted_len, int omit_invalid_bytes_limit, int *omit_invalid_bytes_count) = iconv_convert;

zcc_general_namespace_end(charset);
zcc_namespace_end;

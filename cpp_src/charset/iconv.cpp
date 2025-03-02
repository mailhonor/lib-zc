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

std::string iconv_convert(const char *from_charset, const char *str, int str_len, const char *to_charset, int *invalid_bytes)
{
    int err_bytes = 0;
    if (invalid_bytes)
    {
        *invalid_bytes = 0;
    }
    std::string r;
    if (zcc::empty(from_charset) || zcc::empty(to_charset))
    {
        return r;
    }
    std::string f_charset = correct_name(from_charset);
    std::string t_charset = correct_name(to_charset);
    toupper(f_charset);
    toupper(t_charset);
    if (!invalid_bytes)
    {
        bool append = false;
#ifdef _WIN64
        append = true;
#endif // _WIN64
        const char *f = f_charset.c_str();
        if (!append)
        {
            if (!std::strncmp(f, "ISO-2022-", 9))
            {
                append = true;
            }
            else if (!std::strcmp(f, "UTF-7"))
            {
                append = true;
            }
        }
        if (append)
        {
            t_charset.append("//IGNORE");
        }
    }
    from_charset = f_charset.c_str();
    to_charset = t_charset.c_str();

    iconv_t ic = iconv_open(to_charset, from_charset);
    if (ic == (iconv_t)-1)
    {
        return "";
    }
    if (str_len < 0)
    {
        str_len = strlen(str);
    }
    if (str_len == 0)
    {
        return r;
    }

    char *in_str = (char *)(void *)str;
    size_t in_len = (size_t)str_len;
    size_t out_len = in_len * 4 + 16;
    char *out_str = (char *)malloc((int64_t)(out_len + 1));

    while (in_len > 0)
    {
        iconv(ic, NULL, NULL, NULL, NULL);
        size_t out_len_tmp = out_len;
        char *out_str_tmp = out_str;
        out_len_tmp = out_len;
        out_str_tmp = out_str;

        size_t ret = iconv(ic, &in_str, &in_len, &out_str_tmp, &out_len_tmp);
        size_t len = out_str_tmp - out_str;
        if (len > 0)
        {
            r.append(out_str, len);
        }

        if (ret != (size_t)-1)
        {
            char *out_str_tmp_last = out_str_tmp;
            iconv(ic, NULL, NULL, &out_str_tmp, &out_len_tmp);
            len = out_str_tmp - out_str_tmp_last;
            if (len > 0)
            {
                r.append(out_str_tmp_last, len);
            }
            break;
        }
        if (in_len < 1)
        {
            break;
        }
        in_str++;
        in_len--;
        err_bytes++;
    }

    free(out_str);
    iconv_close(ic);
    if (invalid_bytes)
    {
        *invalid_bytes = err_bytes;
    }
    return r;
}

zcc_general_namespace_end(charset);
zcc_namespace_end;

/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-12-09
 * ================================
 */

/* rfc 2231 */

#include "./mime.h"

zcc_namespace_begin;

std::string mail_parser::decode_2231(const char *src_charset_def, const char *in_line, int64_t in_len, bool with_charset_flag)
{
    std::string result;
    if (in_len < 0)
    {
        in_len = std::strlen(in_line);
    }
    if (in_len < 1)
    {
        return result;
    }

    buffer charset_buf;
    int64_t i, len, start_len, ch;
    char *in_src, *pend, *ps, *p, *start, *charset;

    in_src = (char *)(void *)in_line;
    pend = in_src + in_len;

    if (with_charset_flag)
    {
        p = (char *)std::memchr(in_src, '\'', in_len);
        if (!p)
        {
            goto err;
        }
        len = p - in_src + 1;
        charset_buf.append(in_src, len);

        ps = p + 1;
        p = (char *)std::memchr(ps, '\'', pend - ps);
        if (!p)
        {
            goto err;
        }
        p++;

        {
            char *p;
            p = std::strchr(charset_buf.c_str(), '*');
            if (p)
            {
                *p = 0;
            }
        }
        charset = charset_buf.c_str();
        start = p;
        start_len = pend - p;

        // decode
        buffer tmps;
        for (i = 0; i < start_len; i++)
        {
            ch = start[i];
            if (ch != '%')
            {
                tmps.push_back(ch);
                continue;
            }
            if (i + 1 > start_len)
            {
                break;
            }
            ch = (var_char_xdigitval_vector[(int64_t)(start[i + 1])] << 4) | (var_char_xdigitval_vector[(int64_t)(start[i + 2])]);
            tmps.push_back(ch);
            i += 2;
        }
        result = mail_parser::charset_convert(charset, tmps.c_str(), tmps.size());
        goto over;
    }
    else
    {
        result = mail_parser::header_line_get_utf8(src_charset_def, in_line, in_len);
        goto over;
    }
err:
    result = mail_parser::charset_convert(src_charset_def, in_src, in_len);
over:
    return result;
}

zcc_namespace_end;

/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-12-06
 * ================================
 */

#include "zcc/zcc_stdlib.h"

zcc_namespace_begin;

void http_token_decode(const void *src, int src_size, std::string &result)
{
    int l, r;
    char *p = (char *)src;
    if (src_size < 0)
    {
        src_size = std::strlen((const char *)src);
    }
    for (int i = 0; i < src_size; i++)
    {
        if (p[i] == '%')
        {
            if (i + 3 > src_size)
            {
                break;
            }
            l = var_char_xdigitval_vector[(int)(p[i + 1])];
            r = var_char_xdigitval_vector[(int)(p[i + 2])];
            if ((l != -1) && (r != -1))
            {
                result.push_back((l << 4) + (r));
            }
            i += 2;
        }
        else
        {
            result.push_back(p[i]);
        }
    }
}

std::string http_token_decode(const void *src, int src_size)
{
    std::string result;
    http_token_decode(src, src_size, result);
    return result;
}

void http_token_encode(const void *src, int src_size, std::string &result, bool strict_flag)
{
    unsigned char dec2hex[18] = "0123456789ABCDEF";
    const char *ps = (const char *)src;
    if (src_size < 0)
    {
        src_size = std::strlen((const char *)src);
    }
    for (int i = 0; i < src_size; i++)
    {
        unsigned char ch = ps[i];
        if (ch == ' ')
        {
            result.append("%20");
            continue;
        }
        if (isalnum(ch))
        {
            result.push_back(ch);
            continue;
        }
        if (strict_flag)
        {
            result.push_back('%');
            result.push_back(dec2hex[ch >> 4]);
            result.push_back(dec2hex[ch & 0X0F]);
            continue;
        }
        if (ch > 127)
        {
            result.push_back(ch);
            continue;
        }
        if (std::strchr("._-", ch))
        {
            result.push_back(ch);
            continue;
        }
        result.push_back('%');
        result.push_back(dec2hex[ch >> 4]);
        result.push_back(dec2hex[ch & 0X0F]);
    }
}

std::string http_token_encode(const void *src, int src_size, bool strict_flag)
{
    std::string result;
    http_token_encode(src, src_size, result, strict_flag);
    return result;
}

zcc_namespace_end;

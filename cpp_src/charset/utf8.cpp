/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-10-12
 * ================================
 */

#include "zcc/zcc_charset.h"

zcc_namespace_begin;
zcc_general_namespace_begin(charset);

int utf8_tail_complete(const char *ps, int len)
{
    if (len < 0)
    {
        len = std::strlen(ps);
    }
    int bak_len = len;

    // 找 ascii 或 (ch & 0XC0)
    while (1)
    {
        if (len < 1)
        {
            break;
        }
        unsigned char ch = ((const unsigned char *)ps)[len - 1];
        if (ch < 128)
        {
            return len;
        }
        if ((ch & 0XC0) == 0XC0)
        {
            break;
        }
        len--;
    }
    if (len < 1)
    {
        return len;
    }

    //
    unsigned char ch = ((const unsigned char *)ps)[len - 1];
    int ch_len = utf8_len(ch);
    if (bak_len - len >= ch_len - 1)
    {
        return len + ch_len - 1;
    }
    return len - 1;
}

char *utf8_tail_complete_and_terminate(char *ps, int len)
{
    len = utf8_tail_complete(ps, len);
    ps[len] = 0;
    return ps;
}

std::string &utf8_tail_complete_and_terminate(std::string &s)
{
    int len = utf8_tail_complete(s.c_str(), s.size());
    s.resize(len);
    return s;
}

std::string utf8_get_simple_digest(const char *s, int len, int need_width)
{
    std::string r;

    const unsigned char *ps = (const unsigned char *)s;
    int width = 0;
    bool last_blank = true;
    if (len < 0)
    {
        len = strlen(s);
    }
    for (int i = 0; i < len; i++)
    {
        if (width > need_width - 1)
        {
            break;
        }
        int ch = ps[i];
        int ch_len = zcc::charset::utf8_len(ch);
        if (ch_len == 1)
        {
            if ((ch == ' ') || (ch == '\t') || (ch == '\r') || (ch == '\n') || (ch < 32))
            {
                if (last_blank)
                {
                    continue;
                }
                r.push_back(' ');
                width += 1;
                last_blank = true;
                continue;
            }
            else
            {
                r.push_back(ch);
                width += 1;
                last_blank = false;
                continue;
            }
            continue;
        }
        if (len - i < ch_len)
        {
            break;
        }
        if (ch_len == 2)
        {
            if ((ps[i] == 0XC2) && (ps[i + 1] == 0XA0))
            {
                if (last_blank)
                {
                }
                else
                {
                    r.push_back(' ');
                    width += 1;
                    last_blank = true;
                }
            }
            else
            {
                r.append(s + i, ch_len);
                width += 2;
                last_blank = false;
            }
        }
        else
        {
            r.append(s + i, ch_len);
            width += 2;
            last_blank = false;
        }
        i += ch_len - 1;
    }
    return r;
}

zcc_general_namespace_end(charset);
zcc_namespace_end;

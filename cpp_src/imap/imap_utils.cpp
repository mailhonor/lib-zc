/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2022-11-15
 * ================================
 */

#include "zc.h"

zcc_namespace_begin;

// utils
bool imap_client::parse_imap_result(const char *key_line)
{
    if (zempty(key_line))
    {
        logic_error_ = true;
        need_close_connection_ = true;
        return false;
    }
    else if (key_line[0] == 'B')
    {
        ok_no_bad_ = result_onb::bad;
        return true;
    }
    else if (key_line[0] == 'N')
    {
        ok_no_bad_ = result_onb::no;
        return true;
    }
    else if (key_line[0] == 'O')
    {
        ok_no_bad_ = result_onb::ok;
        return true;
    }
    else
    {
        logic_error_ = true;
        need_close_connection_ = true;
        return false;
    }
}

bool imap_client::parse_imap_result(char tag, const char *line)
{
    if ((line[0] != tag) && (line[0] != '*'))
    {
        logic_error_ = true;
        need_close_connection_ = true;
        return false;
    }
    if (line[1] != ' ')
    {
        logic_error_ = true;
        need_close_connection_ = true;
        return false;
    }
    if (line[2] == 'B')
    {
        ok_no_bad_ = result_onb::bad;
        return true;
    }
    else if (line[2] == 'N')
    {
        ok_no_bad_ = result_onb::no;
        return true;
    }
    else if (line[2] == 'O')
    {
        ok_no_bad_ = result_onb::ok;
        return true;
    }
    else
    {
        logic_error_ = true;
        need_close_connection_ = true;
        return false;
    }
}

bool imap_client::parse_imap_result(char tag, const response_tokens &response_tokens)
{
    auto &token_vector = response_tokens.token_vector_;
    if (token_vector.size() < 2)
    {
        logic_error_ = true;
        need_close_connection_ = true;
        return false;
    }
    const std::string &tmp0 = token_vector[0];
    if ((tmp0.size() != 1))
    {
        logic_error_ = true;
        need_close_connection_ = true;
        return false;
    }
    if ((tmp0[0] != '*') && (tmp0[0] != tag))
    {
        logic_error_ = true;
        need_close_connection_ = true;
        return false;
    }
    const std::string &tmp1 = token_vector[1];
    if (tmp1.empty())
    {
        logic_error_ = true;
        need_close_connection_ = true;
        return false;
    }
    else if (tmp1[0] == 'O')
    {
        ok_no_bad_ = result_onb::ok;
        return true;
    }
    else if (tmp1[0] == 'N')
    {
        ok_no_bad_ = result_onb::no;
        return true;
    }
    else if (tmp1[0] == 'B')
    {
        ok_no_bad_ = result_onb::bad;
        return true;
    }
    logic_error_ = true;
    need_close_connection_ = true;
    return false;
}

std::string imap_client::escape_string(const char *s, int slen)
{
    std::string r;
    bool flag_quote = false, flag_size = false;
    int ch, i;
    if (slen < 0)
    {
        slen = strlen(s);
    }
    r.push_back('"');
    for (i = 0; i < slen; i++)
    {
        ch = s[i];
        if (ch < 32)
        {
            flag_size = true;
            break;
        }
        switch (ch)
        {
        case ' ':
        case '{':
            flag_quote = true;
            break;
        case '\\':
        case '"':
            flag_quote = true;
            r.push_back('\\');
            break;
        }
        r.push_back(ch);
    }
    r.push_back('"');
    if (flag_size)
    {
        r.clear();
        if (slen < 0)
        {
            slen = strlen(s);
        }
        zcc::sprintf_1024(r, "{%d}\r\n", slen);
        r.append(s, slen);
    }
    else if (flag_quote || (r.size() == 2))
    {
    }
    else
    {
        r.clear();
        r.append(s, slen);
    }
    return r;
}

std::string &imap_client::trim_rn(std::string &s)
{
    size_t size;
    size = s.size();
    if (size && (s[size - 1] == '\n'))
    {
        s.resize(size - 1);
        size--;
        if (size && (s[size - 1] == '\r'))
        {
            s.resize(size - 1);
        }
    }
    return s;
}

std::string imap_client::imap_utf7_to_utf8(const char *str, int slen)
{
    std::string r, tmpr, tmps;
    if (slen < 0)
    {
        slen = strlen(str);
    }
    const unsigned char *ps = (const unsigned char *)str, *end = ps + slen;
    const unsigned char *p;

    while (ps < end)
    {
        if (ps[0] != '&')
        {
            r.push_back(ps[0]);
            ps++;
            continue;
        }
        if (ps + 1 == end)
        {
            break;
        }
        ps += 1;
        if (ps[0] == '-')
        {
            r.push_back('&');
            ps += 1;
            continue;
        }
        tmps.clear();
        tmps.push_back('+');
        p = (const unsigned char *)memchr(ps, '-', end - ps);
        if (p)
        {
#if 0
            tmps.append((const char *)ps, p - ps + 1);
            ps = p + 1;
#else
            p ++;
            for (; ps < p; ps ++)
            {
                if (*ps == ',')
                {
                    tmps.push_back('/');
                }
                else
                {
                    tmps.push_back(*ps);
                }
            }
            ps = p;
#endif
        }
        else
        {
            tmps.append((const char *)ps, end - ps);
            tmps.push_back('-');
            ps = end;
        }
        tmpr.clear();
        zcc::charset_convert("UTF-7", tmps.c_str(), tmps.size(), "UTF-8", tmpr, 0, -1, 0);
        r.append(tmpr);
    }
    return r;
}

std::string imap_client::utf8_to_imap_utf7(const char *str, int slen)
{
    std::string r, tmpr, tmps;
    if (slen < 0)
    {
        slen = strlen(str);
    }
    const unsigned char *ps = (const unsigned char *)str, *end = ps + slen;
    const unsigned char *p;

    while (ps < end)
    {
        if (ps[0] == '&')
        {
            r.push_back('&');
            r.push_back('-');
            ps += 1;
            continue;
        }
        if (*(unsigned char *)ps < 128) {
            if (*ps < ' ') {
                goto err;
            }
            r.push_back(*ps);
            ps++;
            continue;
        }
        tmps.clear();
        while (ps < end) {
            if (*(unsigned char *)ps < 128) {
                break;
            }
            tmps.push_back(*ps);
            ps++;
            continue;
        }
        tmpr.clear();
        zcc::charset_convert("UTF-8", tmps.c_str(), tmps.size(), "UTF-7", tmpr, 0, -1, 0);
        if (tmpr.size() > 0) {
            r.push_back('&');
            r.append(tmpr.c_str() + 1);
        }
    }
    return r;
err:
    r.clear();
    return r;
}

zcc_namespace_end;

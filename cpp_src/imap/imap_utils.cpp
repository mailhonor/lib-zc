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

zcc_namespace_end;

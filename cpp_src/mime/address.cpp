/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-12-08
 * ================================
 */

#include "./mime.h"
#include "zcc/zcc_json.h"

zcc_namespace_begin;

static std::vector<mail_parser::mail_address> _line_get_address_vector(const char *src_charset_def, const char *in_str, int64_t in_len, bool loop_mode);

static int64_t _line_address_tok(char **str, int64_t *len, char **rname, char **raddress, char *tmp_cache, int64_t tmp_cache_size)
{
    char *pstr = *str;
    int64_t c;
    int64_t plen = *len, i, inquote = 0, find_lt = 0;
    char *name = 0, *mail = 0, last = 0;
    int64_t tmp_cache_idx = 0;
#define ___put(ch)                          \
    {                                       \
        if (tmp_cache_idx > tmp_cache_size) \
            return -1;                      \
        tmp_cache[tmp_cache_idx++] = (ch);  \
    }

    if (plen <= 0)
    {
        return -1;
    }
    for (i = 0; i < plen; i++)
    {
        c = *(pstr++);
        if (last == '\\')
        {
            ___put(c);
            last = '\0';
            continue;
        }
        if (c == '\\')
        {
            last = c;
            continue;
        }
        if (c == '"')
        {
            if (inquote)
            {
                inquote = 0;
                ___put(c);
            }
            else
            {
                inquote = 1;
                find_lt = 0;
            }
            continue;
        }
        if (inquote)
        {
            ___put(c);
            continue;
        }
        if (c == ',')
        {
            break;
        }
        if (c == ';')
        {
            break;
        }
        if (c == '<')
        {
            find_lt = 1;
        }
        ___put(c);
        if (c == '>')
        {
            if (find_lt == 1)
            {
                break;
            }
        }
    }
    *len = *len - (pstr - *str);
    *str = pstr;

    tmp_cache[tmp_cache_idx] = 0;
    pstr = tmp_cache;
    plen = tmp_cache_idx;
    if (plen < 1)
    {
        return -2;
    }
    while (1)
    {
        pstr = zcc::trim(pstr);
        plen = std::strlen(pstr);
        if (plen < 1)
        {
            return -2;
        }
        if (pstr[plen - 1] == '>')
        {
            pstr[plen - 1] = ' ';
            continue;
        }
        break;
    }
    unsigned char ch;
    int64_t findi = -1;
    for (i = plen - 1; i >= 0; i--)
    {
        ch = pstr[i];
        if ((ch == '<') || (ch == ' ') || (ch == '"') || (ch & 0X80))
        {
            if ((ch & 0X80) == 0)
            {
                pstr[i] = 0;
            }
            findi = i;
            break;
        }
    }
    if (findi > -1)
    {
        name = pstr;
        mail = zcc::trim(pstr + findi + 1);
    }
    else
    {
        name = 0;
        mail = pstr;
    }

    *raddress = mail;

    char *name_bak = name;
    pstr = name;
    while (name && *name)
    {
        if (*name != '"')
        {
            *pstr++ = *name++;
        }
        else
        {
            *pstr++ = ' ';
            name++;
        }
    }
    if (pstr)
    {
        *pstr = 0;
    }
    if (name_bak)
    {
        int64_t slen = zcc::skip(name_bak, std::strlen(name_bak), " \t\"'\r\n", 0, rname);
        if (slen > 0)
        {
            (*rname)[slen] = 0;
        }
        else
        {
            *rname = zcc::trim(name_bak);
        }
    }
    else
    {
        *rname = var_blank_buffer;
    }

#undef ___put
    return 0;
}

static mail_parser::mail_address _create_mime_address_simple(const char *n, const char *a)
{
    mail_parser::mail_address ma;
    ma.name_ = n;
    ma.mail_ = a;
    tolower(ma.mail_);
    return ma;
}

static mail_parser::mail_address _create_mime_address(const char *src_charset_def, const char *n, const char *a, bool loop_mode)
{
    if (loop_mode || (n[0]) || (a[0] != '=') || (a[1] != '?'))
    {
        return _create_mime_address_simple(n, a);
    }

    // =?utf-8?B?ImFiYyIgPHh4eEBhYWEuY29tPg==?=
    // "abc" <xxx@aaa.com>
    std::string tmpbf = mail_parser::header_line_get_utf8(src_charset_def, a);
    auto vec = _line_get_address_vector(src_charset_def, tmpbf.c_str(), tmpbf.size(), true);
    if (vec.empty())
    {
        return _create_mime_address_simple(n, a);
    }

    bool first = true;
    mail_parser::mail_address addr;
    for (auto it = vec.begin(); it != vec.end(); it++)
    {
        if (first)
        {
            first = false;
            addr = *it;
            addr.name_utf8_.clear();
        }
        else
        {
            addr.name_.append(" ").append(it->name_);
        }
    }

    return addr;
}

static std::vector<mail_parser::mail_address> _line_get_address_vector(const char *src_charset_def, const char *in_str, int64_t in_len, bool loop_mode)
{
    std::vector<mail_parser::mail_address> vec;

    if (in_len == -1)
    {
        in_len = std::strlen(in_str);
    }
    if (in_len < 1)
    {
        return vec;
    }
    char *n, *a, *str, *cache;
    int64_t len = (int64_t)in_len, ret;
    str = (char *)in_str;
    cache = (char *)zcc::malloc(in_len + 1024);

    while (1)
    {
        ret = _line_address_tok(&str, &len, &n, &a, cache, in_len + 1000);
        if (ret == -1)
        {
            break;
        }
        if (ret == -2)
        {
            continue;
        }
        vec.push_back(_create_mime_address(src_charset_def, n, a, loop_mode));
    }
    zcc::free(cache);
    return vec;
}

std::vector<mail_parser::mail_address> mail_parser::header_line_get_address_vector(const char *src_charset_def, const char *in_str, int64_t in_len)
{
    return zcc::_line_get_address_vector(src_charset_def, in_str, in_len, false);
}

std::vector<mail_parser::mail_address> mail_parser::header_line_get_address_vector_utf8(const char *src_charset_def, const char *in_str, int64_t in_len)
{
    std::vector<mail_parser::mail_address> vec = zcc::_line_get_address_vector(src_charset_def, in_str, in_len, false);
    for (auto it = vec.begin(); it != vec.end(); it++)
    {
        mail_address &ma = *it;
        if ((!ma.name_.empty()) && (ma.name_utf8_.empty()))
        {
            ma.name_utf8_ = header_line_get_utf8(src_charset_def, ma.name_);
        }
    }
    return vec;
}

zcc_namespace_end;

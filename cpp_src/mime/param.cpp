/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-12-09
 * ================================
 */

#include "./mime.h"

zcc_namespace_begin;

static inline char *ignore_chs(char *p, int64_t plen, const char *ignore, int64_t ignore_len)
{
    char *chs = (char *)(void *)ignore;
    int64_t i, j;
    for (i = 0; i < plen; i++)
    {
        for (j = 0; j < ignore_len; j++)
        {
            if (p[i] == chs[j])
            {
                break;
            }
        }
        if (j == ignore_len)
        {
            return (p + i);
        }
    }
    return p + plen;
}

static inline char *find_delim(char *p, int64_t plen, const char *delim, int64_t delim_len)
{
    char *chs = (char *)(void *)delim;
    int64_t i, j;
    for (i = 0; i < plen; i++)
    {
        for (j = 0; j < delim_len; j++)
        {
            if (p[i] == chs[j])
            {
                return (p + i);
            }
        }
    }
    return 0;
}

static int64_t find_value(char *buf, int64_t len, std::string &value, char **nbuf, bool is_filename)
{
    value.clear();
    char *ps = buf, *pend = ps + len, *p;
    int64_t ch;

    ps = ignore_chs(ps, pend - ps, " \t", 2);
    if (ps == pend)
    {
        return -1;
    }
    if (*ps == '"')
    {
        ps++;
        while (ps < pend)
        {
            ch = *ps++;
            if (ch == '"')
            {
                break;
            }
            else if (ch == '\\')
            {
                if (ps == pend)
                {
                    break;
                }
                int64_t ch2 = *ps++;
                if ((ch2 != '*') && (ch2 != '\\'))
                {
                    // 兼容 Foxmail 较早的版本
                    value.push_back(ch);
                }
                value.push_back(ch2);
            }
            else
            {
                value.push_back(ch);
            }
        }
    }
    else
    {
        p = find_delim(ps, pend - ps, " \t;", 3);
        if (!p)
        {
            value.append(ps, pend - ps);
            ps = pend;
        }
        else
        {
            value.append(ps, p - ps);
            ps = p + 1;
        }
    }

    *nbuf = ps;
    return 0;
}

static int64_t find_next_kv(char *buf, int64_t len, std::string &key, std::string &value, char **nbuf)
{
    key.clear();
    value.clear();
    int64_t is_filename = 0;

    char *ps = buf, *pend = ps + len, *p;

    ps = ignore_chs(ps, pend - ps, " \t;", 3);
    if (ps == pend)
    {
        return -1;
    }

    p = find_delim(ps, pend - ps, "= \t", 3);
    if (!p)
    {
        key.append(ps, pend - ps);
        tolower(key);
        *nbuf = pend;
        return 0;
    }
    key.append(ps, p - ps);
    tolower(key);
    if (*p == '=')
    {
        ps = p + 1; // begin value
    }
    else
    {
        ps = ignore_chs(p, pend - p, " \t", 2);
        if (ps == pend)
        {
            *nbuf = pend;
            return 0;
        }
        if (*ps != '=')
        {
            *nbuf = ps;
            return 0;
        }
        ps = ps + 1;
    }

    if (key == "filename")
    {
        is_filename = true;
    }
    if (find_value(ps, pend - ps, value, &ps, is_filename))
    {
        *nbuf = pend;
        return 0;
    }

    *nbuf = ps;
    return 0;
}

void mail_parser::header_line_get_params(const char *in_line, int64_t in_len, std::string &val, std::vector<std::tuple<std::string, std::string>> &params)
{
    char *ps = (char *)(void *)in_line, *pend = ps + in_len;
    if (find_value(ps, pend - ps, val, &ps, 0))
    {
        return;
    }

    std::string param_key, param_value;

    while (ps < pend)
    {
        if (find_next_kv(ps, pend - ps, param_key, param_value, &ps))
        {
            break;
        }
        params.push_back(std::make_tuple(param_key, param_value));
    }
}

void mail_parser::header_line_get_params(const char *in_line, int64_t in_len, std::string &val, dict &params)
{
    char *ps = (char *)(void *)in_line, *pend = ps + in_len;
    if (find_value(ps, pend - ps, val, &ps, 0))
    {
        return;
    }

    std::string param_key, param_value;

    while (ps < pend)
    {
        if (find_next_kv(ps, pend - ps, param_key, param_value, &ps))
        {
            break;
        }
        params[param_key] = param_value;
    }
}

zcc_namespace_end;

/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2017-07-22
 * ================================
 */

#include "zcc/zcc_http.h"
#include <cctype>

zcc_namespace_begin;

std::map<std::string, std::string> http_cookie_parse(const char *raw_cookie)
{
    std::map<std::string, std::string> r;
    const char *q, *p, *ps = raw_cookie;
    std::string name, value;

    while (1)
    {
        while (*ps)
        {
            if ((*ps == ' ') || (*ps == '\t'))
            {
                ps++;
                continue;
            }
            break;
        }
        p = ps;
        while ((*p != '\0') && (*p != ';'))
        {
            p++;
        }
        do
        {
            q = ps;
            while (q < p)
            {
                if (*q == '=')
                {
                    break;
                }
                q++;
            }
            if (q == p)
            {
                break;
            }
            name.clear();
            name.append(ps, q - ps);
            value.clear();
            q++;
            http_token_decode(q, p - q, value);
        } while (0);
        r[name] = value;

        if (*p == '\0')
        {
            break;
        }
        ps = p + 1;
    }
    return r;
}

std::string http_cookie_build_item(const char *name, const char *value, int64_t expires, const char *path, const char *domain, bool secure, bool httponly)
{
    unsigned char dec2hex[18] = "0123456789ABCDEF";
    std::string r;
    int ch;
    const unsigned char *p;

    r.append(name).append("=");
    if (empty(value))
    {
        r.append("deleted; expires=Thu, 01-Jan-1970 00:00:01 GMT; Max-Age=0;");
        return r;
    }

    // value
    r.append(http_token_encode(value, true));

    if (expires > 0)
    {
        r.append("; expires=").append(rfc7231_time(expires));
        r.append("; Max-Age=").append(std::to_string(expires - second()));
    }

    if (!empty(path))
    {
        r.append("; path=");
        // path
        p = (const unsigned char *)path;
        while ((ch = *p++))
        {
            if (std::isalnum(ch) || (ch == '/'))
            {
                r.push_back(ch);
            }
            else
            {
                r.push_back('%');
                r.push_back(dec2hex[ch >> 4]);
                r.push_back(dec2hex[ch & 0X0F]);
            }
        }
    }

    if (!empty(domain))
    {
        r.append("; domain=");
        // domain
        r.append(http_token_encode(domain, true));
    }

    if (secure)
    {
        r.append("; secure");
    }

    if (httponly)
    {
        r.append("; httponly");
    }

    return r;
}

zcc_namespace_end;

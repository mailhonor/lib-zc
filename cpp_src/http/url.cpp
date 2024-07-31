/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2017-07-22
 * ================================
 */

#include "zcc/zcc_http.h"
#include "zcc/zcc_buffer.h"

zcc_namespace_begin;

dict http_url::parse_query(const char *query, bool lowercase_mode)
{
    dict r;
    const unsigned char *q, *p, *ps = (const unsigned char *)query;
    std::string name, value;
    while (1)
    {
        p = ps;
        while ((*p != '\0') && (*p != '&'))
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
            name.append((const char *)ps, q - ps);
            if (lowercase_mode)
            {
                tolower(name);
            }
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

std::string http_url::build_query(const dict &qs, bool strict)
{
    std::string r;
    bool first = true;
    for (auto it = qs.begin(); it != qs.end(); it++)
    {
        if (first)
        {
            first = false;
        }
        else
        {
            r.append("&");
        }

        http_token_encode(it->first, r, strict);
        r.append("=");
        http_token_encode(it->second, r, strict);
    }
    return r;
}

http_url::http_url()
{
    reset();
}
http_url::http_url(const char *url, int len)
{
    parse_url(url, len);
}

http_url::http_url(const std::string &url)
{
    parse_url(url);
}

http_url::~http_url()
{
}

void http_url::parse_url(const char *url, int len)
{
    buffer bf;
    if (len < 0)
    {
        len = std::strlen(url);
    }
    bf.append(url, len);
    url = bf.c_str();
    reset();
    char *ps = (char *)(void *)url, *p;
    p = std::strstr(ps, "://");
    if (!p)
    {
        if ((ps[0] == '/') && (ps[1] == '/'))
        {
            protocol_ = "http";
            ps += 2;
        }
        else
        {
        }
    }
    else
    {
        protocol_.append(ps, p - ps);
        tolower(protocol_);
        ps = p + 3;
    }
    char *tmp_destination = ps;

    if ((!std::strncmp(ps, "local:", 6)))
    {
        host_ = "local";
        p = ps + 6;
        while (1)
        {
            if ((*p == '?') || (*p == '#') || (*p == '\0'))
            {
                path_.clear();
                path_.append(ps + 6, p - (ps + 6));
                destination_.clear();
                destination_.append(ps, p - ps);
                break;
            }
            p++;
        }
        ps = p + 1;
        if (*p == '?')
        {
            goto query;
        }
        if (*p == '#')
        {
            goto fragment;
        }
        return;
    }

    p = ps;
    while (1)
    {
        if ((*p == '?') || (*p == '#') || (*p == '/') || (*p == ':') || (*p == '\0'))
        {
            if (*p != ':')
            {
                destination_.clear();
                destination_.append(ps, p - ps);
            }
            host_.clear();
            host_.append(ps, p - ps);
            break;
        }
        p++;
    }
    ps = p + 1;
    if (*p == '?')
    {
        goto query;
    }
    if (*p == '#')
    {
        goto fragment;
    }
    if (*p == '/')
    {
        ps -= 1;
        goto path;
    }
    if (*p == ':')
    {
        goto port;
    }
    return;

port:
    p = ps;
    while (1)
    {
        if ((*p == '?') || (*p == '#') || (*p == '/') || (*p == '\0'))
        {
            destination_.clear();
            destination_.append(tmp_destination, p - tmp_destination);
            port_ = std::atoi(ps);
            break;
        }
        p++;
    }
    ps = p + 1;
    if (*p == '?')
    {
        goto query;
    }
    if (*p == '#')
    {
        goto fragment;
    }
    if (*p == '/')
    {
        goto path;
    }
    return;

path:
    p = ps;
    while (1)
    {
        if ((*p == '?') || (*p == '#') || (*p == '\0'))
        {
            path_.clear();
            http_token_decode(ps, p - ps, path_);
            break;
        }
        p++;
    }
    ps = p + 1;
    if (*p == '?')
    {
        goto query;
    }
    if (*p == '#')
    {
        goto fragment;
    }
    return;

query:
    p = ps;
    while (1)
    {
        if ((*p == '#') || (*p == '\0'))
        {
            query_.clear();
            query_.append(ps, p - ps);
            querys_ = parse_query(query_);
            break;
        }
        p++;
    }
    ps = p + 1;
    if (*p == '#')
    {
        goto fragment;
    }
    return;

fragment:
    fragment_ = ps;
}

std::string http_url::build_url()
{
    std::string r;
    r.append(protocol_).append("://");
    if (!destination_.empty())
    {
        r.append(destination_);
    }
    else if (!host_.empty())
    {
        r.append(host_);
        std::string pro = protocol_;
        tolower(pro);
        do
        {
            if (((pro == "") || (pro == "http")) && ((port_ == 80) || (port_ == -1)))
            {
                break;
            }
            if ((pro == "https") && ((port_ == 443) || (port_ == -1)))
            {
                break;
            }
            if ((pro == "ftp") && ((port_ == 21) || (port_ == -1)))
            {
                break;
            }
            if (port_ == -1)
            {
                break;
            }
            r.append(":").append(std::to_string(port_));
        } while (0);
    }
    // path
    r.append("/");
    if (!path_.empty())
    {
        if (path_[0] == '/')
        {
            r.pop_back();
        }
        r.append(path_);
    }
    // query
    if (!query_.empty())
    {
        r.append("?").append(query_);
    }
    else if (!querys_.empty())
    {
        std::string qs = build_query(querys_);
        r.append("?").append(qs);
    }
    // fragment
    if (!fragment_.empty())
    {
        r.append("#").append(fragment_);
    }
    return r;
}

void http_url::reset()
{
    protocol_.clear();
    destination_.clear();
    host_.clear();
    path_.clear();
    query_.clear();
    fragment_.clear();
    port_ = -1;
}

void http_url::debug_show()
{
    zcc_output("%s = %s", "scheme", protocol_.c_str());
    zcc_output("%s = %s", "host", host_.c_str());
    zcc_output("%s = %d", "port", port_);
    zcc_output("%s = %s", "destination", destination_.c_str());
    zcc_output("%s = %s", "path", path_.c_str());
    zcc_output("%s = %s", "query", query_.c_str());
    for (auto it = querys_.begin(); it != querys_.end(); it++)
    {
        zcc_output("    %s = %s", it->first.c_str(), it->second.c_str());
    }
    zcc_output("%s = %s\n", "fragment", fragment_.c_str());
}

zcc_namespace_end;

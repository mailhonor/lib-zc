/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2017-07-22
 * ================================
 */

#include "zc.h"

zurl_t *zurl_parse(const char *url_string)
{
    char *ps = (char *)(void *)url_string, *p;
    zurl_t *ru = (zurl_t *)zcalloc(1, sizeof(zurl_t));

    ru->scheme = zblank_buffer;
    ru->destination = zblank_buffer;
    ru->host = zblank_buffer;
    ru->path = zblank_buffer;
    ru->query = zblank_buffer;
    ru->fragment = zblank_buffer;
    ru->port = -1;

    p = strstr(ps, "://");
    if (!p) {
        if ((ps[0] == '/') && (ps[1] == '/')) {
            ru->scheme = zstrdup("http");
            ps = p + 2;
        } else {
        }
    } else {
        ru->scheme = zmemdupnull(ps, p-ps);
        zstr_tolower(ru->scheme);
        ps = p + 3;
    }
    char *tmp_destination = ps;

    if ((!strncmp(ps, "local:", 6))) {
        ru->host = zstrdup("local");
        p = ps + 6;
        while(1) {
            if ((*p == '?')||(*p == '#') || (*p == '\0')) {
                ru->path = zmemdupnull(ps + 6, p - (ps + 6));
                ru->destination = zmemdupnull(ps, p - ps);
                break;
            }
            p ++;
        }
        ps = p + 1;
        if (*p == '?') {
            goto query;
        }
        if (*p == '#') {
            goto fragment;
        }
        return ru;
    } 

    p = ps;
    while(1) {
        if ((*p == '?')||(*p == '#') || (*p == '/') || (*p == ':') || (*p == '\0')) {
            if (*p != ':') {
                ru->destination = zmemdupnull(ps, p - ps);
            }
            ru->host = zmemdupnull(ps, p - ps);
            break;
        }
        p ++;
    }
    ps = p + 1;
    if (*p == '?') {
        goto query;
    }
    if (*p == '#') {
        goto fragment;
    }
    if (*p == '/') {
        goto path;
    }
    if (*p == ':') {
        goto port;
    }
    return ru;

port:
    p = ps;
    while(1) {
        if ((*p == '?')||(*p == '#') || (*p == '/') || (*p == '\0')) {
            ru->destination = zmemdupnull(tmp_destination, p - tmp_destination);
            ru->port = atoi(ps);
            break;
        }
        p ++;
    }
    ps = p + 1;
    if (*p == '?') {
        goto query;
    }
    if (*p == '#') {
        goto fragment;
    }
    if (*p == '/') {
        goto path;
    }
    return ru;

path:
    p = ps;
    while(1) {
        if ((*p == '?')||(*p == '#')|| (*p == '\0')) {
            ru->path = zmemdupnull(ps, p - ps);
            break;
        }
        p ++;
    }
    ps = p + 1;
    if (*p == '?') {
        goto query;
    }
    if (*p == '#') {
        goto fragment;
    }
    return ru;

query:
    p = ps;
    while(1) {
        if ((*p == '#')|| (*p == '\0')) {
            ru->query = zmemdupnull(ps, p - ps);
            break;
        }
        p ++;
    }
    ps = p + 1;
    if (*p == '#') {
        goto fragment;
    }
    return ru;

fragment:
    ru->fragment = zstrdup(ps);
    return ru;
}

void zurl_free(zurl_t *url)
{
    zfree(url->scheme);
    zfree(url->destination);
    zfree(url->host);
    zfree(url->path);
    zfree(url->query);
    zfree(url->fragment);
    zfree(url);
}

void zurl_debug_show(zurl_t *url)
{
    zdebug_show("%s = %s", "scheme", url->scheme);
    zdebug_show("%s = %s", "host", url->host);
    zdebug_show("%s = %d", "port", url->port);
    zdebug_show("%s = %s", "destination", url->destination);
    zdebug_show("%s = %s", "path", url->path);
    zdebug_show("%s = %s", "query", url->query);
    zdebug_show("%s = %s", "fragment", url->fragment);
}

zdict_t *zurl_query_parse(const char *query, zdict_t *query_vars)
{
    char *q, *p, *ps = ZCONVERT_CHAR_PTR(query);
    zdict_t *vars = query_vars;
    zbuf_t *name = zbuf_create(32);
    zbuf_t *value = zbuf_create(128);
    if (vars == 0) {
        vars = zdict_create();
    }
    while(1) {
        p = ps;
        while((*p != '\0') && (*p != '&')) {
            p++;
        }
        do {
            q = ps;
            while(q < p) {
                if (*q  == '=') {
                    break;
                }
                q ++ ;
            }
            if (q == p) {
                break;
            }
            zbuf_memcpy(name, ps, q - ps);
            zstr_tolower(zbuf_data(name));
            zbuf_reset(value);
            q++;
            zurl_hex_decode(q, p - q, value);
        } while(0);
        zdict_update(vars, zbuf_data(name), value);
        if (*p == '\0') {
            break;
        }
        ps = p + 1;
    }
    zbuf_free(name);
    zbuf_free(value);
    return vars;
}

char *zurl_query_build(const zdict_t *query_vars, zbuf_t *query_buf, zbool_t strict)
{
    int first = 1;
    ZDICT_WALK_BEGIN(query_vars, key, val) {
        if (first) {
            first = 0;
        } else {
            zbuf_put(query_buf, '&');
        }

        zurl_hex_encode(key, strlen(key), query_buf, strict);
        zbuf_put(query_buf, '=');
        zurl_hex_encode(zbuf_data(val), zbuf_len(val), query_buf, strict);
    } ZDICT_WALK_END;
    return (char *)zbuf_data(query_buf);
}

/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2017-07-22
 * ================================
 */

#include "zc.h"
#include <ctype.h>
#include <time.h>

zdict_t *zhttp_cookie_parse(const char *raw_cookie, zdict_t *cookies)
{
    char *q, *p, *ps = ZCONVERT_CHAR_PTR(raw_cookie);
    zdict_t *result = cookies;
    zbuf_t *name = zbuf_create(32);
    zbuf_t *value = zbuf_create(128);

    if (result == 0) {
        result = zdict_create();
    }
    while(1) {
        while(*ps) {
            if ((*ps == ' ') || (*ps == '\t')) {
                ps ++;
                continue;
            }
            break;
        }
        p = ps;
        while((*p != '\0') && (*p != ';')) {
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
        zdict_update(result, zbuf_data(name), value);

        if (*p == '\0') {
            break;
        }
        ps = p + 1;
    }
    zbuf_free(name);
    zbuf_free(value);
    return result;
}

char *zhttp_cookie_build_item(const char *name, const char *value, long expires, const char *path, const char *domain, zbool_t secure, zbool_t httponly, zbuf_t *cookie_result)
{
    int ch;
    char *p;

    zbuf_strcat(cookie_result, name);
    zbuf_put(cookie_result, '=');
    if (zempty(value)) {
        zbuf_strcat(cookie_result, "deleted; expires=Thu, 01-Jan-1970 00:00:01 GMT; Max-Age=0;");
        return zbuf_data(cookie_result);
    }

    p = ZCONVERT_CHAR_PTR(value);
    while ((ch = *p++)) {
        if (ch == ' ') {
            zbuf_put(cookie_result, '+');
        } else if (isalnum(ch)) {
            zbuf_put(cookie_result, ch);
        } else {
            zbuf_put(cookie_result, '%');
            zbuf_put(cookie_result, ch<<4);
            zbuf_put(cookie_result, ch&0X0F);
        }
    }

    if (expires > 0) {
        struct tm tmbuf;
        char timestringbuf[64 + 1];
        gmtime_r((time_t *)expires, &tmbuf);
        strftime(timestringbuf, 64, "%a, %d %b %Y %H:%M:%S GMT", &tmbuf);
        zbuf_strcat(cookie_result, "; expires=");
        zbuf_strcat(cookie_result, timestringbuf);
        zbuf_strcat(cookie_result, "; Max-Age=");
        zbuf_printf_1024(cookie_result, "%ld", expires - time(0));
    }

    if (!zempty(path)) {
        zbuf_strcat(cookie_result, "; path=");
        p = ZCONVERT_CHAR_PTR(path);
        while ((ch = *p++)) {
            if (ch == ' ') {
                zbuf_put(cookie_result, '+');
            } else if (isalnum(ch) || (ch == '/')) {
                zbuf_put(cookie_result, ch);
            } else {
                zbuf_put(cookie_result, '%');
                zbuf_put(cookie_result, ch<<4);
                zbuf_put(cookie_result, ch&0X0F);
            }
        }
    }

    if (!zempty(domain)) {
        zbuf_strcat(cookie_result, "; domain=");
        p = ZCONVERT_CHAR_PTR(domain);
        while ((ch = *p++)) {
            if (ch == ' ') {
                zbuf_put(cookie_result, '+');
            } else if (isalnum(ch)) {
                zbuf_put(cookie_result, ch);
            } else {
                zbuf_put(cookie_result, '%');
                zbuf_put(cookie_result, ch<<4);
                zbuf_put(cookie_result, ch&0X0F);
            }
        }
    }

    if (secure) {
        zbuf_strcat(cookie_result, "; secure");
    }

    if (httponly) {
        zbuf_strcat(cookie_result, "; httponly");
    }

    return zbuf_data(cookie_result);
}


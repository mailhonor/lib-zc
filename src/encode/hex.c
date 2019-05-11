/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2015-12-06
 * ================================
 */

#include "zc.h"
#include <ctype.h>

void zhex_encode(const void *src, int src_size, zbuf_t *str)
{
    unsigned char dec2hex[18] = "0123456789ABCDEF";
    unsigned char *src_c = (unsigned char *)src;
    int src_pos;
    int addch1, addch2;
    for (src_pos = 0; src_pos < src_size; src_pos++) {
        addch1 = dec2hex[src_c[src_pos] >> 4];
        addch2 = dec2hex[src_c[src_pos] & 0X0F];
        ZBUF_PUT(str, addch1);
        ZBUF_PUT(str, addch2);
    }
    zbuf_terminate(str);
}

void zhex_decode(const void *src, int src_size, zbuf_t *str)
{
    unsigned char *src_c = (unsigned char *)src;
    int src_pos;
    unsigned char h_l, h_r;
    int addch;
    for (src_pos = 0; src_pos + 1 < src_size; src_pos += 2) {
        h_l = zchar_xdigitval_vector[src_c[src_pos]] << 4;
        h_r = zchar_xdigitval_vector[src_c[src_pos + 1]];
        addch = (h_l | h_r);
        ZBUF_PUT(str, addch);
    }
    zbuf_terminate(str);
}

void zurl_hex_decode(const void *src, int src_size, zbuf_t *str)
{
    int l, r;
    char *p = (char *)src;
    for (int i = 0; i < src_size; i++) {
        if (p[i] == '+') {
            ZBUF_PUT(str, ' ');
        } else if (p[i] == '%') {
            if (i + 3 > src_size) {
                break;
            }
            l = zchar_xdigitval_vector[(int)(p[i+1])];
            r = zchar_xdigitval_vector[(int)(p[i+2])];
            if ((l!=-1) && (r!=-1)) {
                zbuf_put(str, (l<<4)+(r));
            }
            i += 2;
        } else {
            zbuf_put(str, p[i]);
        }
    }
    zbuf_terminate(str);
}

void zurl_hex_encode(const void *src, int src_size, zbuf_t *str, int strict_flag)
{
    unsigned char dec2hex[18] = "0123456789ABCDEF";
    const char *ps = (const char *)src;
    for (int i = 0; i < src_size; i++) {
        unsigned char ch = ps[i];
        if (ch == ' ') {
            ZBUF_PUT(str, '+');
            continue;
        }
        if (isalnum(ch)) {
            ZBUF_PUT(str, ch);
            continue;
        }
        if (strict_flag) {
            ZBUF_PUT(str, '%');
            zbuf_put(str, dec2hex[ch>>4]);
            zbuf_put(str, dec2hex[ch&0X0F]);
            continue;
        } 
        if (ch > 127) {
            ZBUF_PUT(str, ch);
            continue;
        }
        if (strchr("._-", ch)) {
            ZBUF_PUT(str, ch);
            continue;
        }
        ZBUF_PUT(str, '%');
        zbuf_put(str, dec2hex[ch>>4]);
        zbuf_put(str, dec2hex[ch&0X0F]);
    }
    zbuf_terminate(str);
}

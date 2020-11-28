/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-12-06
 * ================================
 */

#ifndef ___ZC_ZCC_MODE___
#include "zc.h"
#include <ctype.h>
#endif

#ifndef ___ZC_ZCC_MODE___
void zhex_encode(const void *src, int src_size, zbuf_t *str)
#else
void hex_encode(const void *src, int src_size, std::string &str)
#endif
{
    unsigned char dec2hex[18] = "0123456789ABCDEF";
    unsigned char *src_c = (unsigned char *)src;
    int src_pos;
    int addch1, addch2;
    if (src_size < 0) {
        src_size = strlen((const char *)src);
    }
    for (src_pos = 0; src_pos < src_size; src_pos++) {
        addch1 = dec2hex[src_c[src_pos] >> 4];
        addch2 = dec2hex[src_c[src_pos] & 0X0F];
        zbuf_put_cpp(str, addch1);
        zbuf_put_cpp(str, addch2);
    }
    zbuf_terminate_cpp(str);
}

#ifndef ___ZC_ZCC_MODE___
void zhex_decode(const void *src, int src_size, zbuf_t *str)
#else
void hex_decode(const void *src, int src_size, std::string &str)
#endif
{
    unsigned char *src_c = (unsigned char *)src;
    int src_pos;
    unsigned char h_l, h_r;
    int addch;
    if (src_size < 0) {
        src_size = strlen((const char *)src);
    }
    for (src_pos = 0; src_pos + 1 < src_size; src_pos += 2) {
        h_l = zchar_xdigitval_vector[src_c[src_pos]] << 4;
        h_r = zchar_xdigitval_vector[src_c[src_pos + 1]];
        addch = (h_l | h_r);
        zbuf_put_cpp(str, addch);
    }
    zbuf_terminate_cpp(str);
}

#ifndef ___ZC_ZCC_MODE___
void zurl_hex_decode(const void *src, int src_size, zbuf_t *str)
#else
void url_hex_decode(const void *src, int src_size, std::string &str)
#endif
{
    int l, r;
    char *p = (char *)src;
    if (src_size < 0) {
        src_size = strlen((const char *)src);
    }
    for (int i = 0; i < src_size; i++) {
        if (p[i] == '+') {
            zbuf_put_cpp(str, ' ');
        } else if (p[i] == '%') {
            if (i + 3 > src_size) {
                break;
            }
            l = zchar_xdigitval_vector[(int)(p[i+1])];
            r = zchar_xdigitval_vector[(int)(p[i+2])];
            if ((l!=-1) && (r!=-1)) {
                zbuf_put_cpp(str, (l<<4)+(r));
            }
            i += 2;
        } else {
            zbuf_put_cpp(str, p[i]);
        }
    }
    zbuf_terminate_cpp(str);
}

#ifndef ___ZC_ZCC_MODE___
void zurl_hex_encode(const void *src, int src_size, zbuf_t *str, int strict_flag)
#else
void url_hex_encode(const void *src, int src_size, std::string &str, bool strict_flag)
#endif
{
    unsigned char dec2hex[18] = "0123456789ABCDEF";
    const char *ps = (const char *)src;
    if (src_size < 0) {
        src_size = strlen((const char *)src);
    }
    for (int i = 0; i < src_size; i++) {
        unsigned char ch = ps[i];
        if (ch == ' ') {
            zbuf_put_cpp(str, '+');
            continue;
        }
        if (isalnum(ch)) {
            zbuf_put_cpp(str, ch);
            continue;
        }
        if (strict_flag) {
            zbuf_put_cpp(str, '%');
            zbuf_put_cpp(str, dec2hex[ch>>4]);
            zbuf_put_cpp(str, dec2hex[ch&0X0F]);
            continue;
        } 
        if (ch > 127) {
            zbuf_put_cpp(str, ch);
            continue;
        }
        if (strchr("._-", ch)) {
            zbuf_put_cpp(str, ch);
            continue;
        }
        zbuf_put_cpp(str, '%');
        zbuf_put_cpp(str, dec2hex[ch>>4]);
        zbuf_put_cpp(str, dec2hex[ch&0X0F]);
    }
    zbuf_terminate_cpp(str);
}

/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
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
    if (src_size < 0)
    {
        src_size = strlen((const char *)src);
    }
    for (src_pos = 0; src_pos < src_size; src_pos++)
    {
        addch1 = dec2hex[src_c[src_pos] >> 4];
        addch2 = dec2hex[src_c[src_pos] & 0X0F];
        zbuf_put(str, addch1);
        zbuf_put(str, addch2);
    }
    zbuf_terminate(str);
}

void zhex_decode(const void *src, int src_size, zbuf_t *str)
{
    unsigned char *src_c = (unsigned char *)src;
    int src_pos;
    unsigned char h_l, h_r;
    int addch;
    if (src_size < 0)
    {
        src_size = strlen((const char *)src);
    }
    for (src_pos = 0; src_pos + 1 < src_size; src_pos += 2)
    {
        h_l = zchar_xdigitval_vector[src_c[src_pos]] << 4;
        h_r = zchar_xdigitval_vector[src_c[src_pos + 1]];
        addch = (h_l | h_r);
        zbuf_put(str, addch);
    }
    zbuf_terminate(str);
}

void zurl_hex_decode(const void *src, int src_size, zbuf_t *str)
{
    int l, r;
    char *p = (char *)src;
    if (src_size < 0)
    {
        src_size = strlen((const char *)src);
    }
    for (int i = 0; i < src_size; i++)
    {
        if (p[i] == '%')
        {
            if (i + 3 > src_size)
            {
                break;
            }
            l = zchar_xdigitval_vector[(int)(p[i + 1])];
            r = zchar_xdigitval_vector[(int)(p[i + 2])];
            if ((l != -1) && (r != -1))
            {
                zbuf_put(str, (l << 4) + (r));
            }
            i += 2;
        }
        else
        {
            zbuf_put(str, p[i]);
        }
    }
    zbuf_terminate(str);
}

void zurl_hex_encode(const void *src, int src_size, zbuf_t *str, int strict_flag)
{
    unsigned char dec2hex[18] = "0123456789ABCDEF";
    const char *ps = (const char *)src;
    if (src_size < 0)
    {
        src_size = strlen((const char *)src);
    }
    for (int i = 0; i < src_size; i++)
    {
        unsigned char ch = ps[i];
        if (ch == ' ')
        {
            zbuf_strcat(str, "%20");
            continue;
        }
        if (isalnum(ch))
        {
            zbuf_put(str, ch);
            continue;
        }
        if (strict_flag)
        {
            zbuf_put(str, '%');
            zbuf_put(str, dec2hex[ch >> 4]);
            zbuf_put(str, dec2hex[ch & 0X0F]);
            continue;
        }
        if (ch > 127)
        {
            zbuf_put(str, ch);
            continue;
        }
        if (strchr("._-", ch))
        {
            zbuf_put(str, ch);
            continue;
        }
        zbuf_put(str, '%');
        zbuf_put(str, dec2hex[ch >> 4]);
        zbuf_put(str, dec2hex[ch & 0X0F]);
    }
    zbuf_terminate(str);
}

/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2021-04-23
 * ================================
 */

#include "zc.h"

void zuudecode(const void *src, int src_size, zbuf_t *str)
{
    const unsigned char *s = (const unsigned char *)src;
    if (src_size < 0) {
        src_size = strlen((const char *)s);
    }
    const unsigned char *e = s + src_size;

    while (s < e - 4) {
        int v = 0;
        int i;
        for (i = 0; i < 4; i += 1) {
            int c = *s++;
            v = v << 6 | ((c - 0x20) & 0x3F);
        }
        for (i = 2; i >= 0; i -= 1) {
            int c = (v & 0xFF);
            zbuf_put(str, c);
            v = v >> 8;
        }
    }
    while (s < e) {
        int c = *s++;
        zbuf_put(str, c);
    }

    zbuf_terminate(str);
}

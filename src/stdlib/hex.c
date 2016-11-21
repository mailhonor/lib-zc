/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-06
 * ================================
 */

#include "libzc.h"

int zhex_encode(const void *src, int src_size, zbuf_t *dest)
{
    unsigned char dec2hex[16] = "0123456789ABCDEF";
    unsigned char *src_c = (unsigned char *)src;
    int src_pos;
    int old_len = ZBUF_LEN(dest);
    int addch;

    for (src_pos = 0; src_pos < src_size; src_pos++) {
        addch = dec2hex[src_c[src_pos] >> 4];
        ZBUF_PUT(dest, addch);
        addch = dec2hex[src_c[src_pos] & 0X0F];
        ZBUF_PUT(dest, addch);
    }
    ZBUF_TERMINATE(dest);

    return ZBUF_LEN(dest) - old_len;
}

char zhex_to_dec_list[256] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    //  0  1  2  3  4  5  6  7  8  9
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1, -1, -1, -1, -1, -1, -1,
    //  A   B   C   D   E   F
    10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    //  a   b   c   d   e   f
    10, 11, 12, 13, 14, 15,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1
};

int zhex_decode(const void *src, int src_size, zbuf_t *dest)
{
    unsigned char *src_c = (unsigned char *)src;
    int src_pos;
    unsigned char h_l, h_r;
    int addch;
    int old_len = ZBUF_LEN(dest);

    for (src_pos = 0; src_pos + 1 < src_size; src_pos += 2) {
        h_l = zhex_to_dec_list[src_c[src_pos] << 4];
        h_r = zhex_to_dec_list[src_c[src_pos + 1]];
        addch = (h_l | h_r);
        ZBUF_PUT(dest, addch);
    }
    ZBUF_TERMINATE(dest);

    return ZBUF_LEN(dest) - old_len;
}

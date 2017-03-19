/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-06
 * ================================
 */

#include "zc.h"

int zhex_encode(const void *src, int src_size, char *dest, int dest_size)
{
    unsigned const char dec2hex[17] = "0123456789ABCDEF";
    unsigned char *src_c = (unsigned char *)src;
    int src_pos;
    int addch;
    int rlen = 0;

    for (src_pos = 0; src_pos < src_size; src_pos++) {
        if ((dest_size > 0) && (rlen+2 > dest_size)) {
            break;
        }
        addch = dec2hex[src_c[src_pos] >> 4];
        Z_DF_ADD_CHAR(dest_size, dest, rlen, addch);
        addch = dec2hex[src_c[src_pos] & 0X0F];
        Z_DF_ADD_CHAR(dest_size, dest, rlen, addch);
    }

    return rlen;
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

int zhex_decode(const void *src, int src_size, char *dest, int dest_size)
{
    unsigned char *src_c = (unsigned char *)src;
    int src_pos;
    unsigned char h_l, h_r;
    int addch;
    int rlen = 0;

    for (src_pos = 0; src_pos + 1 < src_size; src_pos += 2) {
        if ((dest_size > 0) && (rlen+1 > dest_size)) {
            break;
        }
        h_l = zhex_to_dec_list[src_c[src_pos] << 4];
        h_r = zhex_to_dec_list[src_c[src_pos + 1]];
        addch = (h_l | h_r);
        Z_DF_ADD_CHAR(dest_size, dest, rlen, addch);
    }

    return rlen;
}

/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-06
 * ================================
 */

int zhex_encode(void *src, int src_size, void *dest)
{
    unsigned char dec2hex[16] = "0123456789ABCDEF";
    unsigned char *src_c = src;
    int src_pos;
    unsigned char *dest_result = (unsigned char *)dest;
    int len_result = 0;

    for (src_pos = 0; src_pos < src_size; src_pos++) {
        dest_result[len_result++] = dec2hex[src_c[src_pos] >> 4];
        dest_result[len_result++] = dec2hex[src_c[src_pos] & 0X0F];
    }

    return len_result;
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

int zhex_decode(void *src, int src_size, void *dest)
{
    unsigned char *src_c = src;
    int src_pos;
    unsigned char *dest_result = (unsigned char *)dest;
    int len_result = 0;
    unsigned char h_l, h_r;

    for (src_pos = 0; src_pos + 1 < src_size; src_pos += 2) {
        h_l = zhex_to_dec_list[src_c[src_pos] << 4];
        h_r = zhex_to_dec_list[src_c[src_pos + 1]];

        dest_result[len_result++] = (h_l | h_r);
    }

    return len_result;
}

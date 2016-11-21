/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-02
 * ================================
 */

#include "libzc.h"

/* should check c1 and c2 are hex */
#define ___hex_val(ccc) { ccc = zhex_to_dec_list[ccc];}

#define ___get_next_ch(c0123)    while(1){ \
    if(src_pos >= src_size){ goto over; } \
    c0123 = src_c[src_pos++]; \
    break; \
}

int zqp_decode_2045(const void *src, int src_size, zbuf_t *dest)
{
    unsigned char *src_c = (unsigned char *)src;
    int src_pos = 0;
    unsigned char c0, c1, c2;
    unsigned char addch;
    int old_len = ZBUF_LEN(dest);

    while (1) {
        ___get_next_ch(c0);
        if (c0 != '=') {
            ZBUF_PUT(dest, c0);
            continue;
        }
        ___get_next_ch(c1);
        if (c1 == '\r' || c1 == '\n') {
            ___get_next_ch(c2);
            if (c2 != '\r' && c2 != '\n') {
                src_pos--;
            }
            continue;
        }
        ___get_next_ch(c2);
        ___hex_val(c1);
        ___hex_val(c2);
        addch = ((c1 << 4) | c2);
        ZBUF_PUT(dest, addch);
    }
  over:
    ZBUF_TERMINATE(dest);

    return ZBUF_LEN(dest) - old_len;
}

int zqp_decode_2047(const void *src, int src_size, zbuf_t *dest)
{
    unsigned char *src_c = (unsigned char *)src;
    int src_pos = 0;
    unsigned char c0, c1, c2;
    unsigned char addch;
    int old_len = ZBUF_LEN(dest);

    while (1) {
        ___get_next_ch(c0);
        if (c0 == '_') {
            addch = ' ';
        } else if (c0 != '=') {
            addch = c0;
        } else {
            ___get_next_ch(c1);
            ___get_next_ch(c2);
            ___hex_val(c1);
            ___hex_val(c2);
            addch = (c1 << 4 | c2);
        }
        ZBUF_PUT(dest, addch);
    }

  over:
    ZBUF_TERMINATE(dest);

    return ZBUF_LEN(dest) - old_len;
}

int zqp_decode_get_valid_len(const void *src, int src_size)
{
    unsigned char *src_c = (unsigned char *)src;
    int i;
    unsigned char ch;

    for (i = 0; i < src_size; i++) {
        ch = src_c[i];
        if ((ch < 33) || (ch > 126)) {
            return i;
        }
    }

    return src_size;
}

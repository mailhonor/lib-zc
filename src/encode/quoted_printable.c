/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2015-12-02
 * ================================
 */

#ifndef ___ZC_ZCC_MODE___
#include "zc.h"
#include <ctype.h>
#endif

static const char hex[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

#ifndef ___ZC_ZCC_MODE___
void zqp_encode_2045(const void *src, int src_size, zbuf_t *result, zbool_t mime_flag)
#else
void qp_encode_2045(const void *src, int src_size, std::string &result, bool mime_flag)
#endif
{
    int col_count = 0, i, byte;
    const char *ptr = (const char *)src;
    if (src_size < 0) {
        src_size = strlen(ptr);
    }
    for (i = 0; i < src_size; ++i) {
        byte = ptr[i];
        if ((byte == ' ') || ((byte >= 33) && (byte <= 126)  && (byte != '='))) {
            zbuf_put_cpp(result, byte);
            col_count++;
        } else {
            zbuf_put_cpp(result, '=');
            zbuf_put_cpp(result, hex[((byte >> 4) & 0x0F)]);
            zbuf_put_cpp(result, hex[(byte & 0x0F)]);
            col_count += 3;
        }
        if (col_count > 76) {
            if (mime_flag) {
#ifndef ___ZC_ZCC_MODE___
                zbuf_strcat(result, "=\r\n");
#else
                result.append("=\r\n");
#endif
            }
            col_count = 0;
        }
    }
}

#ifndef ___ZC_ZCC_MODE___
void zqp_encode_2047(const void *src, int src_size, zbuf_t *result)
#else
void qp_encode_2047(const void *src, int src_size, std::string &result)
#endif
{
    const char *ptr = (const char *)src;
    int i, byte;
    if (src_size < 0) {
        src_size = strlen(ptr);
    }
    for (i = 0; i < src_size; ++i) {
        byte = ptr[i];
        if (byte == ' ') {
            zbuf_put_cpp(result, byte);
            zbuf_put_cpp(result, '_');
        } else if (((byte >= 33) && (byte <= 126)  && (byte != '='))) {
            zbuf_put_cpp(result, byte);
        } else {
            zbuf_put_cpp(result, '=');
            zbuf_put_cpp(result, hex[((byte >> 4) & 0x0F)]);
            zbuf_put_cpp(result, hex[(byte & 0x0F)]);
        }
    }
}

/* should check c1 and c2 are hex */
#define ___hex_val(ccc) { ccc = zchar_xdigitval_vector[(unsigned char)ccc];}

#define ___get_next_ch(c0123)    while(1){ \
    if(src_pos >= src_size){ goto over; } \
    c0123 = src_c[src_pos++]; \
    break; \
}

#ifndef ___ZC_ZCC_MODE___
void zqp_decode_2045(const void *src, int src_size, zbuf_t *str)
#else
void qp_decode_2045(const void *src, int src_size, std::string &str)
#endif
{
    unsigned char *src_c = (unsigned char *)src;
    int src_pos = 0;
    char c0, c1, c2;
    char addch;

    if (src_size < 0) {
        src_size = strlen((const char *)src);
    }

    while (1) {
        ___get_next_ch(c0);
        if (c0 != '=') {
            addch = c0;
            goto append;
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
append:
        zbuf_put_cpp(str, addch);
    }

over:
    zbuf_terminate_cpp(str);
}

#ifndef ___ZC_ZCC_MODE___
void zqp_decode_2047(const void *src, int src_size, zbuf_t *str)
#else
void qp_decode_2047(const void *src, int src_size, std::string &str)
#endif
{
    unsigned char *src_c = (unsigned char *)src;
    int src_pos = 0;
    char c0, c1, c2;
    char addch;

    if (src_size < 0) {
        src_size = strlen((const char *)src);
    }

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
        zbuf_put_cpp(str, addch);
    }

over:
    zbuf_terminate_cpp(str);
}
#undef ___get_next_ch

#ifndef ___ZC_ZCC_MODE___
int zqp_decode_get_valid_len(const void *src, int src_size)
{
    unsigned char *src_c = (unsigned char *)src;
    int i;
    unsigned char ch;

    if (src_size < 0) {
        src_size = strlen((const char *)src);
    }

    for (i = 0; i < src_size; i++) {
        ch = src_c[i];
        if ((ch < 33) || (ch > 126)) {
            return i;
        }
    }

    return src_size;
}
#endif

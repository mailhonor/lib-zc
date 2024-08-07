/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-12-02
 * ================================
 */

#include "zcc/zcc_stdlib.h"

zcc_namespace_begin;

static const char b64enc[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

const unsigned char var_base64_decode_table[256] = {
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0x3e,
    0xff,
    0xff,
    0xff,
    0x3f,
    0x34,
    0x35,
    0x36,
    0x37,
    0x38,
    0x39,
    0x3a,
    0x3b,
    0x3c,
    0x3d,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0x00,
    0x01,
    0x02,
    0x03,
    0x04,
    0x05,
    0x06,
    0x07,
    0x08,
    0x09,
    0x0a,
    0x0b,
    0x0c,
    0x0d,
    0x0e,
    0x0f,
    0x10,
    0x11,
    0x12,
    0x13,
    0x14,
    0x15,
    0x16,
    0x17,
    0x18,
    0x19,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0x1a,
    0x1b,
    0x1c,
    0x1d,
    0x1e,
    0x1f,
    0x20,
    0x21,
    0x22,
    0x23,
    0x24,
    0x25,
    0x26,
    0x27,
    0x28,
    0x29,
    0x2a,
    0x2b,
    0x2c,
    0x2d,
    0x2e,
    0x2f,
    0x30,
    0x31,
    0x32,
    0x33,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,

    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
};

void base64_encode(const void *src, int src_size, std::string &str, bool mime_flag)
{
    unsigned char *src_c = (unsigned char *)src;
    char tmp[5] = {0, 0, 0, 0, 0};
    int src_pos;
    int mime_count = 0;

    if (src_size < 0)
    {
        src_size = std::strlen((const char *)src);
    }

    for (src_pos = 0; src_pos < src_size;)
    {
        tmp[0] = b64enc[src_c[src_pos] >> 2];
        switch (src_size - src_pos)
        {
        case 1:
            tmp[1] = b64enc[(src_c[src_pos] & 0x03) << 4];
            tmp[2] = '=';
            tmp[3] = '=';
            src_pos++;
            break;
        case 2:
            tmp[1] = b64enc[((src_c[src_pos] & 0x03) << 4) | (src_c[src_pos + 1] >> 4)];
            tmp[2] = b64enc[((src_c[src_pos + 1] & 0x0f) << 2)];
            tmp[3] = '=';
            src_pos += 2;
            break;
        default:
            tmp[1] = b64enc[((src_c[src_pos] & 0x03) << 4) | (src_c[src_pos + 1] >> 4)];
            tmp[2] = b64enc[((src_c[src_pos + 1] & 0x0f) << 2) | ((src_c[src_pos + 2] & 0xc0) >> 6)];
            tmp[3] = b64enc[src_c[src_pos + 2] & 0x3f];
            src_pos += 3;
            break;
        }

        str.push_back(tmp[0]);
        str.push_back(tmp[1]);
        str.push_back(tmp[2]);
        str.push_back(tmp[3]);

        if (mime_flag)
        {
            mime_count++;
            if (mime_count == 19)
            {
                mime_count = 0;
                str.push_back('\r');
                str.push_back('\n');
            }
        }
    }
}

std::string base64_encode(const void *src, int src_size, bool mime_flag)
{
    std::string str;
    base64_encode(src, src_size, str, mime_flag);
    return str;
}

void base64_decode(const void *src, int src_size, std::string &str)
{
    unsigned char *src_c = (unsigned char *)src;
    int src_pos = 0;
    unsigned char input[4], output[3];
    int ret = -1;
    unsigned char c0, c1, c2, c3;
#if 0
    int illegal = 0;
#endif

    if (src_size < 0)
    {
        src_size = std::strlen((const char *)src);
    }

#define ___get_next_ch(c0123, br)                           \
    while (1)                                               \
    {                                                       \
        if (src_pos >= src_size)                            \
        {                                                   \
            if (br)                                         \
            {                                               \
                c0123 = '=';                                \
                break;                                      \
            }                                               \
            goto over;                                      \
        }                                                   \
        c0123 = src_c[src_pos++];                           \
        if (c0123 == ' ' || c0123 == '\r' || c0123 == '\n') \
        {                                                   \
            continue;                                       \
        }                                                   \
        if (c0123 == '%')                                   \
        {                                                   \
            src_pos += 2;                                   \
            continue;                                       \
        }                                                   \
        break;                                              \
    }

retry:
    ret = -1;
    while (src_pos < src_size)
    {
        ret = -1;
        ___get_next_ch(c0, 0);
        ___get_next_ch(c1, 0);
        ___get_next_ch(c2, 1);
        ___get_next_ch(c3, 1);
        input[0] = var_base64_decode_table[c0];
        if (input[0] == 0xff)
        {
#if 0
            illegal = 1;
#endif
            break;
        }

        input[1] = var_base64_decode_table[c1];
        if (input[1] == 0xff)
        {
#if 0
            illegal = 1;
#endif
            break;
        }
        output[0] = (input[0] << 2) | (input[1] >> 4);

        input[2] = var_base64_decode_table[c2];
        if (input[2] == 0xff)
        {
            if (c2 != '=' || c3 != '=')
            {
#if 0
                illegal = 1;
#endif
                break;
            }
            str.push_back(output[0]);
            ret = 1;
            break;
        }

        output[1] = (input[1] << 4) | (input[2] >> 2);
        input[3] = var_base64_decode_table[c3];
        if (input[3] == 0xff)
        {
            if (c3 != '=')
            {
#if 0
                illegal = 1;
#endif
                break;
            }
            str.push_back(output[0]);
            str.push_back(output[1]);
            ret = 1;
            break;
        }

        output[2] = ((input[2] << 6) & 0xc0) | input[3];
        str.push_back(output[0]);
        str.push_back(output[1]);
        str.push_back(output[2]);
    }

    if (ret == 1)
    {
        goto retry;
    }
over:
    return;
#undef ___get_next_ch
}

std::string base64_decode(const void *src, int src_size)
{
    std::string str;
    base64_decode(src, src_size, str);
    return str;
}

int base64_decode_get_valid_len(const void *src, int src_size)
{
    unsigned char *src_c = (unsigned char *)src, ch;
    int i;

    if (src_size < 0)
    {
        src_size = std::strlen((const char *)src);
    }

    for (i = 0; i < src_size; i++)
    {
        ch = src_c[i];
        if ((ch == '\r') || (ch == '\n') || (ch == '='))
        {
            continue;
        }
        if (var_base64_decode_table[ch] == 0xff)
        {
            return i;
        }
    }

    return src_size;
}

int base64_encode_get_min_len(int in_len, bool mime_flag)
{
    int ret;

    ret = in_len * 4 / 3 + 4;
    if (mime_flag)
    {
        ret += ((in_len / (76 * 3 / 4)) + 1) * 2 + 2;
    }
    ret += 1;

    return ret;
}

zcc_namespace_end;

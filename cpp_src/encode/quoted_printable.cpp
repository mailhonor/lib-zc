/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-12-02
 * ================================
 */

#include "zcc/zcc_stdlib.h"

zcc_namespace_begin;

static const char hex[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

void qp_encode_2045(const void *src, int src_size, std::string &result, bool mime_flag)
{
    int col_count = 0, i, byte;
    const char *ptr = (const char *)src;
    if (src_size < 0)
    {
        src_size = std::strlen(ptr);
    }
    for (i = 0; i < src_size; ++i)
    {
        byte = ptr[i];
        if ((byte == ' ') || ((byte >= 33) && (byte <= 126) && (byte != '=')))
        {
            result.push_back(byte);
            col_count++;
        }
        else
        {
            result.push_back('=');
            result.push_back(hex[((byte >> 4) & 0x0F)]);
            result.push_back(hex[(byte & 0x0F)]);
            col_count += 3;
        }
        if (col_count > 76)
        {
            if (mime_flag)
            {
                result.append("=\r\n");
            }
            col_count = 0;
        }
    }
}

void qp_encode_2047(const void *src, int src_size, std::string &result)
{
    const char *ptr = (const char *)src;
    int i, byte;
    if (src_size < 0)
    {
        src_size = std::strlen(ptr);
    }
    for (i = 0; i < src_size; ++i)
    {
        byte = ptr[i];
        if (byte == ' ')
        {
            result.push_back(byte);
            result.push_back('_');
        }
        else if (((byte >= 33) && (byte <= 126) && (byte != '=')))
        {
            result.push_back(byte);
        }
        else
        {
            result.push_back('=');
            result.push_back(hex[((byte >> 4) & 0x0F)]);
            result.push_back(hex[(byte & 0x0F)]);
        }
    }
}

std::string qp_encode_2045(const void *src, int src_size, bool mime_flag)
{
    std::string result;
    qp_encode_2045(src, src_size, result, mime_flag);
    return result;
}

/* should check c1 and c2 are hex */
#define ___hex_val(ccc)             \
    {                               \
        ccc = hex_char_to_int(ccc); \
    }

#define ___get_next_ch(c0123)     \
    while (1)                     \
    {                             \
        if (src_pos >= src_size)  \
        {                         \
            goto over;            \
        }                         \
        c0123 = src_c[src_pos++]; \
        break;                    \
    }

void qp_decode_2045(const void *src, int src_size, std::string &str)
{
    unsigned char *src_c = (unsigned char *)src;
    int src_pos = 0;
    char c0, c1, c2;
    char addch;

    if (src_size < 0)
    {
        src_size = std::strlen((const char *)src);
    }

    while (1)
    {
        ___get_next_ch(c0);
        if (c0 != '=')
        {
            addch = c0;
            goto append;
        }
        ___get_next_ch(c1);
        if (c1 == '\r' || c1 == '\n')
        {
            ___get_next_ch(c2);
            if (c2 != '\r' && c2 != '\n')
            {
                src_pos--;
            }
            continue;
        }
        ___get_next_ch(c2);
        ___hex_val(c1);
        ___hex_val(c2);
        addch = ((c1 << 4) | c2);
    append:
        str.push_back(addch);
    }

over:
    return;
}

std::string qp_decode_2045(const void *src, int src_size)
{
    std::string result;
    qp_decode_2045(src, src_size, result);
    return result;
}

void qp_decode_2047(const void *src, int src_size, std::string &str)
{
    unsigned char *src_c = (unsigned char *)src;
    int src_pos = 0;
    char c0, c1, c2;
    char addch;

    if (src_size < 0)
    {
        src_size = std::strlen((const char *)src);
    }

    while (1)
    {
        ___get_next_ch(c0);
        if (c0 == '_')
        {
            addch = ' ';
        }
        else if (c0 != '=')
        {
            addch = c0;
        }
        else
        {
            ___get_next_ch(c1);
            ___get_next_ch(c2);
            ___hex_val(c1);
            ___hex_val(c2);
            addch = (c1 << 4 | c2);
        }
        str.push_back(addch);
    }

over:
    return;
}
#undef ___get_next_ch

std::string qp_decode_2047(const void *src, int src_size)
{
    std::string result;
    qp_decode_2047(src, src_size, result);
    return result;
}

int qp_decode_get_valid_len(const void *src, int src_size)
{
    unsigned char *src_c = (unsigned char *)src;
    int i;
    unsigned char ch;

    if (src_size < 0)
    {
        src_size = std::strlen((const char *)src);
    }

    for (i = 0; i < src_size; i++)
    {
        ch = src_c[i];
        if ((ch < 33) || (ch > 126))
        {
            return i;
        }
    }

    return src_size;
}

zcc_namespace_end;

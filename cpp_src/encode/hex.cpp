/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-12-06
 * ================================
 */

#include "zcc/zcc_stdlib.h"

zcc_namespace_begin;

void hex_encode(const void *src, int src_size, std::string &str)
{
    unsigned char dec2hex[18] = "0123456789ABCDEF";
    unsigned char *src_c = (unsigned char *)src;
    int src_pos;
    int addch1, addch2;
    if (src_size < 0)
    {
        src_size = std::strlen((const char *)src);
    }
    for (src_pos = 0; src_pos < src_size; src_pos++)
    {
        addch1 = dec2hex[src_c[src_pos] >> 4];
        addch2 = dec2hex[src_c[src_pos] & 0X0F];
        str.push_back(addch1);
        str.push_back(addch2);
    }
}

std::string hex_encode(const void *src, int src_size)
{
    std::string str;
    hex_encode(src, src_size, str);
    return str;
}

void hex_decode(const void *src, int src_size, std::string &str)
{
    unsigned char *src_c = (unsigned char *)src;
    int src_pos;
    unsigned char h_l, h_r;
    int addch;
    if (src_size < 0)
    {
        src_size = std::strlen((const char *)src);
    }
    for (src_pos = 0; src_pos + 1 < src_size; src_pos += 2)
    {
        h_l = var_char_xdigitval_vector[src_c[src_pos]] << 4;
        h_r = var_char_xdigitval_vector[src_c[src_pos + 1]];
        addch = (h_l | h_r);
        str.push_back(addch);
    }
}

std::string hex_decode(const void *src, int src_size)
{
    std::string str;
    hex_decode(src, src_size, str);
    return str;
}

zcc_namespace_end;

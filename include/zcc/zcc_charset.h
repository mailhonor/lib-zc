/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-12-09
 * ================================
 */

#pragma once

#ifndef ZCC_LIB_INCLUDE_CHARSET___
#define ZCC_LIB_INCLUDE_CHARSET___

#include "./zcc_stdlib.h"
#include <functional>

#ifdef __cplusplus
#pragma pack(push, 4)
zcc_namespace_begin;
zcc_general_namespace_begin(charset);

#pragma pack(push, 8)
struct detect_data
{
    const char *data2;
    int count2;
    const char *data3[16];
    int count3[16];
};
#pragma pack(pop)


extern bool var_debug_mode;
extern const bool var_uconv_supported;
static const int var_name_max_size = 32;
extern detect_data *var_default_detect_data;

const char *correct_name(const char *charset);
inline const char *correct_name(const std::string &charset)
{
    return correct_name(charset.c_str());
}
extern int (*convert)(const char *from_charset, const char *src, int src_len, const char *to_charset, std::string &dest, int *src_converted_len, int omit_invalid_bytes_limit, int *omit_invalid_bytes_count);
int iconv_convert(const char *from_charset, const char *src, int src_len, const char *to_charset, std::string &dest, int *src_converted_len, int omit_invalid_bytes_limit, int *omit_invalid_bytes_count);
int uconv_convert(const char *from_charset, const char *src, int src_len, const char *to_charset, std::string &dest, int *src_converted_len, int omit_invalid_bytes_limit, int *omit_invalid_bytes_count);
void use_uconv();
std::string detect(detect_data *dd, const char **charset_list, const char *data, int size);
std::string detect_1252(detect_data *dd, const char *data, int size);
inline std::string detect_1252(const char *data, int size)
{
    return detect_1252(nullptr, data, size);
}
std::string detect_cjk(const char *data, int size);
inline std::string detect_cjk(const std::string &data)
{
    return detect_cjk(data.c_str(), (int)data.size());
}
extern const char *var_chinese[];
extern const char *var_japanese[];
extern const char *var_korean[];
extern const char *var_cjk[];

std::string convert_to_utf8(const char *from_charset, const char *data, int size);
inline std::string convert_to_utf8(const char *data, int size)
{
    return convert_to_utf8(0, data, size);
}
inline std::string convert_to_utf8(const char *from_charset, const std::string &data)
{
    return convert_to_utf8(from_charset, data.c_str(), data.size());
}
inline std::string convert_to_utf8(const std::string &from_charset, const std::string &data)
{
    return convert_to_utf8(from_charset.c_str(), data.c_str(), data.size());
}
inline std::string convert_to_utf8(const std::string &data)
{
    return convert_to_utf8(0, data.c_str(), data.size());
}

std::string utf8_tail_complete(const std::string &s);

static inline int utf8_len(const unsigned char *buf)
{
    int ch, ret;
    ch = buf[0];
    if ((ch <= 0x7F))
    {
        ret = 1;
    }
    else if ((ch & 0xF0) == 0xF0)
    {
        ret = 4;
    }
    else if ((ch & 0xE0) == 0xE0)
    {
        ret = 3;
    }
    else if ((ch & 0xC0) == 0xC0)
    {
        ret = 2;
    }
    else
    {
        ret = 5;
    }

    return ret;
}

zcc_general_namespace_end(charset);
zcc_namespace_end;
#pragma pack(pop)
#endif // __cplusplus

#endif // ZCC_LIB_INCLUDE_CHARSET___

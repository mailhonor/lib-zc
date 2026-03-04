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
struct ZCC_LIB_API detect_data
{
    const char *data2;
    int count2;
    const char *data3[16];
    int count3[16];
};
#pragma pack(pop)
ZCC_LIB_API extern bool var_debug_mode;
static const int var_name_max_size = 32;
ZCC_LIB_API extern detect_data *var_default_detect_data;

ZCC_LIB_API const char *correct_name(const char *charset);
inline const char *correct_name(const std::string &charset)
{
    return correct_name(charset.c_str());
}
// 使用自定义的转码引擎
ZCC_LIB_API typedef std::string (*convert_engine_t)(const char *from_charset, const char *src, int64_t src_len, const char *to_charset, int64_t *invalid_bytes);
ZCC_LIB_API std::string iconv_convert(const char *from_charset, const char *src, int64_t src_len, const char *to_charset, int64_t *invalid_bytes);
ZCC_LIB_API std::string uconv_convert(const char *from_charset, const char *src, int64_t src_len, const char *to_charset, int64_t *invalid_bytes);
ZCC_LIB_API void use_convert_engine(convert_engine_t engine);
ZCC_LIB_API void use_uconv();
//
ZCC_LIB_API std::string detect(detect_data *dd, const char **charset_list, const char *data, int64_t size);
ZCC_LIB_API std::string detect_cjk(const char *data, int64_t size);
inline std::string detect_cjk(const std::string &data)
{
    return detect_cjk(data.c_str(), (int64_t)data.size());
}
ZCC_LIB_API extern const char *var_chinese[];
ZCC_LIB_API extern const char *var_japanese[];
ZCC_LIB_API extern const char *var_korean[];
ZCC_LIB_API extern const char *var_cjk[];

ZCC_LIB_API extern std::string (*convert_engine)(const char *from_charset, const char *src, int64_t src_len, const char *to_charset, int64_t *invalid_bytes);
inline std::string convert(const char *from_charset, const char *data, int64_t size, const char *to_charset, int64_t *invalid_bytes = 0)
{
    return convert_engine(from_charset, data, size, to_charset, invalid_bytes);
}

ZCC_LIB_API std::string convert_to_utf8(const char *from_charset, const char *data, int64_t size);
inline std::string convert_to_utf8(const char *data, int64_t size)
{
    return convert_to_utf8(0, data, size);
}
inline std::string convert_to_utf8(const char *from_charset, const std::string &data)
{
    return convert_to_utf8(from_charset, data.c_str(), (int64_t)data.size());
}
inline std::string convert_to_utf8(const std::string &from_charset, const std::string &data)
{
    return convert_to_utf8(from_charset.c_str(), data.c_str(), (int64_t)data.size());
}
inline std::string convert_to_utf8(const std::string &data)
{
    return convert_to_utf8(0, data.c_str(), (int64_t)data.size());
}

//
ZCC_LIB_API int64_t utf8_tail_complete(const char *ps, int64_t length);
ZCC_LIB_API int64_t utf8_tail_complete(const std::string &ps);
ZCC_LIB_API int64_t utf8_tail_complete(const std::string &ps, int64_t length);
ZCC_LIB_API char *utf8_tail_complete_and_terminate(char *ps, int64_t length);
ZCC_LIB_API std::string &utf8_tail_complete_and_terminate(std::string &s);

//
ZCC_LIB_API std::string utf8_get_simple_digest(const char *s, int64_t len, int64_t need_width);
inline std::string utf8_get_simple_digest(const std::string &s, int64_t need_width)
{
    return utf8_get_simple_digest(s.c_str(), (int64_t)s.size(), need_width);
}

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

static inline int utf8_len(int ch)
{
    int ret;
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

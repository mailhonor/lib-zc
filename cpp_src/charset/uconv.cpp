/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-10-12
 * ================================
 */
#ifdef ZCC_USE_UCONV__
#include "zcc/zcc_charset.h"
#define uconv_convert_engine_debug(...) \
    if (zcc::charset::var_debug_mode)   \
    {                                   \
        zcc_debug_output(__VA_ARGS__);  \
    }

#include "zcc/zcc_charset_uconv_engine.hpp"

zcc_namespace_begin;
zcc_general_namespace_begin(charset);

std::string uconv_convert(const char *from_charset, const char *src, int64_t src_len, const char *to_charset, int64_t *invalid_bytes)
{
    return uconv_convert_engine(from_charset, src, src_len, to_charset, invalid_bytes);
}

void use_uconv()
{
    use_convert_engine(uconv_convert_engine);
}
zcc_general_namespace_end(charset);
zcc_namespace_end;
#else
#include "zcc/zcc_charset.h"
zcc_namespace_begin;
zcc_general_namespace_begin(charset);
std::string uconv_convert(const char *from_charset, const char *src, int64_t src_len, const char *to_charset, int64_t *invalid_bytes)
{
    return "";
}

void use_uconv()
{
    zcc_error("unsupported icu, please install libicu-dev and rebuild zcc");
}
zcc_general_namespace_end(charset);
zcc_namespace_end;
#endif
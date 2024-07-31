/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-10-12
 * ================================
 */

#include "./mime.h"

zcc_namespace_begin;

std::string mail_parser::charset_convert(const char *from_charset, const char *data, int64_t size)
{
    return charset::convert_to_utf8(from_charset, data, size);
}

zcc_namespace_end;

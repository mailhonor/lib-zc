/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-12-09
 * ================================
 */

#include "./mime.h"

void zmime_iconv(const char *from_charset, const char *data, int size, zbuf_t *result)
{
    zcharset_convert_to_utf8(from_charset, data, size, result);
}

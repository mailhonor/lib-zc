/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-10-12
 * ================================
 */

#include "zc.h"
#include <iconv.h>
#ifdef __linux__
#include <errno.h>
#endif // __linux__
#ifdef _WIN32
#endif // _WIN32

namespace zcc
{

const char *charset_detect(const char **charset_list, const char *data, int size, std::string &charset_result)
{
    char result[zvar_charset_name_max_size+1];
    charset_result.clear();
    if (!zcharset_detect(charset_list, data, size, result)) {
        return 0;
    }
    charset_result.append(result);
    return charset_result.c_str();
}

const char *charset_detect_cjk(const char *data, int size, std::string &charset_result)
{
    char result[zvar_charset_name_max_size+1];
    charset_result.clear();
    if (!zcharset_detect_cjk(data, size, result)) {
        return 0;
    }
    charset_result.append(result);
    return charset_result.c_str();
}

std::string charset_convert_to_utf8(const char *from_charset, const char *data, int size)
{
    std::string r;
    charset_convert_to_utf8(from_charset, data, size, r);
    return r;
}

#include "../../src/charset/iconv.c"

}


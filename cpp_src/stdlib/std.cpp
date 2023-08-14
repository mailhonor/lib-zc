/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-10-12
 * ================================
 */

#include "zc.h"

namespace zcc
{

/* string */

std::string &vsprintf_1024(std::string &str, const char *format, va_list ap)
{
    char buf[1024+1];
    ::zvsnprintf(buf, 1024, format, ap);
    str.append(buf);
    return str;
}

std::string &sprintf_1024(std::string &str, const char *format, ...)
{
    va_list ap;
    char buf[1024+1];

    va_start(ap, format);
    ::zvsnprintf(buf, 1024, format, ap);
    va_end(ap);
    str.append(buf);
    return str;
}

std::string &tolower(std::string &str)
{
    zstr_tolower((char *)(void *)str.c_str());
    return str;
}

std::string &toupper(std::string &str)
{
    zstr_toupper((char *)(void *)str.c_str());
    return str;
}

std::string &trim_right(std::string &str, const char *delims)
{
    if (zempty(delims)) {
        delims = "\r\n \t";
    }
    int olen = (int)(str.size()), len = olen;
    char *p = (char *)(void *)str.c_str() + len;
    for (;len>0;len--) {
        p--;
        if (!strchr(delims, *p)) {
            break;
        }
    }
    if (len != olen) {
        str.resize(len);
    }
    return str;
}

string &string::printf_1024(const char *format, ...)
{
    va_list ap;
    char buf[1024+1];

    va_start(ap, format);
    ::zvsnprintf(buf, 1024, format, ap);
    va_end(ap);
    append(buf);
    return *this;
}

/* vector */
std::vector<std::string> split(const char *s, const char *delims)
{
    std::vector<std::string> r;
    string stmp;
    zstrtok_t stok;
    if (delims == 0) {
        delims = " \t\r\n";
    }
    zstrtok_init(&stok, (char *)s);
    while (zstrtok(&stok, delims)) {
        stmp.clear().append(stok.str, stok.len);
        r.push_back(stmp);
    }
    return r;
}
/* map */

}


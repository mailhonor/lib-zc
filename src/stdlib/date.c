/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2017-07-24
 * ================================
 */

#ifdef _WIN32
#pragma GCC diagnostic ignored "-Wformat="
#endif // _WIN32

#include "zc.h"
#include <time.h>

char *zbuild_rfc1123_date_string(ssize_t t, char *buf)
{
    struct tm tmbuf;
    gmtime_r((time_t *)(&t), &tmbuf);
    strftime(buf, zvar_rfc1123_date_string_size, "%a, %d %b %Y %H:%M:%S GMT", &tmbuf);
    return buf;
}

char *zbuild_rfc822_date_string(ssize_t t, char *buf)
{
    struct tm tmbuf;
    localtime_r((time_t *)&t, &tmbuf);
    strftime(buf, zvar_rfc822_date_string_size, "%a, %d %b %y %T %z (%Z)", &tmbuf);
    return buf;
}

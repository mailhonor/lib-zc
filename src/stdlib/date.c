/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2017-07-24
 * ================================
 */

#ifdef _WIN64
#pragma GCC diagnostic ignored "-Wformat="
#endif // _WIN64

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
#ifdef _WIN64
    // 还需要考虑夏令时, timezone 代码
    strftime(buf, zvar_rfc822_date_string_size, "%a, %d %b %Y %H:%M:%S ", &tmbuf);
    char tb[10];
    int tz = (0 - _timezone)/36;
    if (tz < 0) {
        tb[0] = '-';
    }
    else
    {
        tb[0] = '+';
    }
    zsprintf(tb+1, "%04d", (tz>0)?tz:(0-tz));
    strcat(buf, tb);
#else // _WIN64
    strftime(buf, zvar_rfc822_date_string_size, "%a, %d %b %Y %H:%M:%S %z (%Z)", &tmbuf);
#endif // _WIN64
    return buf;
}

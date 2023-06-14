/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2017-07-24
 * ================================
 */

#include "zc.h"
#include <time.h>

char *zbuild_rfc1123_date_string(long long t, char *buf)
{
    struct tm tmbuf;
    gmtime_r((time_t *)(&t), &tmbuf);
    strftime(buf, zvar_rfc1123_date_string_size, "%a, %d %b %Y %H:%M:%S GMT", &tmbuf);
    return buf;
}

char *zbuild_rfc822_date_string(long long t, char *buf)
{
    struct tm tmbuf;
    localtime_r((time_t *)&t, &tmbuf);
    strftime(buf, zvar_rfc822_date_string_size, "%a, %d %b %y %T %z (%Z)", &tmbuf);
    return buf;
}

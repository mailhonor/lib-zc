/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2024-02-18
 * ================================
 */

#include "zc.h"

zcc_namespace_begin;

std::string build_rfc1123_date_string(ssize_t t)
{
    std::string r;
    char buf[128 + 1];
    zbuild_rfc1123_date_string(t, buf);
    r = buf;
    return r;
}

std::string build_rfc822_date_string(ssize_t t)
{
    std::string r;
    char buf[128 + 1];
    zbuild_rfc822_date_string(t, buf);
    r = buf;
    return r;
}

zcc_namespace_end;

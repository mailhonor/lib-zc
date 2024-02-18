/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2024-02-18
 * ================================
 */

#include "zc.h"

zcc_namespace_begin;

std::string build_unique_id()
{
    std::string r;
    char buf[zvar_unique_id_size + 1];
    zbuild_unique_id(buf);
    r = buf;
    return r;
}

zcc_namespace_end;

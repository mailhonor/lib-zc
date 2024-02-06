/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2024-01-30
 * ================================
 */

#include "zc.h"

#pragma pack(push, 4)
zcc_namespace_begin;

#define zcc_pop_client_info(fmt, args...) zinfo(fmt, ##args)
#define zcc_pop_client_error(fmt, args...) zerror(fmt, ##args)
#define zcc_pop_client_debug(fmt, args...) \
    if (debug_mode_)                        \
    zinfo(fmt, ##args)
#define zcc_pop_client_debug_read_line(s) \
    if (debug_mode_)                       \
    zinfo("pop 读: %s", s.c_str())
#define zcc_pop_client_debug_write_line(s) \
    if (debug_mode_)                        \
    zinfo("pop 写: %s", s.c_str())

zcc_namespace_end;
#pragma pack(pop)

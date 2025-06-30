/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2025-06-03
 * ================================
 */

#include "zcc/zcc_smtp.h"

#pragma pack(push, 4)
zcc_namespace_begin;

#define zcc_smtp_client_info(...) zcc_info(__VA_ARGS__)
#define zcc_smtp_client_error(...) zcc_error(__VA_ARGS__)
#define zcc_smtp_client_debug(...) \
    if (debug_mode_)               \
    zcc_info(__VA_ARGS__)
#define zcc_smtp_client_debug_read_line(s) \
    if (debug_mode_)                       \
    zcc_info("smtp 读: %s", s.c_str())
#define zcc_smtp_client_debug_write_line(s) \
    if (debug_mode_)                        \
    zcc_info("smtp 写: %s", s.c_str())

zcc_namespace_end;
#pragma pack(pop)

/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2024-01-30
 * ================================
 */

#include "zcc/zcc_pop.h"

#pragma pack(push, 4)
zcc_namespace_begin;

#define zcc_pop_client_info(...) zcc_info(__VA_ARGS__)
#define zcc_pop_client_error(...) zcc_error(__VA_ARGS__)
#define zcc_pop_client_debug(...) \
    if (debug_mode_)              \
    zcc_info(__VA_ARGS__)
zcc_namespace_end;
#pragma pack(pop)

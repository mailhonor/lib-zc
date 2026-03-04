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

#define zcc_pop_client_debug zcc_class_debug
#define zcc_pop_client_verbose zcc_class_verbose
#define zcc_pop_client_debug_protocol_read(s) \
    if (debug_protocol_mode_)                 \
    zcc_debug_output("pop 读: %s", s.c_str())
#define zcc_pop_client_debug_protocol_write(s) \
    if (debug_protocol_mode_)                  \
    zcc_debug_output("pop 写: %s", s.c_str())

zcc_namespace_end;
#pragma pack(pop)

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

#define zcc_smtp_client_debug zcc_class_debug
#define zcc_smtp_client_verbose(...) zcc_class_verbose
#define zcc_smtp_client_debug_protocol_read(s) \
    if (debug_protocol_mode_)                  \
    zcc_debug_output("smtp 读: %s", s.c_str())
#define zcc_smtp_client_debug_protocol_write(s) \
    if (debug_protocol_mode_)                   \
    zcc_debug_output("smtp 写: %s", s.c_str())

zcc_namespace_end;
#pragma pack(pop)

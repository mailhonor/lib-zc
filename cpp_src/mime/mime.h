/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2022-10-20
 * ================================
 */

#include "zcc/zcc_mime.h"
#include "zcc/zcc_charset.h"
#include "zcc/zcc_buffer.h"
#include "zcc/zcc_link.h"

#pragma pack(push, 4)
zcc_namespace_begin;

#define zcc_mime_info zcc_info
#define zcc_mime_error zcc_error
#define zcc_mime_debug(...) ((mime_debug_mode_) ? zcc_debug_output(__VA_ARGS__) : (void)0)

zcc_namespace_end;
#pragma pack(pop)

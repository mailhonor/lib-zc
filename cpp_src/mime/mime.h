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

#define zcc_mime_info(...) zcc_info(__VA_ARGS__)
#define zcc_mime_error(...) zcc_error(__VA_ARGS__)
#define zcc_mime_debug(...) \
    if (mime_debug_mode_)   \
    zcc_info(__VA_ARGS__)
#define zcc_mime_debug_read_line(s) \
    if (mime_debug_mode_)           \
    zcc_info("imap 读: %s", s.c_str())
#define zcc_mime_debug_write_line(s) \
    if (mime_debug_mode_)            \
    zcc_info("imap 写: %s", s.c_str())

zcc_namespace_end;
#pragma pack(pop)

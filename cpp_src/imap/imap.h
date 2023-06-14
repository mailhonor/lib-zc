/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2022-10-20
 * ================================
 */

#include "zc.h"

#pragma pack(push, 4)
zcc_namespace_begin;

#define zcc_imap_client_info(fmt, args...) zinfo(fmt, ##args)
#define zcc_imap_client_error(fmt, args...) zerror(fmt, ##args)
#define zcc_imap_client_debug(fmt, args...) \
    if (debug_mode_)                        \
    zinfo(fmt, ##args)
#define zcc_imap_client_debug_read_line(s) \
    if (debug_mode_)                       \
    zinfo("imap 读: %s", s.c_str())
#define zcc_imap_client_debug_write_line(s) \
    if (debug_mode_)                        \
    zinfo("imap 写: %s", s.c_str())

#define zcc_imap_client_read_token_vecotr_one_loop() \
    response_tokens.reset();                         \
    if (!read_response_tokens(response_tokens))      \
    {                                                \
        break;                                       \
    }                                                \
    if (response_tokens.token_vector_.size() < 2)    \
    {                                                \
        zcc_imap_client_info("ERROR want >2 tokens");  \
        need_close_connection_ = true;               \
        break;                                       \
    }

zcc_namespace_end;
#pragma pack(pop)

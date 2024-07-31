/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2022-10-20
 * ================================
 */

#include "zcc/zcc_imap.h"
#include "zcc/zcc_charset.h"

#pragma pack(push, 4)
zcc_namespace_begin;

#define zcc_imap_client_info(...) zcc_info( __VA_ARGS__)
#define zcc_imap_client_error(...) zcc_error(__VA_ARGS__)
#define zcc_imap_client_debug(...) \
    if (debug_mode_)                        \
    zcc_info(__VA_ARGS__)
#define zcc_imap_client_debug_read_line(s) \
    if (debug_mode_)                       \
    zcc_info("imap 读: %s", s.c_str())
#define zcc_imap_client_debug_write_line(s) \
    if (debug_mode_)                        \
    zcc_info("imap 写: %s", s.c_str())

#define zcc_imap_client_read_token_vecotr_one_loop()  \
    response_tokens.reset();                          \
    if (read_response_tokens(response_tokens) < 0)    \
    {                                                 \
        break;                                        \
    }                                                 \
    if (response_tokens.token_vector_.size() < 2)     \
    {                                                 \
        zcc_imap_client_info("ERROR want >2 tokens"); \
        need_close_connection_ = true;                \
        break;                                        \
    }

void _imap_client_parse_mail_flags(imap_client::mail_flags &flags, const imap_client::response_tokens &response_tokens, int offset);

zcc_namespace_end;
#pragma pack(pop)

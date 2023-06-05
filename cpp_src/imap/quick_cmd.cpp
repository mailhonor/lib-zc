/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2023-06-05
 * ================================
 */

#include "./imap.h"

zcc_namespace_begin;

bool imap_client::do_quick_cmd(const std::string &cmd, bool check_result_is_ok)
{
    if (need_close_connection_)
    {
        return false;
    }
    if (cmd.empty())
    {
        return false;
    }
    response_tokens response_tokens;
    int first_ch = cmd[0];
    std::string linebuf;
    fp_append(cmd).fp_append("\r\n");
    zcc_imap_client_debug_write_line(linebuf);

    while (1)
    {
        zcc_imap_client_read_token_vecotr_one_loop();
        if (response_tokens.token_vector_[0] == "*")
        {
            continue;
        }

        if (!parse_imap_result(first_ch, response_tokens))
        {
            return false;
        }
        if (check_result_is_ok)
        {
            if (!result_is_ok())
            {
                return false;
            }
            return true;
        }
    }
    return true;
}

zcc_namespace_end;

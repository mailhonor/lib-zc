/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2023-06-05
 * ================================
 */

#include "./imap.h"

zcc_namespace_begin;

int imap_client::do_quick_cmd(const std::string &cmd)
{
    if (need_close_connection_)
    {
        return -1;
    }
    if (cmd.empty())
    {
        return -1;
    }
    int first_ch = cmd[0];
    fp_append(cmd).fp_append("\r\n");
    zcc_imap_client_debug_write_line(cmd);

    if (simple_line_mode_)
    {
        std::string linebuf;
        while (1)
        {
            linebuf.clear();
            if (fp_gets(linebuf, 10240) < 0)
            {
                return -1;
            }
            zcc_imap_client_debug_read_line(linebuf);
            if (linebuf[0] == '*')
            {
                continue;
            }
            else
            {
                return parse_imap_result(first_ch, linebuf.c_str());
            }
        }
    }
    else
    {
        response_tokens response_tokens;
        while (1)
        {
            zcc_imap_client_read_token_vecotr_one_loop();
            if (response_tokens.token_vector_[0] == "*")
            {
                continue;
            }

            return parse_imap_result(first_ch, response_tokens);
        }
    }

    return -1;
}

zcc_namespace_end;

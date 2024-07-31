/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2023-06-05
 * ================================
 */

#include "./imap.h"

zcc_namespace_begin;

int imap_client::do_auth(const char *user, const char *password)
{
    if (need_close_connection_)
    {
        return -1;
    }
    int r;
    std::string linebuf;
    response_tokens response_tokens;

    linebuf.clear();
    linebuf.append("L login ").append(escape_string(user));
    linebuf.append(" ").append(escape_string(password));
    fp_append(linebuf).fp_append("\r\n");
    zcc_imap_client_debug_write_line(linebuf);
    while (1)
    {
        zcc_imap_client_read_token_vecotr_one_loop();
        if (response_tokens.token_vector_[0] == "*")
        {
            continue;
        }

        if ((r = parse_imap_result('L', response_tokens)) < 1)
        {
            if (r == 0)
            {
                zcc_imap_client_info("imap 登录(LOGIN)失败");
            }
            return r;
        }
        if (!(response_tokens.first_line_.compare(0, 17, "L OK [CAPABILITY ")))
        {
            capability_ = response_tokens.first_line_.substr(16);
            capability_clear_flag_ = false;
            auth_capability_ = true;
        }
        break;
    }

    return 1;
}

zcc_namespace_end;

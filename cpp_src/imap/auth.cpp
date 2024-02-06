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
    bool iret;
    std::string linebuf;

    linebuf.clear();
    linebuf.append("L login ").append(escape_string(user));
    linebuf.append(" ").append(escape_string(password));
    fp_append(linebuf).fp_append("\r\n");
    zcc_imap_client_debug_write_line(linebuf);

    if (fp_gets(linebuf, 10240) < 0)
    {
        return -1;
    }
    trim_line_end_rn(linebuf);
    zcc_imap_client_debug_read_line(linebuf);

    if ((r = parse_imap_result('L', linebuf)) < 1)
    {
        if (r == 0)
        {
            zcc_imap_client_info("imap 登录(LOGIN)失败");
        }
        return r;
    }

    if (!linebuf.compare(0, 17, "L OK [CAPABILITY "))
    {
        capability_ = linebuf.substr(16);
        capability_clear_flag_ = false;
        auth_capability_ = true;
    }

    return 1;
}

zcc_namespace_end;

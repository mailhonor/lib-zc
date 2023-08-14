/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2023-06-05
 * ================================
 */

#include "./imap.h"

zcc_namespace_begin;

void imap_client::set_user_password(const char *user, const char *password)
{
    mail_ = user;
    pass_ = password;
}

bool imap_client::auth()
{
    if (need_close_connection_)
    {
        return false;
    }
    bool iret;
    std::string linebuf;

    linebuf.clear();
    linebuf.append("L login ").append(escape_string(mail_));
    linebuf.append(" ").append(escape_string(pass_));
    fp_append(linebuf).fp_append("\r\n");
    zcc_imap_client_debug_write_line(linebuf);

    if (fp_gets(linebuf, 10240) < 0)
    {
        return false;
    }
    trim_rn(linebuf);
    zcc_imap_client_debug_read_line(linebuf);

    if (!parse_imap_result('L', linebuf)) {
        return false;
    }

    password_error_ = false;
    if (ok_no_bad_ == result_onb::bad)
    {
        password_error_ = true;
        zcc_imap_client_info("imap 登录(LOGIN)失败");
    }

    if (!linebuf.compare(0, 17, "L OK [CAPABILITY "))
    {
        capability_ = linebuf.substr(16);
        capability_clear_flag_ = false;
        auth_capability_ = true;
    }
    
    return true;
}

zcc_namespace_end;

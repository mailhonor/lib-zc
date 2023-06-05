/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2023-06-05
 * ================================
 */

#include "./imap.h"

zcc_namespace_begin;

bool imap_client::cmd_capability(bool force)
{
    if (need_close_connection_)
    {
        return false;
    }
    if (!force)
    {
        if (auth_capability_)
        {
            ok_no_bad_ = result_onb::ok;
            return true;
        }
    }
    std::string linebuf;
    linebuf = "C capability";
    fp_append(linebuf).fp_append("\r\n");
    zcc_imap_client_debug_write_line(linebuf);

    while (1)
    {
        linebuf.clear();
        if (fp_gets(linebuf, 10240) < 0)
        {
            return false;
        }
        zcc_imap_client_debug_read_line(linebuf);
        if (linebuf[0] == '*')
        {
            if (ZSTR_EQ(linebuf.c_str(), "* CAPABILITY "))
            {
                capability_ = linebuf.c_str() + 13;
                capability_clear_flag_ = false;
            }
        }
        else
        {
            if (!parse_imap_result('C', linebuf.c_str()))
            {
                return false;
            }
            return true;
        }
    }
    return true;
}

bool imap_client::get_capability(const char *key)
{
    if (!cmd_capability()){
        return false;
    }
    if (!capability_clear_flag_) {
        capability_clear_flag_ = true;
        size_t len = capability_.size();
        for (size_t i = 0; i < len; i++) {
            int ch = capability_[i];
            if (isalpha(ch)) {
                ch = ztolower(ch);
            } else if (!isdigit(ch)) {
                ch = ' ';
            }
            capability_[i] = ch;
        }
    }
    char keybuf[64 + 1];
    snprintf(keybuf, 64, "%s ", key);
    zstr_tolower(keybuf);
    if (!strstr(capability_.c_str(), keybuf)) {
        return false;
    }
    return true;
}

bool imap_client::get_capability_cached(const char *key, char *cache)
{
    if (!*cache) {
        *cache = (get_capability(key) ? '1' : '2');
    }
    return ((*cache) == '1');
}

zcc_namespace_end;

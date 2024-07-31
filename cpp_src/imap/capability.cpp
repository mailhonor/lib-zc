/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2023-06-05
 * ================================
 */

#include "./imap.h"

zcc_namespace_begin;

int imap_client::cmd_capability(bool force)
{
    if (need_close_connection_)
    {
        return -1;
    }
    if (!force)
    {
        if (auth_capability_)
        {
            return 1;
        }
    }
    std::string linebuf;
    response_tokens response_tokens;

    linebuf = "C capability";
    fp_append(linebuf).fp_append("\r\n");
    zcc_imap_client_debug_write_line(linebuf);
    while (1)
    {
        zcc_imap_client_read_token_vecotr_one_loop();
        if (response_tokens.token_vector_[0] == "*")
        {
            if (!response_tokens.first_line_.compare(0, 13, "* CAPABILITY "))
            {
                capability_ =  response_tokens.first_line_.c_str() + 12;
                capability_clear_flag_ = false;
            }
        }
        else
        {
            return parse_imap_result('C', response_tokens);
        }
    }
    return 1;
}

int imap_client::get_capability(const char *key)
{
    int r;
    if ((r = cmd_capability()) < 1)
    {
        return r;
    }
    if (!capability_clear_flag_)
    {
        capability_clear_flag_ = true;
        uint64_t len = capability_.size();
        for (uint64_t i = 0; i < len; i++)
        {
            int ch = capability_[i];
            if (isalpha(ch))
            {
                ch = tolower(ch);
            }
            else if (!isdigit(ch))
            {
                ch = ' ';
            }
            capability_[i] = ch;
        }
    }
    char keybuf[64 + 1];
    std::snprintf(keybuf, 64, " %s ", key);
    if (!strstr(capability_.c_str(), keybuf))
    {
        return 0;
    }
    return 1;
}

int imap_client::get_capability_cached(const char *key, char *cache)
{
    if (!*cache)
    {
        int r = get_capability(key);
        if (r < 0)
        {
            return -1;
        }
        *cache = (r ? '1' : '2');
    }
    return ((*cache) == '1');
}

zcc_namespace_end;

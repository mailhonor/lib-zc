/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2023-06-05
 * ================================
 */

#include "./imap.h"

zcc_namespace_begin;

int imap_client::check_new_message_by_noop(const std::string &mbox)
{
    if (check_is_need_close())
    {
        return -1;
    }
    cmd_select(mbox);
    if (check_is_need_close())
    {
        return -1;
    }

    int r = 0;
    std::string linebuf, name;
    response_tokens response_tokens;

    linebuf.clear();
    linebuf.append("N noop");
    fp_append(linebuf).fp_append("\r\n");
    zcc_imap_client_debug_write_line(linebuf);

    while (1)
    {
        zcc_imap_client_read_token_vecotr_one_loop();
        if (response_tokens.token_vector_[0] == "*")
        {
            r = 1;
        }
        else
        {
            if (parse_imap_result('L', response_tokens) < 0)
            {
                return -1;
            }
            break;
        }
    }
    return r;
}

int imap_client::idle_beign(const std::string &mbox)
{
    if (check_is_need_close())
    {
        return -1;
    }
    cmd_select(mbox);
    if (check_is_need_close())
    {
        return -1;
    }

    int r = 0;
    std::string linebuf, name;
    response_tokens response_tokens;

    linebuf.clear();
    linebuf.append("I idle");
    fp_append(linebuf).fp_append("\r\n");
    zcc_imap_client_debug_write_line(linebuf);

    while (1)
    {
        zcc_imap_client_read_token_vecotr_one_loop();
        if (response_tokens.token_vector_[0] == "+")
        {
            r = 1;
            break;
        }
        else
        {
            if (parse_imap_result('I', response_tokens) < 0)
            {
                return -1;
            }
            break;
        }
        break;
    }
    return r;
}

bool imap_client::idle_check_new_message(int wait_second)
{
    bool ret = false;
    std::string line;
    if (fp_->timed_read_wait(wait_second) < 1)
    {
        return ret;
    }
    while (1)
    {
        if (fp_gets(line, 10240) < 1)
        {
            ret = true;
            break;
        }
        if (line.empty())
        {
            ret = true;
            break;
        }
        if (line[0] == '*')
        {
            zcc::tolower(line);
            if (strncmp(line.c_str(), "* ok ", 4))
            {
                ret = true;
            }
        }
        if (fp_->timed_read_wait(0) < 1)
        {
            break;
        }
    }
    return ret;
}

int imap_client::idle_end()
{
    if (check_is_need_close())
    {
        return -1;
    }

    int r = 0;
    std::string linebuf, name;
    response_tokens response_tokens;

    linebuf.clear();
    linebuf.append("DONE");
    fp_append(linebuf).fp_append("\r\n");
    zcc_imap_client_debug_write_line(linebuf);

    while (1)
    {
        zcc_imap_client_read_token_vecotr_one_loop();
        if (response_tokens.token_vector_[0] == "*")
        {
            r = 1;
        }
        else
        {
            if (parse_imap_result('I', response_tokens) < 0)
            {
                return -1;
            }
            break;
        }
    }
    return r;
}

zcc_namespace_end;

/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2023-06-05
 * ================================
 */

#include "./imap.h"

zcc_namespace_begin;

int imap_client::check_new_message_by_noop()
{
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
    zcc_imap_client_debug_protocol_write(linebuf);

    while (1)
    {
        zcc_imap_client_read_token_vecotr_one_loop();
        if (response_tokens.token_vector_[0] == "*")
        {
            if ((r != 1) && (response_tokens.token_vector_.size() > 2))
            {
                auto line = response_tokens.token_vector_[2];
                zcc::tolower(line);
                if (line.find("exists") != std::string::npos)
                {
                    r = 1;
                }
            }
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

int imap_client::check_new_message_by_noop(const std::string &folder_name)
{
    if (check_is_need_close())
    {
        return -1;
    }
    if (cmd_select(folder_name) < 0)
    {
        return -1;
    }
    return check_new_message_by_noop();
}

int imap_client::idle_begin()
{
    if (check_is_need_close())
    {
        return -1;
    }
    unsetMaybeHaveNewMessage();

    int r = 0;
    std::string linebuf, name;
    response_tokens response_tokens;

    linebuf.clear();
    linebuf.append("I idle");
    fp_append(linebuf).fp_append("\r\n");
    zcc_imap_client_debug_protocol_write(linebuf);

    while (1)
    {
        zcc_imap_client_read_token_vecotr_one_loop();
        if (response_tokens.token_vector_[0] == "*")
        {
            continue;
        }
        else if (response_tokens.token_vector_[0] == "+")
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
    if (fp_->flush() < 0)
    {
        return -1;
    }
    return r;
}

int imap_client::idle_check_new_message(int wait_second)
{
    if (maybeHaveNewMessage())
    {
        unsetMaybeHaveNewMessage();
        return 1;
    }
    int r = 0;
    int ret;
    std::string line;
    if ((ret = fp_->timed_read_wait(wait_second)) < 0)
    {
        return -1;
    }
    if (ret < 1)
    {
        return 0;
    }
    while (1)
    {
        if (fp_gets(line, 10240) < 1)
        {
            r = -1;
            break;
        }
        if (line.empty())
        {
            r = -1;
            break;
        }
        if (line[0] == '*')
        {
            zcc::tolower(line);
            if (line.find(" exists") != std::string::npos)
            {
                r = 1;
            }
        }
        if ((ret = fp_->timed_read_wait(0)) < 0)
        {
            r = -1;
            break;
        }
        if (ret < 1)
        {
            break;
        }
    }
    if (need_close_connection_)
    {
        return -1;
    }
    return r;
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
    zcc_imap_client_debug_protocol_write(linebuf);

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

int imap_client::check_new_message_by_idle(int wait_second)
{
    int expired = zcc::second() + wait_second + 1;
    if (idle_begin() < 0)
    {
        return -1;
    }
    int r = 0;
    while (1)
    {
        int wait = expired - zcc::second();
        if (wait < 1)
        {
            break;
        }
        r = idle_check_new_message(wait + 1);
        if (r != 0)
        {
            break;
        }
        if (zcc::second() > expired)
        {
            break;
        }
    }
    idle_end();
    return r;
}

int imap_client::run_idle_listener(std::function<void()> callback)
{
    if (check_is_need_close())
    {
        return -1;
    }
    unsetMaybeHaveNewMessage();

    int timeout = get_timeout();
    set_timeout(1200);
    int r = 0;
    std::string linebuf, name;
    response_tokens response_tokens;

    linebuf.clear();
    linebuf.append("I idle");
    fp_append(linebuf).fp_append("\r\n");
    zcc_imap_client_debug_protocol_write(linebuf);

    while (1)
    {
        unsetMaybeHaveNewMessage();
        zcc_imap_client_read_token_vecotr_one_loop();
        if (response_tokens.token_vector_[0] == "*")
        {
            if (maybeHaveNewMessage())
            {
                if (callback)
                {
                    callback();
                }
            }
            continue;
        }
        else if (response_tokens.token_vector_[0] == "+")
        {
            continue;
        }
        else
        {
            if (parse_imap_result('I', response_tokens) < 0)
            {
                r = -1;
                goto over;
            }
            r = 0;
            goto over;
        }
    }
    //

over:
    set_timeout(timeout);
    return r;
}

zcc_namespace_end;

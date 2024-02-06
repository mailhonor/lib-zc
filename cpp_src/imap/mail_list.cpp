/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2023-06-05
 * ================================
 */

#include "./imap.h"

zcc_namespace_begin;

int imap_client::_cmd_search_flag(std::vector<int> &uid_vector, const char *flag_token)
{
    if (need_close_connection_)
    {
        return -1;
    }
    int r = -1;
    std::string linebuf, intbuf;

    linebuf.clear();
    linebuf.append("S UID SEARCH ").append(flag_token);
    fp_append(linebuf).fp_append("\r\n");
    zcc_imap_client_debug_write_line(linebuf);

    while (1)
    {
        linebuf.clear();
        int len = fp_gets(linebuf, 1024 * 1024 * 100);
        if (len > 1024)
        {
            zcc_imap_client_debug_read_line(linebuf);
        }
        else
        {
            if (debug_mode_)
            {
                intbuf = linebuf.substr(0, 1024);
                intbuf.append(" ...");
                zcc_imap_client_debug_read_line(intbuf);
            }
        }
        if (len < 6)
        {
            need_close_connection_ = true;
            goto over;
        }
        if ((linebuf[0] == '*') && (linebuf[1] == ' '))
        {
            if (uid_vector.size() > 1024 * 1024 * 10)
            {
                need_close_connection_ = true;
                zcc_imap_client_error("imap  返回结果太多 > %zd", uid_vector.size());
                goto over;
            }
            if (linebuf[len - 1] != '\n')
            {
                need_close_connection_ = true;
                if (len > 1024 * 1024 * 100 - 1)
                {
                    zcc_imap_client_error("imap 搜索结果返回太长 > %d", len);
                }
                else
                {
                    zcc_imap_client_error("imap 返回结果没有换行符 > %d", len);
                }
                goto over;
            }
            trim_line_end_rn(linebuf);
            const char *ps = linebuf.c_str() + 9, *end = ps + linebuf.size();
            while (ps < end)
            {
                intbuf.clear();
                const char *p = strchr(ps, ' ');
                if (p)
                {
                    intbuf.append(ps, p - ps);
                    ps = p + 1;
                }
                else
                {
                    intbuf.append(ps);
                    ps = end;
                }
                if (intbuf.size())
                {
                    int uid = atoi(intbuf.c_str());
                    uid_vector.push_back(uid);
                }
            }
        }
        else /* if (linebuf == "S ") */
        {
            if ((r = parse_imap_result(linebuf.c_str() + 2) < 1))
            {
                break;
            }
            goto over;
        }
    }

over:
    if (debug_mode_)
    {
        int count = 0;
        for (auto it = uid_vector.begin(); it != uid_vector.end(); it++)
        {
            if ((!verbose_mode_) && (count++ > 1))
            {
                zcc_imap_client_debug("搜索结果: %d (后面忽略; 显示全部结果, 用参数 --verbose)", *it);
                break;
            }
            zcc_imap_client_debug("搜索结果: %d", *it);
        }
    }
    return r;
}

int imap_client::cmd_search_all(std::vector<int> &uid_vector)
{
    return _cmd_search_flag(uid_vector, "ALL");
}

int imap_client::cmd_search_unseen(std::vector<int> &uid_vector)
{
    return _cmd_search_flag(uid_vector, "UNSEEN");
}

int imap_client::cmd_search_answered(std::vector<int> &uid_vector)
{
    return _cmd_search_flag(uid_vector, "ANSWERED");
}

int imap_client::cmd_search_deleted(std::vector<int> &uid_vector)
{
    return _cmd_search_flag(uid_vector, "DELETED");
}

int imap_client::cmd_search_draft(std::vector<int> &uid_vector)
{
    return _cmd_search_flag(uid_vector, "DRAFT");
}

int imap_client::cmd_search_flagged(std::vector<int> &uid_vector)
{
    return _cmd_search_flag(uid_vector, "FLAGGED");
}

int imap_client::get_mail_list(mail_list_result &mail_list)
{
    // * 1 FETCH (UID 1405 FLAGS (\Seen))
    // * 2 FETCH (UID 1407 FLAGS (\Flagged \Seen))
    // * 3 FETCH (UID 1410 FLAGS (\Seen))

    if (need_close_connection_)
    {
        return -1;
    }
    int r = -1;
    std::string linebuf, name;
    response_tokens response_tokens;
    auto &token_vector = response_tokens.token_vector_;

    linebuf.clear();
    linebuf.append("F FETCH 1:* (UID FLAGS)");
    fp_.append(linebuf).append("\r\n");
    zcc_imap_client_debug_write_line(linebuf);

    tmp_verbose_mode_ = true;
    tmp_verbose_line_ = 0;
    while (1)
    {
        zcc_imap_client_read_token_vecotr_one_loop();
        if (token_vector[0] == "*")
        {
            if ((token_vector.size() < 6) || (token_vector[3] != "(UID"))
            {
                need_close_connection_ = true;
                break;
            }

            mail_flags flags;
            _imap_client_parse_mail_flags(flags, response_tokens, 6);
            int uid = atoi(token_vector[4].c_str());
            mail_list[uid] = flags;
        }
        else
        {
            if ((r = parse_imap_result('F', response_tokens)) < 1)
            {
                break;
            }
            goto debug;
        }
    }

    tmp_verbose_mode_ = false;
    tmp_verbose_line_ = 0;
    return r;

debug:
    if (debug_mode_)
    {
        int count = 0;
        for (auto it = mail_list.begin(); it != mail_list.end(); it++)
        {
            if ((!verbose_mode_) && (count++ > 1))
            {
                zcc_imap_client_debug("邮件列表结果: UID: % 6d, a: %d, s: %d, d: %d, f: %d, D: %d, R: %d (后面忽略; 显示全部结果, 用参数 --verbose)", it->first, it->second.answered_, it->second.seen_, it->second.draft_, it->second.flagged_, it->second.deleted_, it->second.recent_);
                break;
            }
            zcc_imap_client_debug("邮件列表结果: UID: % 6d, a: %d, s: %d, d: %d, f: %d, D: %d, R: %d", it->first, it->second.answered_, it->second.seen_, it->second.draft_, it->second.flagged_, it->second.deleted_, it->second.recent_);
        }
    }
    return r;
}

zcc_namespace_end;

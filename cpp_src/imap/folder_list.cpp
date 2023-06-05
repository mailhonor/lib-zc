/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2023-06-05
 * ================================
 */

#include "./imap.h"

zcc_namespace_begin;

// folder_result
imap_client::folder_result::folder_result()
{
    status_ = 0;
    reset();
}

imap_client::folder_result::~folder_result()
{
    if (status_)
    {
        delete status_;
    }
}

void imap_client::folder_result::reset()
{
    name_ = "";
    noinferiors_ = false;
    noselect_ = false;
    subscribed_ = false;
    drafts_ = false;
    junk_ = false;
    trash_ = false;
    sent_ = false;
    if (status_)
    {
        status_->reset();
    }
}

void imap_client::folder_result::debug_show()
{
    std::string tmpbuf;
    zcc::sprintf_1024(tmpbuf, "文件夹: %s, 属性: (Noinferiors: %d, Noselect: %d, Subscribed: %d; Drafts: %d, Junk: %d, Trash: %d, Sent: %d)", name_.c_str(), noinferiors_, noselect_, subscribed_, drafts_, junk_, trash_, sent_);
    if (status_)
    {
        zcc::sprintf_1024(tmpbuf, ", STATUS(RECENT: %d, UIDNEXT: %d, UIDVALIDITY: %d, UNSEEN: %d)", status_->recent_, status_->uidnext_, status_->uidvalidity_, status_->unseen_);
    }
    zinfo("%s", tmpbuf.c_str());
}

const char *imap_client::folder_result::get_special_use()
{
    const char *r = "";
    if (sent_)
    {
        r = "sent";
    }
    else if (drafts_)
    {
        r = "drafts";
    }
    else if (junk_)
    {
        r = "junk";
    }
    else if (trash_)
    {
        r = "trash";
    }
    return r;
}

static void _parse_folder_list(imap_client::folder_result &folder, const imap_client::response_tokens &response_tokens)
{
    // * LIST (\HasNoChildren \UnMarked) "/" vvv
    auto &token_vector = response_tokens.token_vector_;
    folder.reset();
    folder.name_ = token_vector.back();

    bool stop = false;
    for (auto it = token_vector.begin() + 1; (!stop) && (it != token_vector.end()); it++)
    {
        std::string tmp = *it;
        if (tmp.empty())
        {
            continue;
        }
        if (tmp.back() == ')')
        {
            tmp.pop_back();
        }
        zcc::tolower(tmp);
        const char *s = tmp.c_str();
        if (s[0] == '(')
        {
            s++;
        }
        if (s[0] == '\\')
        {
            s++;
        }
        if (ZSTR_EQ(s, "noinferiors"))
        {
            folder.noinferiors_ = true;
        }
        else if (ZSTR_EQ(s, "noselect"))
        {
            folder.noselect_ = true;
        }
        else if (ZSTR_EQ(s, "junk"))
        {
            folder.junk_ = true;
        }
        else if (ZSTR_EQ(s, "trash"))
        {
            folder.trash_ = true;
        }
        else if (ZSTR_EQ(s, "sent"))
        {
            folder.sent_ = true;
        }
        else if (ZSTR_EQ(s, "drafts"))
        {
            folder.drafts_ = true;
        }
    }
}

bool imap_client::_cmd_list(folder_list_result &folder_list, bool list_or_lsub)
{
    // * LIST (\HasNoChildren) "/" "{123}"
    // * LIST (\HasNoChildren) "/" abc'
    // * LIST (\HasNoChildren) "/" 'abc'
    // * LIST (\HasNoChildren) "/" "abc\\r\\ndef"
    // * LIST (\HasChildren \UnMarked) "/" rubbishs
    // * LIST (\HasNoChildren \UnMarked) "/" rubbishs/xxx2
    // * LIST (\HasNoChildren \UnMarked) "/" Sent
    // * LIST (\HasNoChildren \UnMarked) "/" Drafts
    // * LIST (\HasNoChildren) "/" INBOX

    if (need_close_connection_)
    {
        return false;
    }
    const char *cmd = list_or_lsub ? "LIST" : "LSUB";
    std::string linebuf, name;
    response_tokens response_tokens;

    linebuf.clear();
    linebuf.append("L ").append(cmd).append(" \"\" *");
    fp_append(linebuf).fp_append("\r\n");
    zcc_imap_client_debug_write_line(linebuf);

    while (1)
    {
        zcc_imap_client_read_token_vecotr_one_loop();
        if (response_tokens.token_vector_[0] == "*")
        {
            folder_result folder;
            _parse_folder_list(folder, response_tokens);
            folder_list[folder.name_] = folder;
            zcc_imap_client_debug("文件夹: %s", folder.name_.c_str());
        }
        else
        {
            return parse_imap_result('L', response_tokens);
        }
    }
    return false;
}

bool imap_client::cmd_list(folder_list_result &folder_list)
{
    return _cmd_list(folder_list, true);
}

bool imap_client::cmd_lsub(folder_list_result &folder_list)
{
    return _cmd_list(folder_list, false);
}

bool imap_client::get_all_folder_info(folder_list_result &folder_list)
{
    if (need_close_connection_)
    {
        return false;
    }
    folder_list_result folder_lsub;

    //文件夹列表 (LIST)
    if (!cmd_list(folder_list))
    {
        return false;
    }
    if (ok_no_bad_ != result_onb::ok) {
        return true;
    }

    // 文件夹列表 (LSUB)
    if (!cmd_lsub(folder_lsub))
    {
        return false;
    }
    if (ok_no_bad_ != result_onb::ok) {
        return true;
    }
    for (auto it = folder_lsub.begin(); it != folder_lsub.end(); it++)
    {
        auto list_it = folder_list.find(it->first);
        if (list_it != folder_list.end())
        {
            list_it->second.subscribed_ = true;
        }
    }

    // 文件夹属性 (STATUS)
    for (auto it = folder_list.begin(); it != folder_list.end(); it++)
    {
        status_result *st = new status_result();
        if (cmd_status(*st, it->first.c_str()))
        {
            if (ok_no_bad_ == result_onb::ok)
            {
                it->second.status_ = st;
                continue;
            }
            delete st;
            continue;
        }
        delete st;
        return false;
    }
    ok_no_bad_ = result_onb::ok;
    return true;
}

zcc_namespace_end;

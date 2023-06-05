/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2023-06-05
 * ================================
 */

#include "./imap.h"

zcc_namespace_begin;

// select_result
imap_client::select_result::select_result()
{
    reset();
}

imap_client::select_result::~select_result()
{
}

void imap_client::select_result::reset()
{
    flags_ = "";
    exists_ = -1;
    recent_ = -1;
    uidvalidity_ = -1;
    uidnext_ = -1;
    readonly_ = false;
}

// status_result
imap_client::status_result::status_result()
{
    reset();
}

imap_client::status_result::~status_result()
{
}

void imap_client::status_result::reset()
{
    messages_ = -1;
    recent_ = -1;
    uidnext_ = -1;
    uidvalidity_ = -1;
    unseen_ = -1;
}


// FIXME, 有的文件夹不能 SELECT
bool imap_client::cmd_select(select_result &ser, const char *folder_name_imap)
{
    // 1016-1668607103 select inbox
    // * OK [CLOSED] Previous mailbox closed.
    // * FLAGS (\Answered \Flagged \Deleted \Seen \Draft $Forwarded)
    // * OK [PERMANENTFLAGS (\Answered \Flagged \Deleted \Seen \Draft $Forwarded \*)] Flags permitted.
    // * 43 EXISTS
    // * 0 RECENT
    // * OK [UIDVALIDITY 1663559062] UIDs valid
    // * OK [UIDNEXT 1491] Predicted next UID
    // * OK [HIGHESTMODSEQ 2154] Highest
    // 1016-1668607103 OK [READ-WRITE] Select completed (0.001 + 0.000 secs).
    last_selected_ = "";
    if (need_close_connection_)
    {
        return false;
    }
    ser.reset();
    std::string linebuf;
    response_tokens response_tokens;

    linebuf.clear();
    linebuf.append("S SELECT ").append(escape_string(folder_name_imap));
    fp_append(linebuf).fp_append("\r\n");
    zcc_imap_client_debug_write_line(linebuf);

    while (1)
    {
        zcc_imap_client_read_token_vecotr_one_loop();
        auto &token_vector = response_tokens.token_vector_;
        if (token_vector[0] == "*")
        {
            if (token_vector.size() < 3)
            {
                continue;
            }
            std::string &tmp = token_vector[2];
            if (tmp == "EXISTS")
            {
                ser.exists_ = atoi(token_vector[1].c_str());
                zcc_imap_client_debug("EXISTS: %d", ser.exists_);
            }
            else if (tmp == "RECENT")
            {
                ser.recent_ = atoi(token_vector[1].c_str());
                zcc_imap_client_debug("RECETN: %d", ser.recent_);
            }
            else if (tmp == "[UIDVALIDITY")
            {
                if (token_vector.size() > 3)
                {
                    ser.uidvalidity_ = atoi(token_vector[3].c_str());
                    zcc_imap_client_debug("UIDVALIDITY: %d", ser.uidvalidity_);
                }
            }
            else if (tmp == "[UIDNEXT")
            {
                if (token_vector.size() > 3)
                {
                    ser.uidnext_ = atoi(token_vector[3].c_str());
                    zcc_imap_client_debug("UIDNEXT: %d", ser.uidnext_);
                }
            }
        }
        else
        {
            if (!parse_imap_result('S', response_tokens))
            {
                return false;
            }
            if (ok_no_bad_ == result_onb::ok)
            {
                last_selected_ = folder_name_imap;
            }
            return true;
        }
    }

    return false;
}

bool imap_client::cmd_select(const char *folder_name_imap)
{
    select_result ser;
    if (last_selected_ == folder_name_imap)
    {
        return true;
    }
    return cmd_select(ser, folder_name_imap);
}

bool imap_client::cmd_select_forced(const char *folder_name_imap)
{
    select_result ser;
    last_selected_ = "";
    return cmd_select(ser, folder_name_imap);
}

bool imap_client::cmd_status(status_result &ser, const char *folder_name_imap)
{
    // S status abc (MESSAGES RECENT UIDNEXT UIDVALIDITY UNSEEN)
    // * STATUS abc (MESSAGES 0 RECENT 0 UIDNEXT 1 UIDVALIDITY 1663559079 UNSEEN 0)
    // S OK Status completed (0.001 + 0.000 secs).
    if (need_close_connection_)
    {
        return false;
    }
    ser.reset();
    std::string linebuf;
    response_tokens response_tokens;

    linebuf.clear();
    linebuf.append("S STATUS ").append(escape_string(folder_name_imap));
    linebuf.append(" (MESSAGES RECENT UIDNEXT UIDVALIDITY UNSEEN)");
    fp_append(linebuf).fp_append("\r\n");
    zcc_imap_client_debug_write_line(linebuf);

    while (1)
    {
        zcc_imap_client_read_token_vecotr_one_loop();
        auto &token_vector = response_tokens.token_vector_;
        if (token_vector[0] == "*")
        {
            if (token_vector.size() < 4)
            {
                continue;
            }
            for (auto it = token_vector.begin() + 3; it != token_vector.end(); it++)
            {
                const char *k = it->c_str();
                if (k[0] == '(')
                {
                    k++;
                }
                it++;
                if (it == token_vector.end())
                {
                    break;
                }
                int v = atoi(it->c_str());
                zcc_imap_client_debug("得到 status: %s => %d", k, v);
                if (ZSTR_EQ(k, "MESSAGES"))
                {
                    ser.messages_ = v;
                }
                else if (ZSTR_EQ(k, "RECENT"))
                {
                    ser.recent_ = v;
                }
                else if (ZSTR_EQ(k, "UIDNEXT"))
                {
                    ser.uidnext_ = v;
                }
                else if (ZSTR_EQ(k, "UIDVALIDITY"))
                {
                    ser.uidvalidity_ = v;
                }
                else if (ZSTR_EQ(k, "UNSEEN"))
                {
                    ser.unseen_ = v;
                }
            }
        }
        else
        {
            return parse_imap_result('S', response_tokens);
        }
    }
    return false;
}

zcc_namespace_end;

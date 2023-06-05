/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2023-06-05
 * ================================
 */

#include "./imap.h"

zcc_namespace_begin;

// mail_flags
imap_client::mail_flags::mail_flags()
{
    reset();
}

imap_client::mail_flags::~mail_flags()
{
}

void imap_client::mail_flags::reset()
{
    answered_ = false;
    seen_ = false;
    draft_ = false;
    flagged_ = false;
    deleted_ = false;
    recent_ = false;
    forwarded_ = false;
}

std::string imap_client::mail_flags::to_string()
{
    std::string r;
    if (answered_)
    {
        r.append("\\Answered ");
    }
    if (seen_)
    {
        r.append("\\Seen ");
    }
    if (draft_)
    {
        r.append("\\Draft ");
    }
    if (flagged_)
    {
        r.append("\\Flagged ");
    }
    if (deleted_)
    {
        r.append("\\Deleted ");
    }
    if (r.back() == ' ')
    {
        r.resize(r.size() - 1);
    }
    return r;
}

void imap_client::parse_mail_flags(mail_flags &flags, const response_tokens &response_tokens, int offset)
{
    // * 1 FETCH (UID 1405 FLAGS (\Answered \Flagged \Deleted \Seen \Draft) RFC822.SIZE 67404 BODY[]<0> {12}
    // * 1 FETCH (UID 1405 FLAGS (\Seen) RFC822.SIZE 67404 BODY[]<0> {12}
    // 1 FETCH (UID 1405 FLAGS () RFC822.SIZE 67404 BODY[]<0> {12}

    auto &token_vector = response_tokens.token_vector_;
    if (token_vector.size() < (size_t)offset + 1)
    {
        return;
    }

    bool stop = false;
    for (auto it = token_vector.begin() + offset; (!stop) && (it != token_vector.end()); it++)
    {
        std::string tmp = *it;
        if (tmp.empty())
        {
            continue;
        }
        while (tmp.back() == ')')
        {
            tmp.pop_back();
        }
        zcc::tolower(tmp);
        const char *s = tmp.c_str();
        while ((s[0] == '(') || (s[0] == '\\')) {
            s++;
        }
        if (ZSTR_EQ(s, "answered"))
        {
            flags.answered_ = true;
        }
        else if (ZSTR_EQ(s, "flagged"))
        {
            flags.flagged_ = true;
        }
        else if (ZSTR_EQ(s, "deleted"))
        {
            flags.deleted_ = true;
        }
        else if (ZSTR_EQ(s, "seen"))
        {
            flags.seen_ = true;
        }
        else if (ZSTR_EQ(s, "draft"))
        {
            flags.draft_ = true;
        }
    }
}

bool imap_client::get_message(FILE *dest_fp, mail_flags &flags, int uid)
{
    // F FETCH 1 (UID FLAGS RFC822.SIZE BODY.PEEK[]<0.12>)
    // * 1 FETCH (UID 1405 FLAGS (\Seen) RFC822.SIZE 67404 BODY[]<0> {12}
    // Received: fr)
    // F OK Fetch completed (0.002 + 0.000 + 0.001 secs).

    if (need_close_connection_)
    {
        return false;
    }
    
    flags.reset();
    int extra_length;
    std::string linebuf;
    response_tokens response_tokens;
    bool ok = false;

    linebuf.clear();
    zcc::sprintf_1024(linebuf, "F UID FETCH %d (FLAGS BODY.PEEK[])", uid);
    fp_append(linebuf).fp_append("\r\n");
    zcc_imap_client_debug_write_line(linebuf);

    while (1)
    {
        response_tokens.reset();
        if (!read_response_tokens_oneline(response_tokens, extra_length))
        {
            break;
        }
        if (response_tokens.token_vector_.size() < 3)
        {
            logic_error_ = true;
            need_close_connection_ = true;
            break;
        }
        if (response_tokens.token_vector_[0] == "*")
        {
            parse_mail_flags(flags, response_tokens, 3);

            if (!read_big_data(dest_fp, response_tokens.token_vector_.back(), extra_length))
            {
                break;
            }
            zcc_imap_client_debug("获取一封信件, (Answered:%d, Seen: %d, Draft:%d, Flagged: %d, Deleted: %d, Recent: %d)", flags.answered_, flags.seen_, flags.draft_, flags.flagged_, flags.deleted_, flags.recent_);
            if (extra_length > -1) {
                if (!ignore_left_token(-1))
                {
                    break;
                }
            }
        }
        else
        {
            if (extra_length > -1) {
                if (!ignore_left_token(extra_length))
                {
                    break;
                }
            }
            if (!parse_imap_result('F', response_tokens)) {
                break;
            }
            ok = true;
            break;
        }
    }
    return ok;
}

zcc_namespace_end;

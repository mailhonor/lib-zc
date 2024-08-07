/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2023-06-03
 * ================================
 */

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wformat="
#endif // __GNUC__

#include "./imap.h"

#include <chrono>

// 命令 COPY
// 1 copy 1:3 abc
// 1 OK [COPYUID 1683689640 20:22 30:32] Copy completed (0.009 + 0.000 + 0.008 secs).

// 命令 COPY
// 1 move 1:3 abc
// * OK [COPYUID 1683689640 20:22 33:35] Moved UIDs.
// * 3 EXPUNGE
// * 2 EXPUNGE
// * 1 EXPUNGE
// 1 OK Move completed (0.010 + 0.000 + 0.009 secs).

// 命令 APPEND
// a append inbox {3}
// + OK
// 123
// a OK [APPENDUID 1683689637 36] Append completed (0.009 + 1.080 + 0.007 secs).

zcc_namespace_begin;

imap_client::uidplus_result::uidplus_result()
{
    reset();
}

imap_client::uidplus_result::~uidplus_result()
{
}

void imap_client::uidplus_result::reset()
{
    uidvalidity_ = -1;
    uid_ = -1;
}

static bool _parse_move_or_copy_result(imap_client::uidplus_result &result, const imap_client::response_tokens &response_tokens)
{
    result.reset();
    auto &token_vector = response_tokens.token_vector_;
    auto it = token_vector.begin() + 1;
    if (it == token_vector.end())
    {
        return false;
    }

    if (tolower((*it)[0]) != 'o')
    {
        return false;
    }

    it++;
    for (; it != token_vector.end(); it++)
    {
        if (*it != "[COPYUID")
        {
            continue;
        }

        it++;
        if (it == token_vector.end())
        {
            return false;
        }
        result.uidvalidity_ = atoi(it->c_str());

        it++;
        if (it == token_vector.end())
        {
            return false;
        }

        it++;
        if (it == token_vector.end())
        {
            return false;
        }
        result.uid_ = atoi(it->c_str());
        return true;
    }
    return false;
}

static bool _parse_append_result(imap_client::uidplus_result &result, const imap_client::response_tokens &response_tokens)
{
    result.reset();
    auto &token_vector = response_tokens.token_vector_;
    auto it = token_vector.begin() + 1;
    if (it == token_vector.end())
    {
        return false;
    }

    if (tolower((*it)[0]) != 'o')
    {
        return false;
    }

    it++;
    for (; it != token_vector.end(); it++)
    {
        if (*it != "[APPENDUID")
        {
            continue;
        }

        it++;
        if (it == token_vector.end())
        {
            return false;
        }
        result.uidvalidity_ = atoi(it->c_str());

        it++;
        if (it == token_vector.end())
        {
            return false;
        }
        result.uid_ = atoi(it->c_str());
        return true;
    }
    return false;
}

int imap_client::cmd_store(const char *uids, bool plus_or_minus, const char *flags)
{
    if (zcc::empty(uids) || std::strchr(uids, '\n'))
    {
        return 1;
    }
    if (zcc::empty(flags) || std::strchr(flags, '\n'))
    {
        return 1;
    }
    if (need_close_connection_)
    {
        return -1;
    }
    std::string linebuf;
    linebuf.clear();
    linebuf.append("S UID STORE ").append(uids).append(" ");
    linebuf.append(plus_or_minus ? "+" : "-").append("FLAGS.SILENT (").append(flags).append(")");
    return do_quick_cmd(linebuf);
}

int imap_client::cmd_store(const char *uids, bool plus_or_minus, mail_flags &flags)
{
    if (zcc::empty(uids) || std::strchr(uids, '\n'))
    {
        return 1;
    }
    if (need_close_connection_)
    {
        return -1;
    }
    std::string flags_str = flags.to_string();
    if (flags_str.empty())
    {
        return 1;
    }
    std::string linebuf;
    linebuf.clear();
    linebuf.append("S UID STORE ").append(uids).append(" ");
    linebuf.append(plus_or_minus ? "+" : "-").append("FLAGS.SILENT (").append(flags_str).append(")");
    return do_quick_cmd(linebuf);
}

int imap_client::cmd_store(int uid, bool plus_or_minus, mail_flags &flags)
{
    std::string uids = std::to_string(uid);
    return cmd_store(uids.c_str(), plus_or_minus, flags);
}

int imap_client::cmd_store_deleted_flag(const char *uids, bool plus_or_minus)
{
    if (std::strchr(uids, '\n'))
    {
        return 0;
    }
    if (need_close_connection_)
    {
        return 0;
    }
    std::string linebuf;
    linebuf.clear();
    linebuf.append("S UID STORE ").append(uids).append(" ");
    linebuf.append(plus_or_minus ? "+" : "-").append("FLAGS.SILENT (\\Deleted)");
    return do_quick_cmd(linebuf);
}

int imap_client::cmd_store_deleted_flag(int uid, bool plus_or_minus)
{
    std::string uids = std::to_string(uid);
    return cmd_store_deleted_flag(uids.c_str(), plus_or_minus);
}

int imap_client::cmd_expunge()
{
    if (need_close_connection_)
    {
        return -1;
    }
    std::string linebuf;
    linebuf.append("E EXPUNGE");
    return do_quick_cmd(linebuf);
}

int imap_client::delete_mail(int uid)
{
    if (need_close_connection_)
    {
        return -1;
    }
    int r;
    std::string linebuf;
    char intbuf[32];
    std::sprintf(intbuf, "%d", uid);

    linebuf.clear();
    linebuf.append("S UID STORE ").append(intbuf).append(" +FLAGS.SILENT (\\Deleted)");
    if ((r = do_quick_cmd(linebuf)) < 1)
    {
        return r;
    }

    linebuf.clear();
    linebuf.append("E EXPUNGE");
    return do_quick_cmd(linebuf);
}

int imap_client::cmd_move(uidplus_result &result, int from_uid, const char *to_folder_name_imap)
{
    int r = 0;
    result.reset();
    response_tokens response_tokens;
    bool dealed_uidplus = false;
    std::string linebuf;
    char intbuf[32];
    std::sprintf(intbuf, "%d", from_uid);

    linebuf.clear();
    linebuf.append("M UID MOVE ").append(intbuf).append(" ").append(escape_string(to_folder_name_imap));
    fp_append(linebuf).fp_append("\r\n");
    zcc_imap_client_debug_write_line(linebuf);

    while (1)
    {
        zcc_imap_client_read_token_vecotr_one_loop();
        if (response_tokens.token_vector_[0] == "*")
        {
            if (!dealed_uidplus)
            {
                if (_parse_move_or_copy_result(result, response_tokens))
                {
                    dealed_uidplus = true;
                }
            }
            continue;
        }

        if ((r = parse_imap_result('M', response_tokens)) < 1)
        {
            return r;
        }
        if (!dealed_uidplus)
        {
            if (_parse_move_or_copy_result(result, response_tokens))
            {
                dealed_uidplus = true;
            }
        }
        return 1;
    }
    return -1;
}

int imap_client::cmd_copy(uidplus_result &result, int from_uid, const char *to_folder_name_imap)
{
    result.reset();
    int r;
    response_tokens response_tokens;
    bool dealed_uidplus = false;
    std::string linebuf;
    char intbuf[32];
    std::sprintf(intbuf, "%d", from_uid);

    linebuf.clear();
    linebuf.append("C UID COPY ").append(intbuf).append(" ").append(escape_string(to_folder_name_imap));
    fp_append(linebuf).fp_append("\r\n");
    zcc_imap_client_debug_write_line(linebuf);

    while (1)
    {
        zcc_imap_client_read_token_vecotr_one_loop();
        if (response_tokens.token_vector_[0] == "*")
        {
            if (!dealed_uidplus)
            {
                if (_parse_move_or_copy_result(result, response_tokens))
                {
                    dealed_uidplus = true;
                }
            }
            continue;
        }

        if ((r = parse_imap_result('C', response_tokens)) < 1)
        {
            return r;
        }
        if (!dealed_uidplus)
        {
            if (_parse_move_or_copy_result(result, response_tokens))
            {
                dealed_uidplus = true;
            }
        }
        return 1;
    }
    return -1;
}

imap_client::append_session::append_session()
{
    reset();
}

imap_client::append_session::~append_session()
{
}

void imap_client::append_session::reset()
{
    flags_.reset();
    time_ = -1;
    to_folder_ = "";
}

int imap_client::cmd_append_prepare_protocol(append_session &append)
{
    std::string linebuf;
    std::string flags_str = append.flags_.to_string();
    linebuf.clear();
    linebuf.append("A APPEND ").append(escape_string(append.to_folder_));
    if (!flags_str.empty())
    {
        linebuf.append(" (").append(flags_str).append(")");
    }

    if (append.time_ > 0)
    {
        char timebuf[64 + 1];
        std::tm *now_tm = std::localtime((time_t *)&(append.time_));
        std::strftime(timebuf, 64, "%d-%b-%Y %T %Z", now_tm);
        linebuf.append(" \"").append(timebuf).append("\"");
    }
    linebuf.append(" {").append(std::to_string(append.mail_size_)).append("}");
    fp_append(linebuf).fp_append("\r\n");
    zcc_imap_client_debug_write_line(linebuf);

    if ((fp_gets(linebuf, 10240) < 0) || linebuf.empty())
    {
        zcc_imap_client_info("ERROR READ");
        return -1;
    }
    zcc_imap_client_debug_read_line(linebuf);
    if (linebuf[0] != '+')
    {
        zcc_imap_client_info("ERROR want '+ ...' instead of (%s)", linebuf.c_str());
        return 0;
    }
    return 1;
}

int imap_client::cmd_append_over(uidplus_result &result)
{
    result.reset();
    int r;
    fp_append("\r\n");
    bool dealed_uidplus = false;
    response_tokens response_tokens;
    while (1)
    {
        zcc_imap_client_read_token_vecotr_one_loop();
        if (response_tokens.token_vector_[0] == "*")
        {
            continue;
        }

        if ((r = parse_imap_result('A', response_tokens)) < 1)
        {
            return r;
        }
        if (!dealed_uidplus)
        {
            if (!_parse_append_result(result, response_tokens))
            {
                dealed_uidplus = true;
            }
        }
        return 1;
    }
    return 0;
}

int imap_client::append_file(uidplus_result &result, append_session &append, const char *filename)
{
    int r = -1;
    FILE *fp = 0;
    char buf[4096 + 1];
    int64_t size, left;
    int64_t len;

    fp = fopen(filename, "rb");
    if (!fp)
    {
        zcc_imap_client_info("ERROR open %s", filename);
        goto over;
    }
    size = file_get_size(filename);
    if (size < 0)
    {
        zcc_imap_client_info("ERROR open %s", filename);
        goto over;
    }
    append.mail_size_ = size;
    if ((r = cmd_append_prepare_protocol(append)) < 1)
    {
        goto over;
    }

    left = size;
    while (left > 0)
    {
        len = left;
        if (len > 4096)
        {
            len = 4096;
        }
        len = fread(buf, 1, len, fp);
        if (len < 1)
        {
            zcc_imap_client_info("ERROR read %s", filename);
            need_close_connection_ = true;
            goto over;
        }
        if (fp_.write(buf, (int)len) < 0)
        {
            zcc_imap_client_info("ERROR write");
            need_close_connection_ = true;
            goto over;
        }
        left -= len;
    }
    if ((r = cmd_append_over(result)) < 1)
    {
        r = -1;
        goto over;
    }

    r = 1;
over:
    if (fp)
    {
        fclose(fp);
    }
    return r;
}

int imap_client::append_data(uidplus_result &result, append_session &append, const void *data, uint64_t dlen)
{
    int r = -1;
    append.mail_size_ = dlen;
    if (!cmd_append_prepare_protocol(append))
    {
        goto over;
    }

    if (fp_.write(data, (int)dlen) < 0)
    {
        zcc_imap_client_info("ERROR write");
        need_close_connection_ = true;
        goto over;
    }
    if ((r = cmd_append_over(result)) < 1)
    {
        r = -1;
        goto over;
    }

    r = 1;
over:
    return r;
}

zcc_namespace_end;

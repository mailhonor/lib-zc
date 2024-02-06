/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2023-06-05
 * ================================
 */

#include "./imap.h"

zcc_namespace_begin;

int imap_client::cmd_create(const std::string &pathname_utf7)
{
    if (need_close_connection_)
    {
        return -1;
    }
    std::string linebuf;
    linebuf = "C create ";
    linebuf.append(escape_string(pathname_utf7));
    fp_append(linebuf).fp_append("\r\n");
    zcc_imap_client_debug_write_line(linebuf);

    while (1)
    {
        linebuf.clear();
        if (fp_gets(linebuf, 10240) < 0)
        {
            return -1;
        }
        zcc_imap_client_debug_read_line(linebuf);
        if (linebuf[0] == '*')
        {
            continue;
        }
        else
        {
            return parse_imap_result('C', linebuf.c_str());
        }
    }
    return -1;
}

int imap_client::cmd_rename(const std::string &from_pathname_utf7, const std::string &to_pathname_utf7)
{
    if (need_close_connection_)
    {
        return -1;
    }
    std::string linebuf;
    linebuf = "R rename";
    linebuf.append(" ").append(escape_string(from_pathname_utf7));
    linebuf.append(" ").append(escape_string(to_pathname_utf7));
    fp_append(linebuf).fp_append("\r\n");
    zcc_imap_client_debug_write_line(linebuf);

    while (1)
    {
        linebuf.clear();
        if (fp_gets(linebuf, 10240) < 0)
        {
            return -1;
        }
        zcc_imap_client_debug_read_line(linebuf);
        if (linebuf[0] == '*')
        {
            continue;
        }
        else
        {
            return parse_imap_result('R', linebuf.c_str());
        }
    }
    return -1;
}

int imap_client::cmd_subscribe(const std::string &pathname_utf7, bool tf)
{
    if (need_close_connection_)
    {
        return -1;
    }
    std::string linebuf;
    linebuf = "S ";
    linebuf.append(tf ? "subscribe " : "unsubscribe ").append(escape_string(pathname_utf7));
    fp_append(linebuf).fp_append("\r\n");
    zcc_imap_client_debug_write_line(linebuf);

    while (1)
    {
        linebuf.clear();
        if (fp_gets(linebuf, 10240) < 0)
        {
            return -1;
        }
        zcc_imap_client_debug_read_line(linebuf);
        if (linebuf[0] == '*')
        {
            continue;
        }
        else
        {
            return parse_imap_result('S', linebuf.c_str());
        }
    }
    return -1;
}

int imap_client::cmd_delete(const std::string &pathname_utf7)
{
    if (need_close_connection_)
    {
        return -1;
    }
    std::string linebuf;
    linebuf = "D ";
    linebuf.append("DELETE ").append(escape_string(pathname_utf7));
    fp_append(linebuf).fp_append("\r\n");
    zcc_imap_client_debug_write_line(linebuf);

    while (1)
    {
        linebuf.clear();
        if (fp_gets(linebuf, 10240) < 0)
        {
            return -1;
        }
        zcc_imap_client_debug_read_line(linebuf);
        if (linebuf[0] == '*')
        {
            continue;
        }
        else
        {
            return parse_imap_result('D', linebuf.c_str());
        }
    }
    return -1;
}

zcc_namespace_end;

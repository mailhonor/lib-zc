/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2023-08-10
 * ================================
 */

#include "./imap.h"

zcc_namespace_begin;

void imap_client::set_id(const char *id)
{
    id_.clear();
    if (!zempty(id))
    {
        id_ = id;
    }
}

bool imap_client::cmd_id(const char *id)
{
    if (need_close_connection_)
    {
        return false;
    }
    if (zempty(id) && id_.empty())
    {
        return true;
    }
    std::string linebuf;
    linebuf = "I ID ";
    if (!zempty(id))
    {
        linebuf.append(id);
    }
    else
    {
        linebuf.append(id_);
    }

    if (!do_quick_cmd(linebuf, false))
    {
        return false;
    }

    return true;
}

zcc_namespace_end;

/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2023-08-10
 * ================================
 */

#include "./imap.h"

zcc_namespace_begin;

int imap_client::cmd_id(const char *id)
{
    if (need_close_connection_)
    {
        return -1;
    }
    if (zcc::empty(id))
    {
        id = "(\"name\" \"zcc\")";
    }
    std::string linebuf;
    linebuf = "I ID ";
    linebuf.append(id);

    return do_quick_cmd(linebuf);
}

zcc_namespace_end;

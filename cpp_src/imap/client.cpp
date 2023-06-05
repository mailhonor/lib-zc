/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2023-06-05
 * ================================
 */

#include "./imap.h"

zcc_namespace_begin;

imap_client::imap_client()
{
    timeout_ = 10;
    simple_line_length_limit_ = 102400;
}

imap_client::~imap_client()
{
}

void imap_client::set_simple_line_length_limit(int limit)
{
    if (limit < 1024) {
        limit = 1024;
    }
    simple_line_length_limit_ = limit;
}

imap_client &imap_client::fp_append(const char *s, int slen)
{
    fp_.append(s, slen);
    return *this;
}

imap_client &imap_client::fp_append(const std::string &s)
{
    fp_.append(s);
    return *this;
}

int imap_client::fp_readn(void *mem, int strict_len)
{
    int r = fp_.readn(mem, strict_len);
    if (r < strict_len)
    {
        connection_error_ = true;
        need_close_connection_ = true;
    }
    return r;
}

int imap_client::fp_readn(std::string &str, int strict_len)
{
    int r = fp_.readn(str, strict_len);
    if (r < strict_len)
    {
        connection_error_ = true;
        need_close_connection_ = true;
    }
    return r;
}

int imap_client::fp_read_delimiter(void *mem, int delimiter, int max_len)
{
    int r = fp_.read_delimiter(mem, delimiter, max_len);
    if (r < 0)
    {
        connection_error_ = true;
        need_close_connection_ = true;
    }
    return r;
}
int imap_client::fp_read_delimiter(std::string &str, int delimiter, int max_len)
{
    int r = fp_.read_delimiter(str, delimiter, max_len);
    if (r < 0)
    {
        connection_error_ = true;
        need_close_connection_ = true;
    }
    return r;
}

bool imap_client::check_is_need_close()
{
    if (error_ || connection_error_ || need_close_connection_ || logic_error_)
    {
        return true;
    }
    return false;
}

zcc_namespace_end;

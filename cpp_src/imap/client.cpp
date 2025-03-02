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
    simple_line_length_limit_ = 102400;
    fp_ = new iostream();
}

imap_client::imap_client(stream *third_stream, bool auto_release_third_stream)
{
    simple_line_length_limit_ = 102400;
    third_stream_mode_ = true;
    auto_release_third_stream_ = auto_release_third_stream;
    fp_ = third_stream;
}

imap_client::~imap_client()
{
    if (!third_stream_mode_)
    {
        delete fp_;
    }
    else
    {
        if (auto_release_third_stream_)
        {
            delete fp_;
        }
    }
}

void imap_client::set_simple_line_length_limit(int limit)
{
    if (limit < 1024)
    {
        limit = 1024;
    }
    simple_line_length_limit_ = limit;
}

imap_client &imap_client::fp_append(const char *s, int slen)
{
    if (slen < 0)
    {
        slen = std::strlen(s);
    }
    fp_->append(s, slen);
    if (debug_protocol_fn_)
    {
        debug_protocol_fn_('C', s, slen);
    }
    return *this;
}

imap_client &imap_client::fp_append(const std::string &s)
{
    fp_->append(s);
    if (debug_protocol_fn_)
    {
        debug_protocol_fn_('C', s.c_str(), (int)s.size());
    }
    return *this;
}

int imap_client::fp_readn(void *mem, int strict_len)
{
    int r = fp_->readn(mem, strict_len);
    if (r < strict_len)
    {
        need_close_connection_ = true;
    }

    if (r > 0)
    {
        if (debug_protocol_fn_)
        {
            debug_protocol_fn_('S', (const char *)mem, r);
        }
    }
    return r;
}

int imap_client::fp_readn(std::string &str, int strict_len)
{
    int r = fp_->readn(str, strict_len);
    if (r < strict_len)
    {
        need_close_connection_ = true;
    }
    if (r > 0)
    {
        if (debug_protocol_fn_)
        {
            debug_protocol_fn_('S', str.c_str(), str.size());
        }
    }
    return r;
}

int imap_client::fp_read_delimiter(void *mem, int delimiter, int max_len)
{
    int r = fp_->read_delimiter(mem, delimiter, max_len);
    if (r < 0)
    {
        need_close_connection_ = true;
    }
    if (r > 0)
    {
        if (debug_protocol_fn_)
        {
            debug_protocol_fn_('S', (const char *)mem, r);
        }
    }
    return r;
}
int imap_client::fp_read_delimiter(std::string &str, int delimiter, int max_len)
{
    int r = fp_->read_delimiter(str, delimiter, max_len);
    if (r < 0)
    {
        need_close_connection_ = true;
    }
    if (r > 0)
    {
        if (debug_protocol_fn_)
        {
            debug_protocol_fn_('S', str.c_str(), str.size());
        }
    }
    return r;
}

zcc_namespace_end;

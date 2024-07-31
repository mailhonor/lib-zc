/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2017-07-22
 * ================================
 */

#include "zcc/zcc_http.h"

zcc_namespace_begin;

std::string httpd::get_log_prefix()
{
    std::string r;

    if (method_.empty())
    {
        r.append("NIL");
    }
    else
    {
        r.append(method_);
    }

    if (uri_.empty())
    {
        r.append(" NIL");
    }
    else
    {
        r.append("\"").append(uri_).append("\"");
    }

    int ip, port = 0;
    char ipstr[18] = {0};
    if (get_peername(fp_->get_socket(), &ip, &port) > 0)
    {
        get_ipstring(ip, ipstr);
    }
    else
    {
        ipstr[0] = '0';
        ipstr[1] = 0;
    }
    r.append(" ").append(ipstr).append(":").append(std::to_string(port));
    return r;
}

void httpd::vlog_output(log_level ll, const char *fmt, va_list ap)
{
}

void httpd::log_info(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vlog_output(log_level::info, fmt, ap);
    va_end(ap);
}

void httpd::log_error(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vlog_output(log_level::error, fmt, ap);
    va_end(ap);
}

zcc_namespace_end;
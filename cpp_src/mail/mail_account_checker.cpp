/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2023-06-05
 * ================================
 */

#include "zcc/zcc_mail.h"
#include "zcc/zcc_smtp.h"
#include "zcc/zcc_imap.h"
#include "zcc/zcc_pop.h"

zcc_namespace_begin;
//
mail_account_checker_result::mail_account_checker_result()
{
}

mail_account_checker_result::~mail_account_checker_result()
{
}

void mail_account_checker_result::reset()
{
    protocol_.clear();
    connected_destination_.clear();
    protocol_log_.clear();
    last_response_line_.clear();
    error_stage_ = error_none;
}

//

// mail_account_checker
mail_account_checker::mail_account_checker()
{
}

mail_account_checker::~mail_account_checker()
{
}

bool mail_account_checker::check(mail_account_checker_result &result)
{
    return do_actual_checking_work(result);
}

bool mail_account_checker::check()
{
    return do_actual_checking_work(result_);
}

bool mail_account_checker::check_host_dns()
{
    std::vector<std::string> addrs;
    int ret = zcc::get_hostaddr(host_, addrs);
    if (ret < 1)
    {
        return false;
    }
    return true;
}

void mail_account_checker::set_server(const std::string &server, int port)
{
    size_t pos = server.find(':');
    if (pos == std::string::npos)
    {
        host_ = server;
        port_ = port;
    }
    else
    {
        host_ = server.substr(0, pos);
        port_ = std::atoi(server.c_str() + pos + 1);
    }
}

void mail_account_checker::set_username_password(const std::string &username, const std::string &password)
{
    username_ = username;
    password_ = password;
}

void mail_account_checker::set_ssl_mode(SSL_CTX *ssl_ctx)
{
    ssl_ctx_ = ssl_ctx;
    ssl_mode_ = true;
    tls_mode_ = false;
    try_tls_mode_ = false;
}

void mail_account_checker::set_tls_mode(SSL_CTX *ssl_ctx)
{
    ssl_ctx_ = ssl_ctx;
    ssl_mode_ = false;
    tls_mode_ = true;
    try_tls_mode_ = false;
}

void mail_account_checker::set_try_tls_mode(SSL_CTX *ssl_ctx)
{
    ssl_ctx_ = ssl_ctx;
    ssl_mode_ = false;
    tls_mode_ = true;
    try_tls_mode_ = true;
}

void mail_account_checker::set_ssl_tls(SSL_CTX *ssl_ctx, bool ssl_mode, bool tls_mode, bool try_tls_mode)
{
    ssl_mode_ = ssl_mode;
    tls_mode_ = tls_mode;
    ssl_ctx_ = ssl_ctx;
    try_tls_mode_ = try_tls_mode;
}

void mail_account_checker::set_timeout(int timeout)
{
    timeout_ = timeout;
}

void mail_account_checker::set_connect_times(int times)
{
    connect_times_ = times;
}

void mail_account_checker::set_record_protocol(bool tf)
{
    record_protocol_ = tf;
}

void mail_account_checker::set_debug_mode(bool tf)
{
    debug_mode_ = tf;
}

void mail_account_checker::set_verbose_mode(bool tf)
{
    verbose_mode_ = tf;
}

void mail_account_checker::set_debug_protocol_mode(bool tf)
{
    debug_protocol_mode_ = tf;
}

// smtp_account_checker
smtp_account_checker::smtp_account_checker()
{
}

smtp_account_checker::~smtp_account_checker()
{
}

#define _account_checker_some_setting()                                                       \
    if (!check_host_dns())                                                                    \
    {                                                                                         \
        result.error_stage_ = error_resolve;                                                  \
        result.last_response_line_ = "(" + host_ + ")";                                       \
        return false;                                                                         \
    }                                                                                         \
    int ret;                                                                                  \
    std::string destination = host_ + ":" + std::to_string(port_);                            \
    client.set_debug_mode(debug_mode_);                                                       \
    client.set_verbose_mode(verbose_mode_);                                                   \
    client.set_debug_protocol_mode(debug_protocol_mode_);                                     \
    client.set_timeout(10);                                                                   \
    if (record_protocol_)                                                                     \
    {                                                                                         \
        client.set_debug_protocol_fn(                                                         \
            [this, &result](mail_protocol_client_or_server cs, const char *line, int linelen) \
            {                                                                                 \
                mail_protocol_line_record record;                                             \
                record.client_or_server = cs;                                                 \
                record.line.assign(line, linelen);                                            \
                trim_line_end_rn(record.line);                                                \
                result.protocol_log_.push_back(record);                                       \
            });                                                                               \
    }                                                                                         \
    if (ssl_mode_)                                                                            \
    {                                                                                         \
        client.set_ssl_mode(ssl_ctx_);                                                        \
    }                                                                                         \
    else if (try_tls_mode_)                                                                   \
    {                                                                                         \
        client.set_try_tls_mode(ssl_ctx_);                                                    \
    }                                                                                         \
    if (client.connect(destination, connect_times_) < 1)                                      \
    {                                                                                         \
        result.error_stage_ = error_connection;                                               \
        result.last_response_line_ = client.get_last_response_line();                         \
        if (result.last_response_line_.empty())                                               \
        {                                                                                     \
            result.last_response_line_ = "(" + get_last_connected_destination() + ")";        \
        }                                                                                     \
        return false;                                                                         \
    }                                                                                         \
    if (!username_.empty())                                                                   \
    {                                                                                         \
        if ((ret = client.do_auth(username_, password_)) < 0)                                 \
        {                                                                                     \
            result.error_stage_ = error_auth;                                                 \
            result.last_response_line_ = client.get_last_response_line();                     \
            return false;                                                                     \
        }                                                                                     \
        else if (ret < 1)                                                                     \
        {                                                                                     \
            result.error_stage_ = error_auth;                                                 \
            result.last_response_line_ = client.get_last_response_line();                     \
            return false;                                                                     \
        }                                                                                     \
    }                                                                                         \
    client.cmd_quit();                                                                        \
    result.last_response_line_ = client.get_last_response_line();

bool smtp_account_checker::do_actual_checking_work(mail_account_checker_result &result)
{
    result.reset();
    result.protocol_ = "SMTP";
    zcc::smtp_client client;
    _account_checker_some_setting();
    return true;
}

// imap_account_checker
imap_account_checker::imap_account_checker()
{
}

imap_account_checker::~imap_account_checker()
{
}

bool imap_account_checker::do_actual_checking_work(mail_account_checker_result &result)
{
    result.reset();
    result.protocol_ = "IMAP";
    zcc::imap_client client;
    _account_checker_some_setting();
    return true;
}

// pop_account_checker
pop_account_checker::pop_account_checker()
{
}

pop_account_checker::~pop_account_checker()
{
}

bool pop_account_checker::do_actual_checking_work(mail_account_checker_result &result)
{
    result.reset();
    result.protocol_ = "POP";
    zcc::pop_client client;
    _account_checker_some_setting();
    return true;
}

zcc_namespace_end;
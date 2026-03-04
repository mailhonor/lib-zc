/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2025-06-03
 * ================================
 */

#include "./smtp.h"

zcc_namespace_begin;

smtp_client_quick_send::smtp_client_quick_send()
{
}

smtp_client_quick_send::~smtp_client_quick_send()
{
}

void smtp_client_quick_send ::set_ssl_mode(SSL_CTX *ssl_ctx)
{
    ssl_ctx_ = ssl_ctx;
    ssl_mode_ = true;
    tls_mode_ = false;
    try_tls_mode_ = false;
}

void smtp_client_quick_send::set_tls_mode(SSL_CTX *ssl_ctx)
{
    ssl_ctx_ = ssl_ctx;
    ssl_mode_ = false;
    tls_mode_ = true;
    try_tls_mode_ = false;
}

void smtp_client_quick_send::set_try_tls_mode(SSL_CTX *ssl_ctx)
{
    ssl_ctx_ = ssl_ctx;
    ssl_mode_ = false;
    tls_mode_ = true;
    try_tls_mode_ = true;
}

void smtp_client_quick_send::set_ssl_tls(SSL_CTX *ssl_ctx, bool ssl_mode, bool tls_mode, bool try_tls_mode)
{
    ssl_mode_ = ssl_mode;
    tls_mode_ = tls_mode;
    ssl_ctx_ = ssl_ctx;
    try_tls_mode_ = try_tls_mode;
}

void smtp_client_quick_send::set_connect_times(int times)
{
    connect_times_ = times;
}

void smtp_client_quick_send::set_timeout(int timeout)
{
    timeout_ = timeout;
}

void smtp_client_quick_send::set_process_callback(std::function<void(smtp_client_stage stage, int process_percentage_value)> callback)
{
    process_callback_ = callback;
}

void smtp_client_quick_send::set_server(const std::string &server, int port)
{
    size_t pos = server.find(':');
    if (pos == std::string::npos)
    {
        destination_ = server + ":" + std::to_string(port);
    }
    else
    {
        destination_ = server;
    }
}

void smtp_client_quick_send::set_username_password(const std::string &username, const std::string &password)
{
    username_ = username;
    password_ = password;
}

void smtp_client_quick_send::set_mail_from(const std::string &mail_from)
{
    mail_from_ = mail_from;
}

void smtp_client_quick_send::add_rcpt_to(const std::string &rcpt_to)
{
    auto to = rcpt_to;
    zcc::tolower(to);
    if (to.empty())
    {
        return;
    }
    if (rcpt_to_unique_.find(to) != rcpt_to_unique_.end())
    {
        return;
    }
    rcpt_to_unique_.insert(to);
    rcpt_to_.push_back(rcpt_to);
}

void smtp_client_quick_send::add_rcpt_to(const std::set<std::string> &rcpt_tos)
{
    for (const auto &to : rcpt_tos)
    {
        add_rcpt_to(to);
    }
}

bool smtp_client_quick_send::send_once_inner(const char *data, int len, const std::string &pathname)
{
    int r;
    stage_ = smtp_client_stage_init;

#define set_process(a, b)        \
    if (process_callback_)       \
    {                            \
        process_callback_(a, b); \
    }

    //
    if ((!data) && pathname.empty())
    {
        zcc_class_debug("数据为空, 文件路径为空");
        return false;
    }
    if (rcpt_to_.empty())
    {
        zcc_class_debug("收件人列表为空");
        return false;
    }

    if (data && len < 0)
    {
        len = (int)std::strlen(data);
    }
    if (!data)
    {
        int64_t size = file_get_size(pathname);
        if (size < 1)
        {
            zcc_class_debug("邮件文件不存在或大小为0或系统错误");
            return false;
        }
        if (size > 1024 * 1024 * 1024)
        {
            zcc_class_debug("邮件文件过大, 超过1GB");
            return false;
        }
        len = (int)size;
    }
    zcc::smtp_client client;
    client.set_debug_mode(debug_mode_);
    client.set_verbose_mode(verbose_mode_);
    client.set_debug_protocol_mode(debug_protocol_mode_);
    client.set_timeout(timeout_);
    client.set_ssl_tls(ssl_ctx_, ssl_mode_, tls_mode_, try_tls_mode_);
    if (record_protocol_logs_)
    {
        client.set_debug_protocol_fn([this](mail_protocol_client_or_server sc, const char *line, int len)
                                     {
                                         mail_protocol_line_record record;
                                         record.client_or_server = sc;
                                         record.line.assign(line, len);
                                         protocol_logs_.push_back(record);
                                         //
                                     });
    }

    //
    set_process(smtp_client_stage_connect, 0);
    if (client.connect(destination_, connect_times_) < 1)
    {
        stage_ = smtp_client_stage_connect;
        last_response_line_ = client.get_last_response_line();
        if (last_response_line_.empty())
        {
            last_response_line_ = "(" + destination_ + ")";
        }
        return false;
    }
    set_process(smtp_client_stage_connect, 100);

    //
    if (!username_.empty())
    {
        set_process(smtp_client_stage_auth, 0);
        if (client.do_auth(username_, password_) < 1)
        {
            stage_ = smtp_client_stage_auth;
            last_response_line_ = client.get_last_response_line();
            return false;
        }
        set_process(smtp_client_stage_auth, 100);
    }

    //
    set_process(smtp_client_stage_mail_from, 0);
    if (client.cmd_mail_from(mail_from_) < 1)
    {
        stage_ = smtp_client_stage_mail_from;
        last_response_line_ = client.get_last_response_line();
        return false;
    }
    set_process(smtp_client_stage_mail_from, 100);

    //
    set_process(smtp_client_stage_rcpt_to, 0);
    size_t sent_count = 0;
    for (auto &to : rcpt_to_)
    {
        sent_count++;
        if (client.cmd_rcpt_to(to) < 1)
        {
            stage_ = smtp_client_stage_rcpt_to;
            last_response_line_ = client.get_last_response_line();
            error_rcpt_ = to;
            return false;
        }
        set_process(smtp_client_stage_rcpt_to, (int)(sent_count * 100 / rcpt_to_.size()));
    }

    if (process_callback_)
    {
        r = client.do_quick_send_once(data, len, pathname, [this, len](int64_t sent)
                                      { process_callback_(smtp_client_stage_data, (int)(sent * 100 / len)); });
    }
    else
    {
        r = client.do_quick_send_once(data, len, pathname);
    }
    if (r < 1)
    {
        stage_ = smtp_client_stage_data;
        last_response_line_ = client.get_last_response_line();
        return false;
    }
    set_process(smtp_client_stage_data, 100);
    last_response_line_ = client.get_last_response_line();

    //
    set_process(smtp_client_stage_quit, 0);
    client.cmd_quit();
    set_process(smtp_client_stage_quit, 100);

    //
    stage_ = smtp_client_stage_done;
    set_process(smtp_client_stage_done, 100);

    //
    return true;
}

bool smtp_client_quick_send::send_once_by_data(const char *data, int len)
{
    return send_once_inner(data, len, "");
}

bool smtp_client_quick_send::send_once_by_file(const std::string &pathname)
{
    return send_once_inner(nullptr, 0, pathname);
}

zcc_namespace_end;

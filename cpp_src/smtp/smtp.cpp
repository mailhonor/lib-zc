/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2025-06-03
 * ================================
 */

#include "./smtp.h"

zcc_namespace_begin;

smtp_client::smtp_client()
{
    fp_ = new iostream();
}

smtp_client::smtp_client(stream *third_stream)
{
    fp_ = third_stream;
}

smtp_client::~smtp_client()
{
    delete fp_;
}

void smtp_client::set_ssl_mode(SSL_CTX *ssl_ctx)
{
    ssl_ctx_ = ssl_ctx;
    ssl_mode_ = true;
    tls_mode_ = false;
    try_tls_mode_ = false;
}

void smtp_client::set_tls_mode(SSL_CTX *ssl_ctx)
{
    ssl_ctx_ = ssl_ctx;
    ssl_mode_ = false;
    tls_mode_ = true;
    try_tls_mode_ = false;
}

void smtp_client::set_try_tls_mode(SSL_CTX *ssl_ctx)
{
    ssl_ctx_ = ssl_ctx;
    ssl_mode_ = false;
    tls_mode_ = true;
    try_tls_mode_ = true;
}

void smtp_client::set_ssl_tls(SSL_CTX *ssl_ctx, bool ssl_mode, bool tls_mode, bool try_tls_mode)
{
    ssl_mode_ = ssl_mode;
    tls_mode_ = tls_mode;
    ssl_ctx_ = ssl_ctx;
    try_tls_mode_ = try_tls_mode;
}

void smtp_client::set_timeout(int timeout)
{
    fp_->set_timeout(timeout);
}

int smtp_client::simple_quick_cmd(const std::string &cmd, const std::string &debug_info)
{
    if (need_close_connection_)
    {
        return -1;
    }
    ehlo_response_lines_.clear();
    fp_->append(cmd).append("\r\n");
    if (debug_protocol_fn_)
    {
        if (debug_info.empty())
        {
            debug_protocol_fn_('C', cmd.c_str(), cmd.size());
        }
        else
        {
            debug_protocol_fn_('C', debug_info.c_str(), debug_info.size());
        }
    }
    if (fp_->gets(last_response_line_, 10240) < 1)
    {
        need_close_connection_ = true;
        return -1;
    }
    zcc::trim_line_end_rn(last_response_line_);
    if (debug_protocol_fn_)
    {
        debug_protocol_fn_('S', last_response_line_.c_str(), last_response_line_.size());
    }
    if (last_response_line_[0] == '2' || last_response_line_[0] == '3')
    {
        return 1;
    }
    return 0;
}

int smtp_client::do_STARTTLS()
{
    if (need_close_connection_)
    {
        return -1;
    }
    int r;
    if (ssl_flag_)
    {
        return 1;
    }

    if ((r = simple_quick_cmd("STARTTLS")) < 1)
    {
        return r;
    }

    if (fp_->tls_connect(ssl_ctx_) < 0)
    {
        zcc_smtp_client_error("建立SSL");
        return -1;
    }
    ssl_flag_ = true;

    return 1;
}

int smtp_client::fp_connect(const char *destination, int times)
{
    if (need_close_connection_)
    {
        return -1;
    }
    if (connected_)
    {
        return 1;
    }
    if (times > 0)
    {
        for (int i = 0; i < times; i++)
        {
            if (fp_connect(destination, 0) > 0)
            {
                return 1;
            }
        }
        return -1;
    }
    if (!fp_->connect(destination))
    {
        zcc_smtp_client_error("连接(%s)", destination);
        need_close_connection_ = true;
        return -1;
    }
    if (ssl_mode_)
    {
        if (fp_->tls_connect(ssl_ctx_) < 0)
        {
            disconnect();
            need_close_connection_ = true;
            zcc_smtp_client_error("建立SSL(%s)", destination);
            return -1;
        }
        ssl_flag_ = true;
    }
    need_close_connection_ = false;
    connected_ = true;
    return 1;
}

void smtp_client::disconnect()
{
    fp_->close();
    opened_ = false;
    need_close_connection_ = false;
    connected_ = false;
    authed_ = false;
    ssl_flag_ = false;
}

int smtp_client::welcome()
{
    if (fp_->gets(last_response_line_, 10240) < 1)
    {
        need_close_connection_ = true;
        return -1;
    }
    zcc::trim_line_end_rn(last_response_line_);
    if (debug_protocol_fn_)
    {
        debug_protocol_fn_('S', last_response_line_.c_str(), last_response_line_.size());
    }
    if (last_response_line_[0] == '2' || last_response_line_[0] == '3')
    {
        return 1;
    }
    return 0;
}

int smtp_client::connect(const char *destination, int times)
{
    int r = -1;
    if (opened_)
    {
        return 1;
    }
    if (times < 1)
    {
        times = 1;
    }

    if ((r = fp_connect(destination, 3)) < 1)
    {
        return r;
    }

    if ((r = welcome()) < 1)
    {
        return r;
    }

    if (tls_mode_)
    {
        if ((r = do_STARTTLS()) < 1)
        {
            if (r < 0)
            {
                return r;
            }
            if (!try_tls_mode_)
            {
                return -1;
            }
        }
    }
    if ((r = cmd_ehlo(ehlo_key_.c_str())) < 1)
    {
        return r;
    }

    return 1;
}

int smtp_client::cmd_quit()
{
    if (need_close_connection_)
    {
        return 1;
    }
    return simple_quick_cmd("QUIT");
}

int smtp_client::do_auth_basic(const char *user, const char *password)
{
    std::string cmd;
    int r;

    if (need_close_connection_)
    {
        return -1;
    }

    if (empty(user))
    {
        return 1;
    }
    if ((r = simple_quick_cmd("AUTH LOGIN")) < 1)
    {
        return r;
    }
    std::string debug_line;
    debug_line = "(user: " + std::string(user) + ")";
    if ((r = simple_quick_cmd(base64_encode(user), debug_line)) < 1)
    {
        return r;
    }
    if ((r = simple_quick_cmd(base64_encode(password), "(password)")) < 1)
    {
        return r;
    }

    return 1;
}

int smtp_client::do_auth(const char *user, const char *password)
{
    return do_auth_basic(user, password);
}

int smtp_client::cmd_noop()
{
    if (need_close_connection_)
    {
        return -1;
    }
    return simple_quick_cmd("NOOP");
}

int smtp_client::cmd_rset()
{
    if (need_close_connection_)
    {
        return -1;
    }
    return simple_quick_cmd("RSET");
}

int smtp_client::cmd_helo(const char *key)
{
    if (need_close_connection_)
    {
        return -1;
    }
    if (empty(key))

    {
        return 1;
    }
    return simple_quick_cmd("HELO " + std::string(key));
}

int smtp_client::cmd_ehlo(const char *key)
{
    if (need_close_connection_)
    {
        return -1;
    }
    if (empty(key))
    {
        return 1;
    }
    ehlo_response_lines_.clear();
    fp_->append("EHLO ").append(key).append("\r\n");
    if (debug_protocol_fn_)
    {
        std::string debug_line = "EHLO " + std::string(key);
        debug_protocol_fn_('C', debug_line.c_str(), debug_line.size());
    }
    for (int i = 0; i < 100; i++)
    {
        if (fp_->gets(last_response_line_, 10240) < 1)
        {
            need_close_connection_ = true;
            return -1;
        }
        zcc::trim_line_end_rn(last_response_line_);
        if (debug_protocol_fn_)
        {
            debug_protocol_fn_('S', last_response_line_.c_str(), last_response_line_.size());
        }
        ehlo_response_lines_.push_back(last_response_line_);
        if (last_response_line_.size() < 3)
        {
            continue;
        }
        if ((!zcc::isdigit(last_response_line_[0])) || (!zcc::isdigit(last_response_line_[1])) || (!zcc::isdigit(last_response_line_[2])))
        {
            continue;
        }
        if (last_response_line_[3] == ' ' || last_response_line_[3] == '\0')
        {
            if (last_response_line_[0] == '2' || last_response_line_[0] == '3')
            {
                return 1;
            }
            return 0;
        }
    }
    need_close_connection_ = true;
    return -1;
}

int smtp_client::cmd_mail_from(const std::string &from, const std::string &params)
{
    if (need_close_connection_)
    {
        return -1;
    }
    std::string cmd;
    cmd = "MAIL FROM: <" + from + ">";
    if (!params.empty())
    {
        cmd += " " + params;
    }
    return simple_quick_cmd(cmd);
}

int smtp_client::cmd_rcpt_to(const std::string &to)
{
    if (need_close_connection_)
    {
        return -1;
    }
    return simple_quick_cmd("RCPT TO: <" + to + ">");
}

int smtp_client::cmd_data()
{
    if (need_close_connection_)
    {
        return -1;
    }
    return simple_quick_cmd("DATA");
}

static int smtp_client_do_send_raw_data_once(smtp_client *smtp, const char *ps, const char *end, std::function<void(size_t send_bytes)> process_callback, size_t send_bytes)
{
    while (ps < end)
    {
        size_t write_size = end - ps;
        if (write_size > 10240)
        {
            write_size = 10240;
        }
        if (smtp->get_iostream().write(ps, write_size) < 1)
        {
            return -1;
        }
        ps += write_size;
        send_bytes += write_size;
        process_callback(send_bytes);
    }
    return 1;
}

int smtp_client::do_quick_send_raw_data_once(const char *data, size_t size, std::function<void(int64_t send_bytes)> process_callback)
{
    if (need_close_connection_)
    {
        return -1;
    }
    int r;
    ehlo_response_lines_.clear();
    if ((r = cmd_data()) < 1)
    {
        return r;
    }
    if (debug_protocol_fn_)
    {
        std::string debug_line = "(send data: " + std::to_string(size) + " bytes)";
        debug_protocol_fn_('C', debug_line.c_str(), (int)debug_line.size());
    }
    if (size < 1)
    {
        return 1;
    }
    const char *ps = data, *end = data + size;
    const char *p = nullptr;
    if (*ps == '.')
    {
        fp_->append("..");
        ps++;
    }
    while (ps < end)
    {
        const char *pss = ps;
        const char *p = (const char *)zcc::memmem(ps, end - ps, "\n.", 2);
        if (!p)
        {
            ps = end;
            if (process_callback)
            {
                smtp_client_do_send_raw_data_once(this, pss, end, process_callback, pss - data);
            }
            else
            {
                fp_->append(pss, ps - pss);
            }
        }
        else
        {
            ps = p + 1;
            if (process_callback)
            {
                smtp_client_do_send_raw_data_once(this, pss, p + 2, process_callback, pss - data);
            }
            else
            {
                fp_->append(pss, p - pss + 2);
            }
        }
        if (fp_->is_exception())
        {
            need_close_connection_ = true;
            return -1;
        }
    }
    if ((r = cmd_data_end()) < 1)
    {
        return r;
    }
    return 1;
}

int smtp_client::do_quick_send_file_once(const std::string &filename, std::function<void(int64_t send_bytes)> process_callback)
{
    if (need_close_connection_)
    {
        return -1;
    }
    zcc::mmap_reader reader;
    if (reader.open(filename.c_str()) < 1)
    {
        zcc_smtp_client_error("打开文件(%s)", filename.c_str());
        return -1;
    }
    size_t size = reader.size_;
    if (debug_protocol_fn_)
    {
        std::string debug_line = "(send file: " + filename + ")";
        debug_protocol_fn_('C', debug_line.c_str(), (int)debug_line.size());
    }
    if (size < 1)
    {
        return 1;
    }
    return do_quick_send_raw_data_once((const char *)reader.data_, size, process_callback);
}

int smtp_client::cmd_data_end()
{
    if (need_close_connection_)
    {
        return -1;
    }
    return simple_quick_cmd("\r\n.", "(data end)");
}

zcc_namespace_end;

/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2026-06-03
 * ================================
 */

#pragma once

#ifndef ZCC_LIB_INCLUDE_SMTP___
#define ZCC_LIB_INCLUDE_SMTP___

#include <functional>
#include "./zcc_stdlib.h"
#include "./zcc_stream.h"

#ifdef __cplusplus
#pragma pack(push, 4)
zcc_namespace_begin;

class smtp_client
{
public:
    smtp_client();
    smtp_client(stream *third_stream);
    virtual ~smtp_client();
    inline void set_debug_mode(bool tf = true) { debug_mode_ = tf; }
    inline void set_verbose_mode(bool tf = true) { verbose_mode_ = tf; }
    inline void set_debug_protocol_fn(std::function<void(int /* S/C */, const char *, int)> fn)
    {
        debug_protocol_fn_ = fn;
    }
    inline bool check_is_need_close() { return need_close_connection_; }

    void set_timeout(int timeout);
    void set_ssl_mode(SSL_CTX *ssl_ctx);
    void set_tls_mode(SSL_CTX *ssl_ctx);
    void set_try_tls_mode(SSL_CTX *ssl_ctx);
    void set_ssl_tls(SSL_CTX *ssl_ctx, bool ssl_mode, bool tls_mode, bool try_tls_mode = false);
    void set_ehlo_key(const char *key)
    {
        ehlo_key_ = key;
    }
    stream &get_iostream()
    {
        return *fp_;
    }
    int connect(const char *destination, int times = 3);
    inline int connect(const std::string &destination, int times = 3)
    {
        return connect(destination.c_str(), times);
    }
    void disconnect();
    int cmd_quit();
    int do_auth_basic(const char *user, const char *password);
    inline int do_auth_basic(const std::string &user, const std::string &password)
    {
        return do_auth_basic(user.c_str(), password.c_str());
    }
    int do_auth(const char *user, const char *password);
    inline int do_auth(const std::string &user, const std::string &password)
    {
        return do_auth(user.c_str(), password.c_str());
    }
    int cmd_mail_from(const std::string &from, const std::string &params = "");
    int cmd_rcpt_to(const std::string &to);
    int cmd_data();
    int do_quick_send_raw_data_once(const char *data, size_t size, std::function<void(int64_t send_bytes)> process_callback = nullptr);
    inline int do_quick_send_raw_data_once(const std::string &data, std::function<void(int64_t send_bytes)> process_callback = nullptr)
    {
        return do_quick_send_raw_data_once(data.c_str(), data.size(), process_callback);
    }
    int do_quick_send_file_once(const std::string &filename, std::function<void(int64_t send_bytes)> process_callback = nullptr);
    int cmd_data_end();
    int cmd_rset();
    int cmd_noop();
    inline const std::string &get_last_response_line()
    {
        return last_response_line_;
    }

protected:
    int fp_connect(const char *destination, int times);
    int welcome();
    int do_STARTTLS();
    int cmd_ehlo(const char *key);
    inline int cmd_ehlo(const std::string &key)
    {
        return cmd_ehlo(key.c_str());
    }
    int cmd_helo(const char *key);
    inline int cmd_helo(const std::string &key)
    {
        return cmd_helo(key.c_str());
    }
    int simple_quick_cmd(const std::string &cmd, const std::string &debug_info = "");

protected:
    std::vector<std::string> capability_;
    std::string blank_;
    stream *fp_{nullptr};
    SSL_CTX *ssl_ctx_{NULL};
    bool ssl_mode_{false};
    bool tls_mode_{false};
    bool try_tls_mode_{false};
    std::function<void(int, const char *, int)> debug_protocol_fn_{0};
    std::vector<std::string> ehlo_response_lines_;
    std::string last_response_line_;
    std::string ehlo_key_{"zcc-smtp"};

protected:
    bool debug_mode_{false};
    bool verbose_mode_{false};
    bool need_close_connection_{false};
    bool connected_{false};
    bool opened_{false};
    bool authed_{false};
    bool ssl_flag_{false};
};

zcc_namespace_end;
#pragma pack(pop)
#endif // __cplusplus

#endif // ZCC_LIB_INCLUDE_SMTP___

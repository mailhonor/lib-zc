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
#include <set>
#include "./zcc_stdlib.h"
#include "./zcc_stream.h"
#include "./zcc_mail.h"

#ifdef __cplusplus
#pragma pack(push, 4)
zcc_namespace_begin;

class ZCC_LIB_API smtp_client
{
public:
    smtp_client();
    smtp_client(stream *third_stream);
    virtual ~smtp_client();
    inline void set_debug_mode(bool tf = true) { debug_mode_ = tf; }
    inline void set_verbose_mode(bool tf = true) { verbose_mode_ = tf; }
    inline void set_debug_protocol_mode(bool tf = true) { debug_protocol_mode_ = tf; }
    inline void set_debug_protocol_fn(std::function<void(mail_protocol_client_or_server, const char *, int)> fn)
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
    int do_quick_send_file_once(const std::string &pathname, std::function<void(int64_t send_bytes)> process_callback = nullptr);
    int do_quick_send_once(const char *raw_data, size_t size, const std::string &pathname, std::function<void(int64_t send_bytes)> process_callback = nullptr);
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
    std::function<void(mail_protocol_client_or_server, const char *, int)> debug_protocol_fn_{0};
    std::vector<std::string> ehlo_response_lines_;
    std::string last_response_line_;
    std::string ehlo_key_{"zcc-smtp"};

protected:
    bool debug_mode_{false};
    bool verbose_mode_{false};
    bool debug_protocol_mode_{false};
    bool need_close_connection_{false};
    bool connected_{false};
    bool opened_{false};
    bool authed_{false};
    bool ssl_flag_{false};
};

enum smtp_client_stage
{
    smtp_client_stage_init,
    smtp_client_stage_connect,
    smtp_client_stage_auth,
    smtp_client_stage_mail_from,
    smtp_client_stage_rcpt_to,
    smtp_client_stage_data,
    smtp_client_stage_quit,
    smtp_client_stage_done,
};

class ZCC_LIB_API smtp_client_quick_send
{
public:
    smtp_client_quick_send();
    virtual ~smtp_client_quick_send();
    inline smtp_client_stage &get_stage() { return stage_; }
    inline const std::string &get_last_response_line() { return last_response_line_; }
    inline const std::vector<mail_protocol_line_record> &get_protocol_logs() { return protocol_logs_; }
    inline const std::string get_error_rcpt() { return error_rcpt_; }

public:
    bool send_once_by_data(const char *data, int len = -1);
    inline bool send_once_by_data(const std::string &data)
    {
        return send_once_by_data(data.c_str(), (int)data.size());
    }
    bool send_once_by_file(const std::string &pathname);
    inline void set_debug_mode(bool tf = true) { debug_mode_ = tf; }
    inline void set_verbose_mode(bool tf = true) { verbose_mode_ = tf; }
    inline void set_debug_protocol_mode(bool tf = true) { debug_protocol_mode_ = tf; }
    inline void set_record_protocol_logs(bool tf = true) { record_protocol_logs_ = tf; }
    void set_connect_times(int times);
    void set_timeout(int timeout);
    void set_ssl_mode(SSL_CTX *ssl_ctx);
    void set_tls_mode(SSL_CTX *ssl_ctx);
    void set_try_tls_mode(SSL_CTX *ssl_ctx);
    void set_ssl_tls(SSL_CTX *ssl_ctx, bool ssl_mode, bool tls_mode, bool try_tls_mode = false);
    void set_process_callback(std::function<void(smtp_client_stage stage, int process_percentage_value)> callback);
    void set_server(const std::string &server, int port = 25);
    void set_username_password(const std::string &username, const std::string &password);
    void set_mail_from(const std::string &mail_from);
    void add_rcpt_to(const std::string &rcpt_to);
    void add_rcpt_to(const std::set<std::string> &rcpt_tos);

protected:
    bool send_once_inner(const char *data, int len, const std::string &pathname);
    //
    smtp_client_stage stage_{smtp_client_stage_init};
    std::string last_response_line_;
    std::vector<mail_protocol_line_record> protocol_logs_;
    //
    bool debug_mode_{false};
    bool verbose_mode_{false};
    bool debug_protocol_mode_{false};
    bool record_protocol_logs_{false};
    bool ssl_mode_{false};
    bool tls_mode_{false};
    bool try_tls_mode_{false};
    SSL_CTX *ssl_ctx_{NULL};
    int timeout_{10};
    int connect_times_{3};
    std::function<void(smtp_client_stage tage, int process_percentage_value)> process_callback_{nullptr};
    std::string destination_;
    std::string username_;
    std::string password_;
    std::string mail_from_;
    std::vector<std::string> rcpt_to_;
    std::set<std::string> rcpt_to_unique_;
    std::string error_rcpt_;
};

zcc_namespace_end;
#pragma pack(pop)
#endif // __cplusplus

#endif // ZCC_LIB_INCLUDE_SMTP___

/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2026-01-27
 * ================================
 */

#pragma once

#ifndef ZCC_LIB_INCLUDE_MAIL___
#define ZCC_LIB_INCLUDE_MAIL___

#include "./zcc_stdlib.h"
#include "./zcc_openssl.h"

#ifdef __cplusplus
#pragma pack(push, 4)
zcc_namespace_begin;

enum mail_protocol_client_or_server
{
    mail_protocol_client = 'C',
    mail_protocol_server = 'S',
};

struct ZCC_LIB_API mail_protocol_line_record
{
    mail_protocol_client_or_server client_or_server;
    std::string line;
};

enum mail_account_checker_error_stage
{
    error_none = 0,
    error_resolve,
    error_connection,
    error_auth,
};

class ZCC_LIB_API mail_account_checker_result
{
    friend class mail_account_checker;

public:
    mail_account_checker_result();
    ~mail_account_checker_result();
    void reset();

public:
    std::string protocol_;
    std::string connected_destination_;
    std::vector<mail_protocol_line_record> protocol_log_;
    std::string last_response_line_;
    mail_account_checker_error_stage error_stage_{error_none};
};

class ZCC_LIB_API mail_account_checker
{
public:
public:
    mail_account_checker();
    virtual ~mail_account_checker();
    // return true: success, false: failed
    virtual bool do_actual_checking_work(mail_account_checker_result &result) = 0;
    bool check(mail_account_checker_result &result);
    bool check();
    inline const mail_account_checker_result &get_result() const { return result_; }

public:
    virtual void set_server(const std::string &server, int port = -1);
    void set_username_password(const std::string &username, const std::string &password);
    void set_ssl_mode(SSL_CTX *ssl_ctx);
    void set_tls_mode(SSL_CTX *ssl_ctx);
    void set_try_tls_mode(SSL_CTX *ssl_ctx);
    void set_ssl_tls(SSL_CTX *ssl_ctx, bool ssl_mode, bool tls_mode, bool try_tls_mode = false);
    void set_timeout(int timeout);
    void set_connect_times(int times);
    void set_record_protocol(bool tf = true);
    void set_debug_mode(bool tf);
    void set_verbose_mode(bool tf);
    void set_debug_protocol_mode(bool tf);

protected:
    bool check_host_dns();

protected:
    std::string host_;
    int port_{0};
    std::string username_;
    std::string password_;
    int timeout_{10};
    int connect_times_{3};
    SSL_CTX *ssl_ctx_{nullptr};
    bool ssl_mode_{false};
    bool tls_mode_{false};
    bool try_tls_mode_{false};
    bool record_protocol_{false};
    bool debug_mode_{false};
    bool verbose_mode_{false};
    bool debug_protocol_mode_{false};
    mail_account_checker_result result_;
};

//
class ZCC_LIB_API smtp_account_checker : public mail_account_checker
{
public:
    smtp_account_checker();
    ~smtp_account_checker();
    inline void set_server(const std::string &server, int port = 25)
    {
        mail_account_checker::set_server(server, port);
    }
    bool do_actual_checking_work(mail_account_checker_result &result);
};

//
class ZCC_LIB_API imap_account_checker : public mail_account_checker
{
public:
    imap_account_checker();
    ~imap_account_checker();
    inline void set_server(const std::string &server, int port = 143)
    {
        mail_account_checker::set_server(server, port);
    }
    bool do_actual_checking_work(mail_account_checker_result &result);
};

//
class ZCC_LIB_API pop_account_checker : public mail_account_checker
{
public:
    pop_account_checker();
    ~pop_account_checker();
    inline void set_server(const std::string &server, int port = 110)
    {
        mail_account_checker::set_server(server, port);
    }
    bool do_actual_checking_work(mail_account_checker_result &result);
};

zcc_namespace_end;
#pragma pack(pop)
#endif // __cplusplus

#endif // ZCC_LIB_INCLUDE_MAIL___

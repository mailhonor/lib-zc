/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2017-02-18
 * ================================
 */

#pragma once

#ifndef ZCC_LIB_INCLUDE_POP___
#define ZCC_LIB_INCLUDE_POP___

#include <functional>
#include "./zcc_stdlib.h"
#include "./zcc_stream.h"

#ifdef __cplusplus
#pragma pack(push, 4)
zcc_namespace_begin;

class pop_client
{
public:
    pop_client();
    pop_client(stream *third_stream);
    virtual ~pop_client();
    inline void set_debug_mode(bool tf = true) { debug_mode_ = tf; }
    inline void set_verbose_mode(bool tf = true) { verbose_mode_ = tf; }
    inline void set_debug_protocol_fn(std::function<void(int /* S/C */, const char *, int)> fn)
    {
        debug_protocol_fn_ = fn;
    }
    inline bool check_is_need_close() { return need_close_connection_; }

    void set_timeout(int timeout);
    void set_ssl_tls(SSL_CTX *ssl_ctx, bool ssl_mode, bool tls_mode, bool tls_try_mode = false);
    virtual inline void set_ssl_tls(void *ssl_ctx, bool ssl_mode, bool tls_mode, bool tls_try_mode = false)
    {
        set_ssl_tls((SSL_CTX *)ssl_ctx, ssl_mode, tls_mode, tls_try_mode);
    }
    stream &get_iostream() { return *fp_; }
    int connect(const char *destination, int times = 3);
    void disconnect();
    void close();
    int auth_basic(const char *user, const char *password);
    int auth_apop(const char *user, const char *password);
    int auth_auto(const char *user, const char *password);
    inline const std::string &get_banner_apop_id() { return banner_apop_id_; }
    int cmd_capa(std::vector<std::string> &capability);
    int cmd_capa();
    int cmd_stat(uint64_t &count, uint64_t &size_sum);
    int cmd_list(std::vector<std::pair<int, uint64_t>> &msg_number_sizes);
    int cmd_list(std::vector<int> &msg_numbers);
    int cmd_list(int msg_number, uint64_t &size);
    int cmd_uidl(std::map<std::string, int> &result);
    int cmd_uidl(int msg_number, std::string &unique_id);
    int cmd_retr(int msg_number, std::string &data);
    int cmd_retr(int msg_number, FILE *fp);
    int cmd_top(int msg_number, std::string &data, int extra_line_count = 0);
    int cmd_dele(int msg_number);
    int cmd_rset();
    int cmd_noop();
    int cmd_quit();
    const std::string &get_capability(const char *key_lowercase);
    const std::vector<std::string> &get_capability();

public:
    int simple_quick_cmd(const std::string &cmd);
    int simple_quick_cmd(const std::string &cmd, std::string &response);
    pop_client &fp_append(const char *s, int slen = -1);
    pop_client &fp_append(const std::string &s);
    int fp_readn(std::string &str, int strict_len);
    int fp_readn(void *mem, int strict_len);
    int fp_read_delimiter(void *mem, int delimiter, int max_len);
    int fp_read_delimiter(std::string &str, int delimiter, int max_len);
    inline int fp_gets(std::string &str, int max_len) { return fp_read_delimiter(str, '\n', max_len); }
    inline int fp_gets(void *mem, int max_len) { return fp_read_delimiter(mem, '\n', max_len); }

protected:
    int fp_connect(const char *destination, int times);
    int welcome();
    int do_STLS();

protected:
    std::string banner_apop_id_;
    std::vector<std::string> capability_;
    std::string blank_;
    stream *fp_{nullptr};
    SSL_CTX *ssl_ctx_{NULL};
    bool ssl_mode_{false};
    bool tls_mode_{false};
    bool tls_try_mode_{false};
    std::function<void(int, const char *, int)> debug_protocol_fn_{0};

protected:
    int _get_msg_data(const std::string &cmd, FILE *fp, std::string *data);
    bool debug_mode_{false};
    bool verbose_mode_{false};
    bool need_close_connection_{false};
    bool connected_{false};
    bool opened_{false};
    bool quited_{false};
    bool authed_{false};
    bool ssl_flag_{false};
    bool third_stream_mode_{false};
    int capa_before_auth_{-1};
    int capa_after_auth_{-1};
};

zcc_namespace_end;
#pragma pack(pop)
#endif // __cplusplus

#endif // ZCC_LIB_INCLUDE_POP___

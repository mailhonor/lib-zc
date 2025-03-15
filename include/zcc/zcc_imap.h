/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2017-02-18
 * ================================
 */

#pragma once

#ifndef ZCC_LIB_INCLUDE_IMAP___
#define ZCC_LIB_INCLUDE_IMAP___

#include <functional>
#include <set>
#include "./zcc_stdlib.h"
#include "./zcc_stream.h"

#ifdef __cplusplus
#pragma pack(push, 4)
zcc_namespace_begin;

class imap_client
{
public:
    enum result_onb
    {
        ok = 0,
        no,
        bad
    };

    class response_tokens
    {
    public:
        response_tokens();
        ~response_tokens();
        void reset();
        std::vector<std::string> token_vector_;
        std::string first_line_;
    };

    class uidplus_result
    {
    public:
        uidplus_result();
        ~uidplus_result();
        void reset();
        int uidvalidity_;
        int uid_;
    };

    class status_result
    {
    public:
        status_result();
        ~status_result();
        void reset();
        int messages_;
        int recent_;
        int uidnext_;
        int uidvalidity_;
        int unseen_;
    };

    class folder_result
    {
    public:
        folder_result();
        ~folder_result();
        void reset();
        const char *get_special_use();
        void debug_show();

    public:
        std::string name_;
        std::set<std::string> attrs_;
        int hierarchy_separator_;
        bool noinferiors_;
        bool noselect_;
        bool inbox_;
        bool drafts_;
        bool junk_;
        bool trash_;
        bool sent_;
        // LSUB 得到的结果
        bool subscribed_;
        // STATUS 得到的结果
        status_result *status_;
    };

    typedef std::map<std::string /* imap pahtname */, folder_result> folder_list_result;

    class select_result
    {
    public:
        select_result();
        ~select_result();
        void reset();

    public:
        std::string flags_;
        int exists_;
        int recent_;
        int uidvalidity_;
        int uidnext_;
        bool readonly_;
    };

    class mail_flags
    {
    public:
        mail_flags();
        ~mail_flags();
        void reset();
        std::string to_string();
        bool answered_;
        bool seen_;
        bool draft_;
        bool flagged_;
        bool deleted_;
        bool recent_;
        bool forwarded_;
    };

    typedef std::map<int, mail_flags> mail_list_result;

    class append_session
    {
    public:
        append_session();
        ~append_session();
        void reset();
        std::string to_folder_;
        mail_flags flags_;
        int64_t time_;
        uint64_t mail_size_;
    };

public:
    static std::string imap_utf7_to_utf8(const char *str, int slen);
    static inline std::string imap_utf7_to_utf8(const std::string &str)
    {
        return imap_client::imap_utf7_to_utf8(str.c_str(), (int)str.size());
    }
    static std::string utf8_to_imap_utf7(const char *str, int slen);
    static inline std::string utf8_to_imap_utf7(const std::string &str)
    {
        return imap_client::utf8_to_imap_utf7(str.c_str(), (int)str.size());
    }
    static std::string escape_string(const char *s, int len = -1);
    static inline std::string escape_string(const std::string &s)
    {
        return escape_string(s.c_str(), (int)(s.size()));
    }

public:
    imap_client();
    imap_client(stream *third_stream, bool auto_release_third_stream = true);
    virtual ~imap_client();
    inline bool is_no() { return (ok_no_bad_ == result_onb::no); }
    inline bool is_bad() { return (ok_no_bad_ == result_onb::bad); }
    inline void set_debug_mode(bool tf = true) { debug_mode_ = tf; }
    inline void set_verbose_mode(bool tf = true) { verbose_mode_ = tf; }
    inline void set_simple_line_mode(bool tf = true) { simple_line_mode_ = tf; }
    inline void set_debug_protocol_fn(std::function<void(int /* S/C */, const char *, int)> fn)
    {
        debug_protocol_fn_ = fn;
    }
    inline bool check_is_need_close() { return need_close_connection_; }

    void set_simple_line_length_limit(int limit);
    void set_timeout(int timeout);
    void set_ssl_tls(SSL_CTX *ssl_ctx, bool ssl_mode, bool tls_mode, bool tls_try_mode = false);
    virtual inline void set_ssl_tls(void *ssl_ctx, bool ssl_mode, bool tls_mode, bool tls_try_mode = false)
    {
        set_ssl_tls((SSL_CTX *)ssl_ctx, ssl_mode, tls_mode, tls_try_mode);
    }
    stream &get_stream() { return *fp_; }
    int connect(const char *destination, int times = 1);
    void disconnect();
    int do_auth(const char *user, const char *password);
    int cmd_logout();
    int cmd_capability(bool force = false);
    int cmd_id(const char *id = "");
    int cmd_list(folder_list_result &folder_list);
    int cmd_lsub(folder_list_result &folder_list);
    int cmd_select(select_result &ser, const char *folder_name_imap);
    inline int cmd_select(select_result &ser, const std::string &folder_name_imap)
    {
        return cmd_select(ser, folder_name_imap.c_str());
    }
    int cmd_select(const char *folder_name_imap);
    inline int cmd_select(const std::string &folder_name_imap) { return cmd_select(folder_name_imap.c_str()); }
    int cmd_select_forced(select_result &ser, const char *folder_name_imap);
    inline int cmd_select_forced(select_result &ser, const std::string &folder_name_imap)
    {
        return cmd_select_forced(ser, folder_name_imap.c_str());
    }
    int cmd_select_forced(const char *folder_name_imap);
    inline int cmd_select_forced(const std::string &folder_name_imap) { return cmd_select_forced(folder_name_imap.c_str()); }
    int cmd_status(status_result &ser, const char *folder_name_imap);
    inline int cmd_status(status_result &ser, const std::string &folder_name_imap)
    {
        return cmd_status(ser, folder_name_imap.c_str());
    }
    int cmd_search(std::vector<int> &uid_vector, const std::string &search_string);
    int cmd_search_all(std::vector<int> &uid_vector);
    int cmd_search_unseen(std::vector<int> &uid_vector);
    int cmd_search_answered(std::vector<int> &uid_vector);
    int cmd_search_deleted(std::vector<int> &uid_vector);
    int cmd_search_draft(std::vector<int> &uid_vector);
    int cmd_search_flagged(std::vector<int> &uid_vector);
    int get_mail_list(mail_list_result &mail_list);
    int get_message(FILE *dest_fp, mail_flags &flags, int uid);
    int get_all_folder_info(folder_list_result &folder_list);
    int cmd_create(const std::string &pathname_utf7);
    int cmd_rename(const std::string &from_pathname_utf7, const std::string &to_pathname_utf7);
    int cmd_subscribe(const std::string &pathname_utf7, bool tf = true);
    int cmd_delete(const std::string &pathname_utf7);
    inline int cmd_unsubscribe(const std::string &pathname_utf7) { return cmd_subscribe(pathname_utf7, false); }
    int delete_mail(int uid);
    int cmd_move(uidplus_result &result, int from_uid, const char *to_folder_name_imap);
    inline int cmd_move(uidplus_result &result, int from_uid, const std::string &to_folder_name_imap)
    {
        return cmd_move(result, from_uid, to_folder_name_imap.c_str());
    }
    int cmd_copy(uidplus_result &result, int from_uid, const char *to_folder_name_imap);
    inline int cmd_copy(uidplus_result &result, int from_uid, const std::string &to_folder_name_imap)
    {
        return cmd_copy(result, from_uid, to_folder_name_imap.c_str());
    }
    int cmd_store(const char *uids, bool plus_or_minus, mail_flags &flags);
    int cmd_store(int uid, bool plus_or_minus, mail_flags &flags);
    int cmd_store(const char *uids, bool plus_or_minus, const char *flags);
    int cmd_store_deleted_flag(const char *uids, bool plus_or_minus);
    int cmd_store_deleted_flag(int uid, bool plus_or_minus);
    int cmd_expunge();
    int cmd_append_prepare_protocol(append_session &append);
    int cmd_append_over(uidplus_result &result);
    int append_file(uidplus_result &result, append_session &append, const char *filename);
    int append_data(uidplus_result &result, append_session &append, const void *data, uint64_t dlen);
    int get_capability(const char *key_lowercase);
    int get_capability_cached(const char *key, char *cache);
    inline int get_capability_move() { return get_capability_cached("move", &capability_move_); }
    inline int get_capability_uidplus() { return get_capability_cached("uidplus", &capability_move_); }
    inline int get_capability_idle() { return get_capability_cached("idle", &capability_idle_); }
    std::string &get_capability() { return capability_; }
    int check_new_message_by_noop(const std::string &mbox = "INBOX");
    int idle_beign(const std::string &mbox = "INBOX");
    bool idle_check_new_message(int wait_second = 0);
    int idle_end();

public:
    imap_client &fp_append(const char *s, int slen = -1);
    imap_client &fp_append(const std::string &s);
    int fp_readn(std::string &str, int strict_len);
    int fp_readn(void *mem, int strict_len);
    int fp_read_delimiter(void *mem, int delimiter, int max_len);
    int fp_read_delimiter(std::string &str, int delimiter, int max_len);
    inline int fp_gets(std::string &str, int max_len) { return fp_read_delimiter(str, '\n', max_len); }
    inline int fp_gets(void *mem, int max_len) { return fp_read_delimiter(mem, '\n', max_len); }
    int parse_imap_result(const char *key_line);
    int parse_imap_result(char tag, const char *line);
    inline bool parse_imap_result(char tag, const std::string &line) { return parse_imap_result(tag, line.c_str()); }
    int parse_imap_result(char tag, const response_tokens &tokens);
    int read_big_data(FILE *dest_fp, std::string &raw_content, int extra_length);
    int read_response_tokens_oneline(response_tokens &tokens, int &extra_length);
    int read_response_tokens(response_tokens &tokens);
    int ignore_left_token(int extra_length);

protected:
    int fp_connect(const char *destination, int times);
    int welcome();
    int do_quick_cmd(const std::string &cmd);
    int do_startTLS();

private:
    int _cmd_list(folder_list_result &folder_list, bool list_or_lsub);
    int _cmd_search_flag(std::vector<int> &uid_vector, const char *flag_token);

protected:
    stream *fp_{nullptr};
    SSL_CTX *ssl_ctx_{NULL};
    bool ssl_mode_{false};
    bool tls_mode_{false};
    bool tls_try_mode_{false};
    bool auto_release_third_stream_{false};
    std::string capability_;
    std::string last_selected_;
    int simple_line_length_limit_{102400};
    std::function<void(int, const char *, int)> debug_protocol_fn_{0};

protected:
    result_onb ok_no_bad_{ok};
    bool debug_mode_{false};
    bool verbose_mode_{false};
    bool simple_line_mode_{false};
    bool opened_{false};
    bool connected_{false};
    bool logout_{false};
    bool ssl_flag_{false};
    bool need_close_connection_{false};
    bool tmp_verbose_mode_{false};
    bool third_stream_mode_{false};
    int tmp_verbose_line_{0};
    bool auth_capability_{false};
    bool capability_clear_flag_{false};
    char capability_move_{0};
    char capability_uidplus_{0};
    char capability_idle_{0};
};

zcc_namespace_end;
#pragma pack(pop)
#endif // __cplusplus

#endif // ZCC_LIB_INCLUDE_IMAP___

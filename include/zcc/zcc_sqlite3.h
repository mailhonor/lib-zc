/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2025-01-01
 * ================================
 */

#pragma once

#ifndef ZCC_LIB_INCLUDE_SQLITE3___
#define ZCC_LIB_INCLUDE_SQLITE3___

#include "./zcc_stdlib.h"
#pragma pack(push, 4)
#ifdef __cplusplus

extern "C"
{
    struct zcc_sqlite3;
    struct zcc_sqlite3_stmt;
};

zcc_namespace_begin;

class sqlite3_min_single;
class sqlite3_mini_client;
class ZCC_LIB_API sqlite3_mini_stmt
{
    friend class sqlite3_mini_client;

public:
    sqlite3_mini_stmt();
    sqlite3_mini_stmt(sqlite3_mini_client *client);
    ~sqlite3_mini_stmt();
    void close();
    void reset_result();
    void bind_client(sqlite3_mini_client *client);
    bool prepare(const char *sql, int slen = -1);
    inline bool prepare(const std::string &sql)
    {
        return prepare(sql.c_str(), (int)sql.size());
    }
#ifdef _WIN64
    bool prepare_printf(const char *sql_fmt, ...);
#else  // _WIN64
    bool __attribute__((format(gnu_printf, 2, 3))) prepare_printf(const char *sql_fmt, ...);
#endif // _WIN64

    bool bind_blob(int sn, const void *data, uint64_t data_len);
    bool bind(int sn, const char *str, int slen = -1);
    bool bind(int sn, const std::string &str);
    bool bind(int sn, int64_t val);
    bool bind(int sn, int val);
    bool bind(int sn, double val);
    bool bind_blob(const std::string &name, const void *data, uint64_t data_len);
    bool bind(const std::string &name, const char *str, int slen = -1);
    bool bind(const std::string &name, const std::string &str);
    bool bind(const std::string &name, int64_t val);
    bool bind(const std::string &name, int val);
    bool bind(const std::string &name, double val);
    bool step();
    bool step(bool &row_flag);
    std::string column_string(int sn);
    ZCC_DEPRECATED inline const char *column_text(int sn) { return column_text_inner(sn); }
    ZCC_DEPRECATED inline const void *column_blob(int sn) { return column_blob_inner(sn); }
    ZCC_DEPRECATED inline int column_bytes(int sn) { return column_bytes_inner(sn); }
    inline const char *column_text_data(int sn) { return column_text_inner(sn); }
    inline int column_text_length(int sn) { return column_bytes_inner(sn); }
    inline const void *column_blob_data(int sn) { return column_blob_inner(sn); }
    inline int column_blob_length(int sn) { return column_bytes_inner(sn); }
    int column_int(int sn);
    int64_t column_long(int sn);
    double column_double(int sn);
    bool column_bool(int sn, bool def = false);

protected:
    sqlite3_mini_client *client_{0};
    struct zcc_sqlite3_stmt *stmt_{0};
    std::string sql_;
    std::map<std::string, int> parameters_;
    bool need_reset_{false};

private:
    const char *column_text_inner(int sn);
    const void *column_blob_inner(int sn);
    int column_bytes_inner(int sn);
};

class ZCC_LIB_API sqlite3_mini_client
{
    friend sqlite3_mini_stmt;

public:
    static void env_init();
    static void env_fini();
    static void env_lock();
    static void env_unlock();
    static std::string escape_string_DONOT_USE(const char *data);
    inline static std::string escape_string_DONOT_USE(const std::string &data)
    {
        return escape_string_DONOT_USE(data.c_str());
    }

public:
    sqlite3_mini_client();
    virtual ~sqlite3_mini_client();
    inline const std::string &get_db_pathname() { return db_pathname_; }
    inline const std::string &get_last_error() { return last_error_; }
    bool close();
    bool open(const std::string &db_pathname);
    bool begin_with_type(const char *type);
    inline bool begin()
    {
        return begin_with_type("");
    }
    inline bool begin_EXCLUSIVE()
    {
        return begin_with_type("EXCLUSIVE");
    }
    inline bool begin_SHARED()
    {
        return begin_with_type("DEFERRED");
    }

    bool commit();
    bool rollback();
    bool exec(const std::string &sql);
    bool transaction_exec(const char *sqls[]);
    bool transaction_exec(const std::vector<std::string> &sqls);
    bool transaction_exec(const std::list<std::string> &sqls);
    bool transaction_exec(const std::string &sql);
    //
    inline void set_debug_mode(bool tf = true) { debug_mode_ = tf; }

protected:
    void check_not_released_stmt();
    std::string db_pathname_;
    struct zcc_sqlite3 *handler_{0};
    std::string last_exec_sql_debug_;
    std::string last_error_;
    bool debug_mode_{false};

protected:
    void lock_for_execute();
    void unlock_for_execute();
    void lock_for_transaction();
    void unlock_for_transaction();

private:
    sqlite3_min_single *single_open_{nullptr};
    int begin_depth_{0};
    bool transaction_lock_mode_{false};
};

zcc_namespace_end;
#endif // __cplusplus
#pragma pack(pop)
#endif // ZCC_LIB_INCLUDE_SQLITE3___

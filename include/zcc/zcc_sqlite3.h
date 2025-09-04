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
#include <sqlite3.h>

#pragma pack(push, 4)
#ifdef __cplusplus
zcc_namespace_begin;

class sqlite3_mini_client;

class sqlite3_mini_stmt
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
    bool bind_blob(const char *name, const void *data, uint64_t data_len);
    bool bind_blob(const std::string &name, const void *data, uint64_t data_len);
    bool bind(const std::string &name, const char *str, int slen = -1);
    bool bind(const std::string &name, const std::string &str);
    bool bind(const std::string &name, int64_t val);
    bool bind(const std::string &name, int val);
    bool bind(const std::string &name, double val);
    bool step();
    bool step(bool &row_flag);
    std::string column_string(int sn);
    const char *column_text(int sn);
    const void *column_blob(int sn);
    int column_bytes(int sn);
    int column_int(int sn);
    int64_t column_long(int sn);
    double column_double(int sn);
    bool column_bool(int sn, bool def = false);

protected:
    sqlite3_mini_client *client_{0};
    sqlite3_stmt *stmt_{0};
    std::string sql_;
    std::map<std::string, int> parameters_;
    bool need_reset_{false};
};

class sqlite3_mini_client
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
    sqlite3 *handler_{0};
    std::string last_exec_sql_debug_;
    std::string last_error_;
    bool debug_mode_{false};

private:
    std::string db_pathname_;
    void *single_open_{0};
    int begin_depth_{0};
};

zcc_namespace_end;
#endif // __cplusplus
#pragma pack(pop)
#endif // ZCC_LIB_INCLUDE_SQLITE3___

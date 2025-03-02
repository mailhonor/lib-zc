/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2025-01-01
 * ================================
 */

#ifdef ZCC_USE_SQLITE3__

#include "zcc/zcc_sqlite3.h"

#define _client_stmt_null_warning()         \
    if (!client_)                           \
    {                                       \
        zcc_error("sqlite client is null"); \
        return false;                       \
    }                                       \
    if (!stmt_)                             \
    {                                       \
        zcc_error("sqlite stmt is null");   \
        return false;                       \
    }

#define _bind_sn_warning(sn)                            \
    {                                                   \
        if (sn < 1)                                     \
        {                                               \
            zcc_fatal("sn must > 0; %s", sql_.c_str()); \
        }                                               \
    }

zcc_namespace_begin;

sqlite3_mini_stmt::sqlite3_mini_stmt()
{
    client_ = 0;
    stmt_ = 0;
    need_reset_ = false;
}

sqlite3_mini_stmt::sqlite3_mini_stmt(sqlite3_mini_client *client)
{
    client_ = 0;
    stmt_ = 0;
    bind_client(client);
}

sqlite3_mini_stmt::~sqlite3_mini_stmt()
{
    close();
}

void sqlite3_mini_stmt::close()
{
    if (stmt_)
    {
        sqlite3_finalize(stmt_);
        stmt_ = 0;
    }
}

void sqlite3_mini_stmt::reset_result()
{
    if (stmt_)
    {
        sqlite3_reset(stmt_);
        need_reset_ = false;
    }
}

void sqlite3_mini_stmt::bind_client(sqlite3_mini_client *client)
{
    if (stmt_)
    {
        sqlite3_finalize(stmt_);
        stmt_ = 0;
        need_reset_ = false;
    }
    client_ = client;
}

bool sqlite3_mini_stmt::prepare(const char *sql, int slen)
{
    int ret;
    if (!client_)
    {
        zcc_error("sqlite = null");
        return false;
    }
    if (slen < 0)
    {
        slen = std::strlen(sql);
    }
    if (stmt_)
    {
        sqlite3_finalize(stmt_);
        stmt_ = 0;
        need_reset_ = false;
    }
    if (client_->debug_mode_)
    {
        client_->last_exec_sql_debug_ = ", last_stmt: ";
        client_->last_exec_sql_debug_.append(sql, slen);
    }
    if ((ret = sqlite3_prepare_v2(client_->handler_, sql, slen, &stmt_, 0)) != SQLITE_OK)
    {
        zcc_error("illegal sqlite3 sql: %s, %s, errocode = %d, %d", sql, sqlite3_errmsg(client_->handler_), ret, client_->handler_ ? 1 : 0);
        return false;
    }
    sql_.clear();
    sql_.append(sql, slen);

    parameters_.clear();
    int count = sqlite3_bind_parameter_count(stmt_);
    for (int i = 1; i <= count; i++)
    {
        const char *name = sqlite3_bind_parameter_name(stmt_, i);
        if (name && name[0])
        {
            parameters_[name] = i;
        }
    }
    return true;
}

bool sqlite3_mini_stmt::prepare_printf(const char *sql_fmt, ...)
{
    va_list ap;
    char buf[4096 + 1];
    va_start(ap, sql_fmt);
    std::vsnprintf(buf, 4096, sql_fmt, ap);
    va_end(ap);
    return prepare(buf, -1);
}

static void _bind_free(void *s)
{
    if (s)
    {
        delete (char *)s;
    }
}

bool sqlite3_mini_stmt::bind_blob(int sn, const void *data, uint64_t data_len)
{
    _client_stmt_null_warning();
    _bind_sn_warning(sn);
    if (need_reset_)
    {
        reset_result();
    }
    char *new_data = new char[data_len + 1];
    std::memcpy(new_data, data, data_len);
    new_data[data_len] = 0;
    if (sqlite3_bind_blob64(stmt_, sn, new_data, data_len, _bind_free) != SQLITE_OK)
    {
        zcc_error("bind sqlite3(sn_%d): %s", sn, sqlite3_errmsg(client_->handler_));
        delete new_data;
        return false;
    }
    return true;
}

bool sqlite3_mini_stmt::bind(int sn, const char *str, int slen)
{
    _client_stmt_null_warning();
    _bind_sn_warning(sn);
    if (need_reset_)
    {
        reset_result();
    }
    if (slen < 0)
    {
        slen = strlen(str);
    }
    char *new_str = new char[slen + 1];
    std::memcpy(new_str, str, slen);
    new_str[slen] = 0;
    if (sqlite3_bind_text(stmt_, sn, new_str, slen, _bind_free) != SQLITE_OK)
    {
        zcc_error("bind sqlite3(sn_%d): %s, %s", sn, sql_.c_str(), sqlite3_errmsg(client_->handler_));
        return false;
    }
    return true;
}

bool sqlite3_mini_stmt::bind(int sn, const std::string &str)
{
    return bind(sn, str.c_str(), (int)str.size());
}

bool sqlite3_mini_stmt::bind(int sn, int64_t val)
{
    _client_stmt_null_warning();
    _bind_sn_warning(sn);
    if (need_reset_)
    {
        reset_result();
    }
    if (sqlite3_bind_int64(stmt_, sn, val) != SQLITE_OK)
    {
        zcc_error("bind sqlite3(sn_%d): %s, %s", sn, sql_.c_str(), sqlite3_errmsg(client_->handler_));
        return false;
    }
    return true;
}

bool sqlite3_mini_stmt::bind(int sn, int val)
{
    _client_stmt_null_warning();
    _bind_sn_warning(sn);
    if (need_reset_)
    {
        reset_result();
    }
    if (sqlite3_bind_int(stmt_, sn, val) != SQLITE_OK)
    {
        zcc_error("bind sqlite3(sn_%d): %s, %s", sn, sql_.c_str(), sqlite3_errmsg(client_->handler_));
        return false;
    }
    return true;
}

bool sqlite3_mini_stmt::bind(int sn, double val)
{
    _client_stmt_null_warning();
    _bind_sn_warning(sn);
    if (need_reset_)
    {
        reset_result();
    }
    if (sqlite3_bind_double(stmt_, sn, val) != SQLITE_OK)
    {
        zcc_error("bind sqlite3(sn_%d): %s, %s", sn, sql_.c_str(), sqlite3_errmsg(client_->handler_));
        return false;
    }
    return true;
}

bool sqlite3_mini_stmt::bind_blob(const std::string &name, const void *data, uint64_t data_len)
{
    auto it = parameters_.find(name);
    if (it == parameters_.end())
    {
        zcc_error("bind: unknown %s", name.c_str());
        return false;
    }
    return bind_blob(it->second, data, data_len);
}

bool sqlite3_mini_stmt::bind(const std::string &name, const char *str, int slen)
{
    auto it = parameters_.find(name);
    if (it == parameters_.end())
    {
        zcc_error("bind: unknown %s", name.c_str());
        return false;
    }
    return bind(it->second, str, slen);
}

bool sqlite3_mini_stmt::bind(const std::string &name, const std::string &str)
{
    auto it = parameters_.find(name);
    if (it == parameters_.end())
    {
        zcc_error("bind: unknown %s", name.c_str());
        return false;
    }
    return bind(it->second, str);
}

bool sqlite3_mini_stmt::bind(const std::string &name, int64_t val)
{
    auto it = parameters_.find(name);
    if (it == parameters_.end())
    {
        zcc_error("bind: unknown %s", name.c_str());
        return false;
    }
    return bind(it->second, val);
}

bool sqlite3_mini_stmt::bind(const std::string &name, int val)
{
    auto it = parameters_.find(name);
    if (it == parameters_.end())
    {
        zcc_error("bind: unknown %s", name.c_str());
        return false;
    }
    return bind(it->second, val);
}

bool sqlite3_mini_stmt::bind(const std::string &name, double val)
{
    auto it = parameters_.find(name);
    if (it == parameters_.end())
    {
        zcc_error("bind: unknown %s", name.c_str());
        return false;
    }
    return bind(it->second, val);
}

bool sqlite3_mini_stmt::step()
{
    bool row_flag;
    return step(row_flag);
}

bool sqlite3_mini_stmt::step(bool &row_flag)
{
    row_flag = false;
    _client_stmt_null_warning();
    int ret = sqlite3_step(stmt_);
    need_reset_ = true;
    if (ret == SQLITE_DONE)
    {
        return true;
    }
    else if (ret == SQLITE_ROW)
    {
        row_flag = true;
        return true;
    }
    zcc_error("exec sqlite3: %s, %s", sql_.c_str(), sqlite3_errmsg(client_->handler_));
    return false;
}

std::string sqlite3_mini_stmt::column_string(int sn)
{
    std::string r;
    const char *ps = (const char *)sqlite3_column_blob(stmt_, sn);
    int len = sqlite3_column_bytes(stmt_, sn);
    if (ps && (len > 0))
    {
        r.append(ps, len);
    }
    return r;
}

const char *sqlite3_mini_stmt::column_text(int sn)
{
    return (const char *)sqlite3_column_text(stmt_, sn);
}

const void *sqlite3_mini_stmt::column_blob(int sn)
{
    return (const void *)sqlite3_column_blob(stmt_, sn);
}

int sqlite3_mini_stmt::column_bytes(int sn)
{
    return sqlite3_column_bytes(stmt_, sn);
}

int sqlite3_mini_stmt::column_int(int sn)
{
    return sqlite3_column_int(stmt_, sn);
}

int64_t sqlite3_mini_stmt::column_long(int sn)
{
    return (int64_t)sqlite3_column_int64(stmt_, sn);
}

double sqlite3_mini_stmt::column_double(int sn)
{
    return sqlite3_column_double(stmt_, sn);
}

bool sqlite3_mini_stmt::column_bool(int sn, bool def)
{
    char buf[16 + 1];
    const char *p = (const char *)sqlite3_column_text(stmt_, sn);
    int len = (int)sqlite3_column_bytes(stmt_, sn);
    if (len < 1)
    {
        return def;
    }
    if (len > 16)
    {
        len = 16;
    }
    memcpy(buf, p, len);
    buf[len] = 0;
    return str_to_bool(buf);
}

zcc_namespace_end;

#endif // ZCC_USE_SQLITE3__
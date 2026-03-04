/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2025-01-01
 * ================================
 */

#include "./sqlite3_mini_client.h"
#include <cstdlib>
#include <cstdio>
#include <mutex>
#include <thread>
#ifdef _WIN64
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <io.h>
#else // _WIN64
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#endif // _WIN64

zcc_namespace_begin;

class sqlite3_min_single
{
public:
    zcc_sqlite3 *db_p{0};
    int used{0};

    //
public:
    rwlocker thread_locker_;
    std::recursive_mutex lock_file_locker_;
    int lock_file_fd_{-1};
    int lock_file_count_{0};
};

static std::recursive_mutex _env_locker;

static bool env_init_flag = false;
void sqlite3_mini_client::env_init()
{
    if (env_init_flag)
    {
        return;
    }
    _env_locker.lock();
    if (!env_init_flag)
    {
        int ret;
        if ((ret = sqlite3_config(SQLITE_CONFIG_SERIALIZED)) != SQLITE_OK)
        {
            zcc_fatal("sqlite3 must support SQLITE_CONFIG_SERIALIZED");
        }
        if ((ret = sqlite3_initialize()) != SQLITE_OK)
        {
            zcc_fatal("sqlite3 sqlite3_initialize");
        }
        env_init_flag = true;
    }
    _env_locker.unlock();
}

void sqlite3_mini_client::env_fini()
{
    _env_locker.lock();
    if (env_init_flag)
    {
        sqlite3_shutdown();
    }
    _env_locker.unlock();
}

void sqlite3_mini_client::env_lock()
{
    _env_locker.lock();
}

void sqlite3_mini_client::env_unlock()
{
    _env_locker.unlock();
}

std::string sqlite3_mini_client::escape_string_DONOT_USE(const char *data)
{
    std::string r;
    char *e = sqlite3_mprintf("%q", data);
    r = e;
    return r;
}

sqlite3_mini_client::sqlite3_mini_client()
{
    handler_ = 0;
}

sqlite3_mini_client::~sqlite3_mini_client()
{
    close();
}

static std::map<std::string, sqlite3_min_single *> _var_all_sqlite3_dbs;

static int my_busy_handler(void *ptr, int count)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (count > 100)
    {
        return 0;
    }
    return 1;
}
bool sqlite3_mini_client::open(const std::string &db_pathname)
{
    env_init();
    sqlite3_min_single *single;
    zcc_sqlite3 *db_p = 0;
    int db_flags = 0;
    int ret;
    bool ok = false;

    // db_flags |= SQLITE_OPEN_SHAREDCACHE;
    db_flags |= SQLITE_OPEN_READWRITE;
    db_flags |= SQLITE_OPEN_CREATE;

    close();
    db_pathname_ = db_pathname;
    std::string &pathname = db_pathname_;

    ok = false;
    env_lock();
    if (_var_all_sqlite3_dbs.find(pathname) == _var_all_sqlite3_dbs.end())
    {
        if ((ret = sqlite3_open_v2(pathname.c_str(), &db_p, db_flags, 0)) != SQLITE_OK)
        {
            zcc_error("open sqlite3 db:%s, ret=%d, (%m)", pathname.c_str(), ret);
            db_p = 0;
            goto over;
        }
        if (sqlite3_busy_handler(db_p, my_busy_handler, db_p) != SQLITE_OK)
        {
            zcc_error("set sqlite3 timeout");
            goto over;
        }

        single = new sqlite3_min_single();
        single->db_p = db_p;
        single->used = 0;
        single->lock_file_fd_ = zcc::open(pathname, O_RDWR, 0);
        if (single->lock_file_fd_ == -1)
        {
            zcc_error("open lock file failed, %s", pathname.c_str());
            delete single;
            goto over;
        }
        _var_all_sqlite3_dbs[pathname] = single;
    }
    single = _var_all_sqlite3_dbs[pathname];
    db_p = single->db_p;
    single->used++;
    single_open_ = single;

    ok = true;
over:
    if (!ok)
    {
        if (db_p)
        {
            sqlite3_close(db_p);
            db_p = 0;
        }
    }
    handler_ = db_p;
    env_unlock();
    return ok;
}

bool sqlite3_mini_client::close()
{
    if (!handler_)
    {
        return true;
    }
    bool r = true;
    env_lock();

    auto it = _var_all_sqlite3_dbs.find(db_pathname_);
    if (it != _var_all_sqlite3_dbs.end())
    {
        sqlite3_min_single *single = it->second;
        single->used--;
        if (single->used == 0)
        {
            global_low_level_mutex_lock();
            int ret;
            if ((ret = sqlite3_close(handler_)) != SQLITE_OK)
            {
                r = false;
                zcc_error("close sqlite3, ret:%d", ret);
                if (ret == SQLITE_BUSY)
                {
                    zcc_error("sqlite3 is busy, maybe stmt not released");
                }
            }
            global_low_level_mutex_unlock();
            zcc::close_fd(single->lock_file_fd_);
            delete single;
            _var_all_sqlite3_dbs.erase(it);
        }
    }
    env_unlock();
    db_pathname_.clear();
    handler_ = 0;
    return r;
}

bool sqlite3_mini_client::begin_with_type(const char *type)
{
    if (!handler_)
    {
        return false;
    }
    char sql[128];
    char *errmsg = 0;
    if (begin_depth_ == 0)
    {
        // BEGIN;
        lock_for_transaction();
        sprintf(sql, "BEGIN %s;", type);
        if (sqlite3_exec(handler_, sql, NULL, NULL, &errmsg) != SQLITE_OK)
        {
            last_error_ = errmsg;
            zcc_error("exec sqlite3(BEGIN), %s%s", errmsg, last_exec_sql_debug_.c_str());
            unlock_for_transaction();
            return false;
        }
    }
    else
    {
        sprintf(sql, "SAVEPOINT _KSC_%d", begin_depth_);
        if (sqlite3_exec(handler_, sql, NULL, NULL, &errmsg) != SQLITE_OK)
        {
            last_error_ = errmsg;
            zcc_error("exec sqlite3(SAVEPOINT), %s%s", errmsg, last_exec_sql_debug_.c_str());
            return false;
        }
    }
    begin_depth_++;
    return true;
}

bool sqlite3_mini_client::commit()
{
    if (!handler_)
    {
        return false;
    }
    char *errmsg = 0;
    if (begin_depth_ == 1)
    {
        if (sqlite3_exec(handler_, "COMMIT;", NULL, NULL, &errmsg) != SQLITE_OK)
        {
            last_error_ = errmsg;
            zcc_error("exec sqlite3(COMMIT), %s", errmsg);
            return false;
        }
    }
    else
    {
        char sql[128];
        sprintf(sql, "RELEASE _KSC_%d;", begin_depth_ - 1);
        if (sqlite3_exec(handler_, sql, NULL, NULL, &errmsg) != SQLITE_OK)
        {
            zcc_error("exec sqlite3(RELEASE), %s%s", errmsg);
            return false;
        }
    }
    begin_depth_--;
    if (begin_depth_ == 0)
    {
        unlock_for_transaction();
    }

    return true;
}

bool sqlite3_mini_client::rollback()
{
    if (!handler_)
    {
        return false;
    }
    char *errmsg = 0;
    begin_depth_--;
    if (begin_depth_ == 0)
    {
        if (sqlite3_exec(handler_, "ROLLBACK;", NULL, NULL, &errmsg) != SQLITE_OK)
        {
            last_error_ = errmsg;
            zcc_error("exec sqlite3(ROLLBACK), %s", errmsg);
            unlock_for_transaction();
            return false;
        }
        unlock_for_transaction();
        return true;
    }
    else
    {
        char sql[128];
        sprintf(sql, "ROLLBACK TO SAVEPOINT _KSC_%d", begin_depth_);
        if (sqlite3_exec(handler_, sql, NULL, NULL, &errmsg) != SQLITE_OK)
        {
            last_error_ = errmsg;
            zcc_error("exec sqlite3(ROLLBACK SAVEPOINT), %s", errmsg);
            return false;
        }
        return true;
    }
}

bool sqlite3_mini_client::exec(const std::string &sql)
{
    if (!handler_)
    {
        return false;
    }
    char *errmsg = 0;
    if (debug_mode_)
    {
        last_exec_sql_debug_ = ", last_sql: ";
        last_exec_sql_debug_.append(sql);
    }
    lock_for_execute();
    if (sqlite3_exec(handler_, sql.c_str(), NULL, NULL, &errmsg) != SQLITE_OK)
    {
        unlock_for_execute();
        last_error_ = errmsg;
        zcc_error("exec sqlite3(%s), %s", sql.c_str(), errmsg);
        return false;
    }
    unlock_for_execute();
    return true;
}

bool sqlite3_mini_client::transaction_exec(const char *sqls[])
{
    bool ok = false;

    if (!begin())
    {
        goto over;
    }
    for (const char **sql = sqls; *sql; sql++)
    {
        if (!exec(*sql))
        {
            rollback();
            goto over;
        }
    }
    if (!commit())
    {
        rollback();
        goto over;
    }

    ok = true;
over:
    return ok;
}

bool sqlite3_mini_client::transaction_exec(const std::vector<std::string> &sqls)
{
    bool ok = false;

    if (!begin())
    {
        goto over;
    }
    for (auto &sql : sqls)
    {
        if (!exec(sql))
        {
            rollback();
            goto over;
        }
    }
    if (!commit())
    {
        rollback();
        goto over;
    }

    ok = true;
over:
    return ok;
}

bool sqlite3_mini_client::transaction_exec(const std::list<std::string> &sqls)
{
    bool ok = false;

    if (!begin())
    {
        goto over;
    }
    for (auto &sql : sqls)
    {
        if (!exec(sql))
        {
            rollback();
            goto over;
        }
    }
    if (!commit())
    {
        rollback();
        goto over;
    }

    ok = true;
over:
    return ok;
}

bool sqlite3_mini_client::transaction_exec(const std::string &sql)
{
    bool ok = false;

    if (!begin())
    {
        goto over;
    }
    if (!exec(sql))
    {
        rollback();
        goto over;
    }
    if (!commit())
    {
        rollback();
        goto over;
    }

    ok = true;
over:
    return ok;
}

void sqlite3_mini_client::lock_for_execute()
{
    if (transaction_lock_mode_)
    {
        return;
    }
    single_open_->thread_locker_.read_lock();
    single_open_->lock_file_locker_.lock();
    single_open_->lock_file_count_++;
    if (single_open_->lock_file_count_ == 1)
    {
        flock_sh(single_open_->lock_file_fd_);
    }
    single_open_->lock_file_locker_.unlock();
}

void sqlite3_mini_client::unlock_for_execute()
{
    if (transaction_lock_mode_)
    {
        return;
    }
    single_open_->lock_file_locker_.lock();
    single_open_->lock_file_count_--;
    if (single_open_->lock_file_count_ == 0)
    {
        flock_un(single_open_->lock_file_fd_);
    }
    single_open_->lock_file_locker_.unlock();
    single_open_->thread_locker_.unlock();
}

void sqlite3_mini_client::lock_for_transaction()
{
    single_open_->thread_locker_.write_lock();
    flock_ex(single_open_->lock_file_fd_);
    transaction_lock_mode_ = true;
}

void sqlite3_mini_client::unlock_for_transaction()
{
    transaction_lock_mode_ = false;
    flock_un(single_open_->lock_file_fd_);
    single_open_->thread_locker_.unlock();
}

zcc_namespace_end;
/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2017-11-25
 * ================================
 */

#include "zcc/zcc_memcache.h"

zcc_namespace_begin;

class memcache_engine
{
public:
    bool connect(const char *destination);
    void open_socket(int socket);
    void close(bool close_fd_or_release_ssl);
    int cmd_get(const char *key, int &flag, std::string &value);
    int cmd_asrpa(const char *op, const char *key, int flag, long timeout, const void *data, int len);
    int64_t cmd_incr_decr(const char *op, const char *key, uint64_t n);
    int cmd_del(const char *key);
    int cmd_flush_all(int64_t after_second);
    int cmd_version(std::string &version);

public:
    iostream fp_;
    std::string destination_;
    bool opened_{false};
    bool my_socket_{false};
    bool auto_reconnect_{false};
};

static bool is_valid_key(const char *key)
{
    if (zcc::empty(key))
    {
        return false;
    }
    while (*key)
    {
        if (std::iscntrl(*key) || std::isblank(*key))
        {
            return false;
        }
        key++;
    }
    return true;
}

void memcache_engine::close(bool close_fd_or_release_ssl)
{
    if (!opened_)
    {
        return;
    }
    opened_ = false;
    if (my_socket_)
    {
        fp_.close(true);
    }
    else
    {
        fp_.close(close_fd_or_release_ssl);
    }
}

bool memcache_engine::connect(const char *destination)
{
    close(true);
    destination_ = destination;
    if (!fp_.connect(destination))
    {
        return false;
    }
    return true;
}

void memcache_engine::open_socket(int socket)
{
    close(true);
    fp_.open_socket(socket);
}

int memcache_engine::cmd_get(const char *key, int &flag, std::string &value)
{
    value.clear();
    if (fp_.is_closed())
    {
        return -1;
    }
    if (!is_valid_key(key))
    {
        return -1;
    }
    fp_.puts("get ").append(key).append("\r\n");
    int have_val = 0;
    int protocol_error = 0;
    std::string str;
    char linebuf[1024 + 1];
    int linelen;
    while (1)
    {
        str.clear();
        if ((linelen = fp_.gets(linebuf, 1024)) < 1)
        {
            protocol_error = 1;
            break;
        }
        linebuf[linelen] = 0;
        char *p, *ps = linebuf;
        if (!std::strncmp(ps, "VALUE ", 6))
        {
            p = std::strchr(ps + 6, ' ');
            if (!p)
            {
                protocol_error = 1;
                break;
            }
            ps = p + 1;
            p = std::strchr(ps, ' ');
            if (!p)
            {
                protocol_error = 1;
                break;
            }
            if (flag)
            {
                *p = 0;
                flag = std::atoi(ps);
                *p = ' ';
            }
            ps = p + 1;
            int len = std::atoi(ps);
            if (len < 0)
            {
                protocol_error = 1;
                break;
            }
            if ((fp_.readn(value, len) < len) || (fp_.readn(0, 2) < 2))
            {
                protocol_error = 1;
                break;
            }
            have_val = 1;
            continue;
        }
        if (!strncmp(ps, "END", 3))
        {
            break;
        }
        protocol_error = 1;
        break;
    }
    if (protocol_error)
    {
        close(false);
        return -1;
    }
    return (have_val ? 1 : 0);
}

int memcache_engine::cmd_asrpa(const char *op, const char *key, int flag, long timeout, const void *data, int len)
{
    int ret = -1;
    if (fp_.is_closed())
    {
        return -1;
    }
    if (!is_valid_key(key))
    {
        return -1;
    }
    char linebuf[1024 + 1], *ps = linebuf;
    fp_.append(op).append(" ").append(key).append(" ").append(std::to_string(flag)).append(" ");
    fp_.append(std::to_string(timeout)).append(" ").append(std::to_string(len)).append("\r\n");
    fp_.write(data, len);
    fp_.append("\r\n");
    if (fp_.gets(linebuf, 1024) < 1)
    {
        ret = -1;
        goto over;
    }
    if (!strncmp(ps, "STORED", 6))
    {
        ret = 1;
        goto over;
    }
    if (!strncmp(ps, "NOT_STORED", 10))
    {
        ret = 0;
        goto over;
    }

over:
    if (ret < 0)
    {
        close(false);
    }
    return ret;
}

int64_t memcache_engine::cmd_incr_decr(const char *op, const char *key, uint64_t n)
{
    int ret = -1;
    if (fp_.is_closed())
    {
        return -1;
    }
    if (!is_valid_key(key))
    {
        return -1;
    }

    char linebuf[1024 + 1], *ps = linebuf;
    fp_.append(op).append(" ").append(key).append(" ").append(std::to_string(n)).append("\r\n");

    if (fp_.gets(linebuf, 1024) < 1)
    {
        close(false);
        return -1;
    }
    if (isdigit(ps[0]))
    {
        return std::atoll(ps);
    }
    close(false);
    return -1;
}

int memcache_engine::cmd_del(const char *key)
{
    int ret = -1;
    if (fp_.is_closed())
    {
        return -1;
    }
    if (!is_valid_key(key))
    {
        return -1;
    }
    char linebuf[1024 + 1], *ps = linebuf;
    fp_.append("delete ").append(key).append("\r\n");

    if (fp_.gets(linebuf, 1024) < 1)
    {
        close(false);
        return -1;
    }
    if (!std::strncmp(ps, "DELETED", 7))
    {
        return 1;
    }
    else if (!std::strncmp(ps, "NOT_FOUND", 9))
    {
        return 0;
    }
    close(false);
    return -1;
}

int memcache_engine::cmd_flush_all(int64_t after_second)
{
    int ret = -1;
    if (fp_.is_closed())
    {
        return -1;
    }
    char linebuf[1024 + 1], *ps = linebuf;
    fp_.append("flush_all ").append(std::to_string(after_second)).append("\r\n");

    if (fp_.gets(linebuf, 1024) < 1)
    {
        close(false);
        return -1;
    }
    if (!std::strncmp(ps, "OK", 2))
    {
        return 1;
    }
    close(false);
    return -1;
}

int memcache_engine::cmd_version(std::string &version)
{
    version.clear();
    if (fp_.is_closed())
    {
        return -1;
    }
    char linebuf[1024 + 1], *ps = linebuf;
    fp_.append("version\r\n");

    if (fp_.gets(linebuf, 1024) < 1)
    {
        close(false);
        return -1;
    }
    if (strncmp(ps, "VERSION ", 8))
    {
        close(false);
        return -1;
    }
    version.append(ps + 8);
    trim_line_end_rn(version);
    return 1;
}

memcache::memcache()
{
    engine_ = new memcache_engine();
}
memcache::~memcache()
{
    delete engine_;
}

bool memcache::connect(const char *destination, int times)
{
    int ret = false;
    for (int i = 0; i < times; i++)
    {
        ret = engine_->connect(destination);
        if (ret)
        {
            break;
        }
    }
    return ret;
}

void memcache::set_auto_reconnect(bool tf)
{
    engine_->auto_reconnect_ = tf;
}

void memcache::set_timeout(int timeout)
{
    engine_->fp_.set_timeout(timeout);
}

// 断开连接
void memcache::disconnect(bool close_fd_or_release_ssl)
{
    int timeout = engine_->fp_.get_timeout();
    engine_->close(close_fd_or_release_ssl);
    delete engine_;
    engine_ = new memcache_engine();
    engine_->fp_.set_timeout(timeout);
}

#define try_reconnect_do_sth(sth)   \
    int ret = -2;                   \
    int times = 1;                  \
    if (engine_->auto_reconnect_)   \
    {                               \
        times = 3;                  \
    }                               \
    for (int i = 0; i < times; i++) \
    {                               \
        ret = sth;                  \
        if (ret == -1)              \
        {                           \
            continue;               \
        }                           \
    }                               \
    return ret;

int memcache::cmd_get(const char *key, int &flag, std::string &value)
{
    try_reconnect_do_sth(engine_->cmd_get(key, flag, value));
}

int memcache::cmd_add(const char *key, int flag, int64_t timeout, const void *data, int len)
{
    try_reconnect_do_sth(engine_->cmd_asrpa("add", key, flag, timeout, data, len));
}

int memcache::cmd_set(const char *key, int flag, int64_t timeout, const void *data, int len)
{
    try_reconnect_do_sth(engine_->cmd_asrpa("set", key, flag, timeout, data, len));
}

int memcache::cmd_replace(const char *key, int flag, int64_t timeout, const void *data, int len)
{
    try_reconnect_do_sth(engine_->cmd_asrpa("replace", key, flag, timeout, data, len));
}

int memcache::cmd_append(const char *key, int flag, int64_t timeout, const void *data, int len)
{
    try_reconnect_do_sth(engine_->cmd_asrpa("append", key, flag, timeout, data, len));
}

int memcache::cmd_prepend(const char *key, int flag, int64_t timeout, const void *data, int len)
{
    try_reconnect_do_sth(engine_->cmd_asrpa("prepend", key, flag, timeout, data, len));
}

int64_t memcache::cmd_incr(const char *key, uint64_t n)
{
    try_reconnect_do_sth(engine_->cmd_incr_decr("incr", key, n));
}

int64_t memcache::cmd_decr(const char *key, uint64_t n)
{
    try_reconnect_do_sth(engine_->cmd_incr_decr("decr", key, n));
}

int memcache::cmd_del(const char *key)
{
    try_reconnect_do_sth(engine_->cmd_del(key));
}

int memcache::cmd_flush_all(int64_t after_second)
{
    try_reconnect_do_sth(engine_->cmd_flush_all(after_second));
}

int memcache::cmd_version(std::string &version)
{
    try_reconnect_do_sth(engine_->cmd_version(version));
}

zcc_namespace_end;

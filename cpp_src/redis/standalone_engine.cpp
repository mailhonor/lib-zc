/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-11-25
 * ================================
 */

#include "./redis.h"

zcc_namespace_begin;

class redis_client_standalone_engine : public redis_client_basic_engine
{
public:
    redis_client_standalone_engine();
    ~redis_client_standalone_engine();
    int open();
    void close();
    int query_protocol(int64_t *number_ret, std::string *string_ret, std::list<std::string> *list_ret, json *json_ret, const std::list<std::string> &query_tokens);
    void set_timeout(int wait_timeout);
    int timed_read_wait(int wait_timeout);
    int timed_write_wait(int wait_timeout);
    void set_auto_reconnect(bool tf = true);

public:
    iostream fp_;
};

redis_client_standalone_engine::redis_client_standalone_engine()
{
}

redis_client_standalone_engine::~redis_client_standalone_engine()
{
    close();
}

int redis_client_standalone_engine::query_protocol(int64_t *number_ret, std::string *string_ret, std::list<std::string> *list_ret, json *json_ret, const std::list<std::string> &query_tokens)
{
    if (!fp_.is_opened())
    {
        open();
    }
    if (!fp_.is_opened())
    {
        info_msg_ = "can not open " + destination_;
        return redis_fatal;
    }
    int ret = -2;
    int times = 1;
    if (auto_reconnect_)
    {
        times = 3;
    }
    for (int i = 0; i < times; i++)
    {
        ret = query_protocol_by_stream(number_ret, string_ret, list_ret, json_ret, query_tokens, fp_);
        if (ret == redis_fatal)
        {
            close();
            open();
            continue;
        }
        break;
    }
    return ret;
}

int redis_client_standalone_engine::open()
{
    close();
    if (!fp_.connect(destination_))
    {
        return redis_fatal;
    }
    if (!password_.empty())
    {
        int ret = query_protocol(0, 0, 0, 0, {"AUTH", password_});
        if (ret == 0)
        {
            return 0;
        }
        if (ret < 0)
        {
            if (!std::strncmp(info_msg_.c_str(), "-ERR ", 5))
            {
                return redis_none;
            }
            else
            {
                return redis_error;
            }
        }
    }
    return redis_ok;
}

void redis_client_standalone_engine::close()
{
    fp_.close();
}

void redis_client_standalone_engine::set_timeout(int wait_timeout)
{
    wait_timeout_ = wait_timeout;
    fp_.set_timeout(wait_timeout);
}

int redis_client_standalone_engine::timed_read_wait(int wait_timeout)
{
    if (!fp_.is_opened())
    {
        return redis_error;
    }
    return fp_.timed_read_wait(wait_timeout);
}

int redis_client_standalone_engine::timed_write_wait(int wait_timeout)
{
    if (!fp_.is_opened())
    {
        return redis_error;
    }
    return fp_.timed_read_wait(wait_timeout);
}

void redis_client_standalone_engine::set_auto_reconnect(bool tf)
{
    auto_reconnect_ = tf;
}

redis_client_basic_engine *redis_client_standalone_engine_create()
{
    redis_client_standalone_engine *engine = new redis_client_standalone_engine();
    return engine;
}

zcc_namespace_end;

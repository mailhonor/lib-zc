/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-11-25
 * ================================
 */

#include "./redis.h"

zcc_namespace_begin;

redis_client_basic_engine *redis_client_standalone_engine_create();
redis_client_basic_engine *redis_client_cluster_engine_create();

redis_client::redis_client()
{
}

redis_client::~redis_client()
{
    close();
}

void redis_client::open(redis_client_basic_engine *engine)
{
    close();
    engine_ = engine;
    engine_->set_timeout(wait_timeout_);
    engine_->set_auto_reconnect(auto_reconnect_);
}

int redis_client::connect(const char *destination, int times)
{
    close();
    engine_ = redis_client_standalone_engine_create();
    engine_->set_timeout(wait_timeout_);
    engine_->set_auto_reconnect(auto_reconnect_);
    engine_->set_password(password_);
    engine_->set_destination(destination);
    int ret = redis_error;
    for (int i = 0; i < times; i++)
    {
        ret = engine_->open();
        if (ret >= redis_error)
        {
            break;
        }
    }
    return ret;
}

int redis_client::cluster_connect(const char *destination, int times)
{
    close();
    engine_ = redis_client_cluster_engine_create();
    engine_->set_timeout(wait_timeout_);
    engine_->set_auto_reconnect(auto_reconnect_);
    engine_->set_password(password_);
    engine_->set_destination(destination);
    int ret = redis_error;
    for (int i = 0; i < times; i++)
    {
        ret = engine_->open();
        if (ret >= redis_error)
        {
            break;
        }
    }
    return ret;
}

void redis_client::close(bool release_engine)
{
    if (release_engine)
    {
        if (engine_)
        {
            delete engine_;
            engine_ = nullptr;
        }
    }
}

void redis_client::set_timeout(int wait_timeout)
{
    wait_timeout_ = wait_timeout;
    if (engine_)
    {
        engine_->set_timeout(wait_timeout_);
    }
}

void redis_client::set_auto_reconnect(bool tf)
{
    auto_reconnect_ = tf;
    if (engine_)
    {
        engine_->set_auto_reconnect(tf);
    }
}

void redis_client::set_password(const char *password)
{
    password_ = password;
    if (engine_)
    {
        engine_->set_password(password);
    }
}

const std::string &redis_client::get_info_msg()
{
    if (engine_)
    {
        return engine_->info_msg_;
    }
    else
    {
        return var_blank_string;
    }
}

int redis_client::exec_command(const std::list<std::string> &query_tokens)
{
    if (!engine_)
    {
        return redis_error;
    }
    return engine_->query_protocol(0, 0, 0, 0, query_tokens);
}

int redis_client::exec_command(int64_t &number_ret, const std::list<std::string> &query_tokens)
{
    if (!engine_)
    {
        return redis_error;
    }
    return engine_->query_protocol(&number_ret, 0, 0, 0, query_tokens);
}

int redis_client::exec_command(std::string &string_ret, const std::list<std::string> &query_tokens)
{
    if (!engine_)
    {
        return redis_error;
    }
    return engine_->query_protocol(0, &string_ret, 0, 0, query_tokens);
}

int redis_client::exec_command(std::list<std::string> &list_ret, const std::list<std::string> &query_tokens)
{
    if (!engine_)
    {
        return redis_error;
    }
    return engine_->query_protocol(0, 0, &list_ret, 0, query_tokens);
}

int redis_client::exec_command(json &json_ret, const std::list<std::string> &query_tokens)
{
    if (!engine_)
    {
        return redis_error;
    }
    return engine_->query_protocol(0, 0, 0, &json_ret, query_tokens);
}

int redis_client::_general_scan(std::list<std::string> &list_ret, int64_t &cursor_ret, const std::string &cmd, int64_t cursor)
{
    const std::list<std::string> query_tokens{cmd, std::to_string(cursor)};
    int ret = exec_command(list_ret, query_tokens);
    if (ret > 0)
    {
        if (list_ret.empty())
        {
            return redis_error;
        }
        std::string &cursor_str = list_ret.front();
        cursor_ret = std::atol(cursor_str.c_str());
        list_ret.pop_front();
    }
    return ret;
}

int redis_client::info(std::map<std::string, std::string> &name_value_dict, std::string &string_ret)
{
    name_value_dict.clear();
    string_ret.clear();
    int ret = exec_command(string_ret, {"INFO"});
    if (ret < 1)
    {
        return ret;
    }
    const char *ps, *p = string_ret.c_str();
    std::string tmpn, tmpv;
    while (*p)
    {
        if ((*p == '#') || (*p == ' ') || (*p == '\r'))
        {
            p++;
            for (; *p; p++)
            {
                if (*p == '\n')
                {
                    break;
                }
            }
            if (*p == '\n')
            {
                p++;
            }
            continue;
        }
        ps = p;
        for (; *p; p++)
        {
            if (*p == ':')
            {
                break;
            }
        }
        if (*p == 0)
        {
            break;
        }
        tmpn.clear();
        if (p > ps)
        {
            tmpn.append(ps, p - ps);
        }
        p++;
        ps = p;

        for (; *p; p++)
        {
            if (*p == '\n')
            {
                break;
            }
        }
        if (*p == 0)
        {
            break;
        }
        tmpv.clear();
        if (p - ps > 1)
        {
            tmpv.append(ps, p - ps - 1);
        }
        name_value_dict[tmpn] = tmpv;
        p++;
    }
    return ret;
}

int redis_client::subscribe(const char *channel)
{
    if (!engine_)
    {
        return redis_error;
    }
    return engine_->query_protocol((int64_t *)-1, 0, 0, 0, {"SUBSCRIBE", channel});
}

int redis_client::unsubscribe(const char *channel)
{
    if (empty(channel))
    {
        return exec_command({"UNSUBSCRIBE"});
    }
    else
    {
        return exec_command({"UNSUBSCRIBE", channel});
    }
}

int redis_client::psubscribe(const char *channel)
{
    if (!engine_)
    {
        return redis_error;
    }
    return engine_->query_protocol((int64_t *)-1, 0, 0, 0, {"PSUBSCRIBE", channel});
}

int redis_client::punsubscribe(const char *channel)
{
    if (empty(channel))
    {
        return exec_command({"PUNSUBSCRIBE"});
    }
    else
    {
        return exec_command({"PUNSUBSCRIBE", channel});
    }
}

int redis_client::fetch_channel_message(std::string &type, std::string &channel, std::string &data, int timeout)
{
    int ret;
    std::list<std::string> list_ret;
    if (!engine_)
    {
        return redis_error;
    }
    if ((ret = engine_->timed_read_wait(timeout)) < 1)
    {
        return ret;
    }
    ret = exec_command(list_ret, var_blank_list);
    if (ret < 1)
    {
        return ret;
    }
    if (list_ret.empty() || ((list_ret.size() != 3) && (list_ret.size() != 4)))
    {
        return redis_error;
    }
    type = list_ret.front();
    data = list_ret.back();
    list_ret.pop_back();
    channel = list_ret.back();
    return ret;
}

zcc_namespace_end;

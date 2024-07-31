/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2016-01-15
 * ================================
 */

#pragma once

#ifndef ZCC_LIB_INCLUDE_REDIS___
#define ZCC_LIB_INCLUDE_REDIS___

#include "./zcc_stdlib.h"

#ifdef __cplusplus
#pragma pack(push, 4)
zcc_namespace_begin;

class redis_client_basic_engine
{
public:
    redis_client_basic_engine();
    virtual ~redis_client_basic_engine();
    virtual int open() { return -2; }
    virtual int query_protocol(int64_t *number_ret, std::string *string_ret, std::list<std::string> *list_ret, json *json_ret, const std::list<std::string> &query_tokens) = 0;
    int query_protocol_by_stream(int64_t *number_ret, std::string *string_ret, std::list<std::string> *list_ret, json *json_ret, const std::list<std::string> &query_tokens, stream &fp);
    virtual inline void set_destination(const char *destination) { destination_ = destination; }
    virtual inline void set_destination(const std::string &destination) { destination_ = destination; }
    virtual inline void set_password(const char *password) { password_ = password; }
    virtual inline void set_password(const std::string &password) { password_ = password; }
    virtual inline void set_timeout(int wait_timeout) { wait_timeout_ = wait_timeout; }
    virtual inline int timed_read_wait(int wait_timeout) { return 1; }
    virtual inline int timed_write_wait(int wait_timeout) { return 1; }
    virtual inline void set_auto_reconnect(bool tf = true) {}

public:
    std::string info_msg_;
    std::string destination_;
    std::string password_;
    int wait_timeout_{-1};
    bool auto_reconnect_{false};
};

// redis命令返回结果可以抽象为json, 绝大部分可以简化为4类:
// 成功/失败, 整数, 字符串, 字符串 list

// redis_client 类里面所有的操作类方法,其返回值规范
// -2: 网络错误/系统错误/协议错误
// -1: 逻辑错误
//  0: 失败/不存在/逻辑错误/等
// >0: 成功/存在/逻辑正确/等

class redis_client
{
public:
    redis_client();
    virtual ~redis_client();
    void open(redis_client_basic_engine *engine);
    const std::string &get_info_msg();
    void set_timeout(int wait_timeout);
    void set_auto_reconnect(bool tf = true);
    virtual void set_password(const char *password);
    virtual void inline set_password(const std::string &password) { set_password(password.c_str()); }
    int connect(const char *destination, int times = 1);
    int connect(const std::string &destination, int times = 1)
    {
        return connect(destination.c_str(), times);
    }
    int cluster_connect(const char *destinations, int times = 1);
    int cluster_connect(const std::string &destination, int times = 1)
    {
        return cluster_connect(destination.c_str(), times);
    }
    void close(bool release_engine = true);
    int exec_command(const std::list<std::string> &query_tokens);
    int exec_command(int64_t &number_ret, const std::list<std::string> &query_tokens);
    int exec_command(std::string &string_ret, const std::list<std::string> &query_tokens);
    int exec_command(std::list<std::string> &list_ret, const std::list<std::string> &query_tokens);
    int exec_command(json &json_ret, const std::list<std::string> &query_tokens);
    int info(std::map<std::string, std::string> &name_value_dict, std::string &string_ret);
    int scan(std::list<std::string> &list_ret, int64_t &cursor_ret, int64_t cursor)
    {
        return _general_scan(list_ret, cursor_ret, "SCAN", cursor);
    }
    int hscan(std::list<std::string> &list_ret, int64_t &cursor_ret, int64_t cursor)
    {
        return _general_scan(list_ret, cursor_ret, "HSCAN", cursor);
    }
    int sscan(std::list<std::string> &list_ret, int64_t &cursor_ret, int64_t cursor)
    {
        return _general_scan(list_ret, cursor_ret, "SSCAN", cursor);
    }
    int zscan(std::list<std::string> &list_ret, int64_t &cursor_ret, int64_t cursor)
    {
        return _general_scan(list_ret, cursor_ret, "ZSCAN", cursor);
    }

    int subscribe(const char *channel);
    int subscribe(const std::string &channel)
    {
        return subscribe(channel.c_str());
    }
    int unsubscribe(const char *channel = nullptr);
    int unsubscribe(const std::string &channel = var_blank_string)
    {
        return unsubscribe(channel.c_str());
    }
    int psubscribe(const char *channel);
    int psubscribe(const std::string &channel)
    {
        return psubscribe(channel.c_str());
    }
    int punsubscribe(const char *channel = nullptr);
    int punsubscribe(const std::string &channel = var_blank_string)
    {
        return punsubscribe(channel.c_str());
    }
    // 返回 -1: 出错, 返回 0: 超时, 返回 1, 有数据
    int fetch_channel_message(std::string &type, std::string &channel, std::string &data, int timeout);

protected:
    int _general_scan(std::list<std::string> &list_ret, int64_t &cursor_ret, const std::string &cmd, int64_t cursor);

protected:
    redis_client_basic_engine *engine_{nullptr};
    std::string password_;
    int wait_timeout_{-1};
    bool auto_reconnect_{false};
};

zcc_namespace_end;
#pragma pack(pop)
#endif // __cplusplus

#endif // ZCC_LIB_INCLUDE_REDIS___

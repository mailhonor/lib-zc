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

// 作者也写过几个版本的客户端, 都不满意. 其中有一版有600多个方法.
// 封装的东西很难满足使用者包括本人的需求. 哪怕仅仅是 GET 协议也是如此.
class redis_client
{
public:
    redis_client();
    virtual ~redis_client();
    // 打开一个引擎, 一般不用这个方法
    // 这个方法用于自己提供引擎, 比如其他的 socket/stream等
    void open(redis_client_basic_engine *engine);
    // 信息, 错误信息等
    const std::string &get_info_msg();
    // 设置超时
    void set_timeout(int wait_timeout);
    // 自动重连
    void set_auto_reconnect(bool tf = true);
    // 密码
    virtual void set_password(const char *password);
    virtual void inline set_password(const std::string &password) { set_password(password.c_str()); }
    // 连接服务器, 如 localhost:6379, times: 重试次数
    int connect(const char *destination, int times = 1);
    int connect(const std::string &destination, int times = 1)
    {
        return connect(destination.c_str(), times);
    }
    // 连接集群
    int cluster_connect(const char *destinations, int times = 1);
    int cluster_connect(const std::string &destination, int times = 1)
    {
        return cluster_connect(destination.c_str(), times);
    }
    // 关闭, release_engine 为真则释放引擎
    void close(bool release_engine = true);
    // 返回 -1: 错; 0: 失败/不存在/逻辑错误/...; 1: 成功/存在/逻辑正确
    int exec_command(const std::list<std::string> &query_tokens);
    // 返回: 如上; 一些命令, 适合得到一个整数结果并赋值给 number_ret, 如 klen/incrby/ttl
    int exec_command(int64_t &number_ret, const std::list<std::string> &query_tokens);
    // 返回: 如上; 一些命令, 适合得到一个字符串结果赋值给 string_ret, 如 GET/HGET
    // rc.exec_command(sval, {"GET", "abc"});
    int exec_command(std::string &string_ret, const std::list<std::string> &query_tokens);
    // 返回: 如上; 一些命令, 适合得到一串字符串结果并赋值给 list_ret, 如 MGET/HMGET/ */
    int exec_command(std::list<std::string> &list_ret, const std::list<std::string> &query_tokens);
    // 返回: 如上; 所有命令都可以
    int exec_command(json &json_ret, const std::list<std::string> &query_tokens);

    // 命令 info 封装
    int info(std::map<std::string, std::string> &name_value_dict, std::string &string_ret);

    // 命令 scan 封装
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

    // 订阅
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
    // 读取频道,  返回 -1: 出错, 返回 0: 超时, 返回 1, 有数据
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

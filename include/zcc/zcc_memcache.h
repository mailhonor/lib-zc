/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2017-02-18
 * ================================
 */

#pragma once

#ifndef ZCC_LIB_INCLUDE_MEMCACHE__
#define ZCC_LIB_INCLUDE_MEMCACHE__

#include "./zcc_stdlib.h"
#include "./zcc_stream.h"

#ifdef __cplusplus
#pragma pack(push, 4)
zcc_namespace_begin;

// memcache 客户端
class memcache_engine;
class memcache
{
public:
    memcache();
    ~memcache();
    // 连接
    bool connect(const char *destination, int times = 1);
    inline bool connect(const std::string &destination, int times = 1)
    {
        return connect(destination.c_str(), times);
    }
    // 断开连接
    void disconnect(bool close_fd_or_release_ssl = true);

    void set_auto_reconnect(bool tf = true);

    // 设置可读/可写超时时间, 单位秒
    void set_timeout(int wait_timeout);

    // GET命令, 返回 -1: 错, 0: 不存在, 1: 存在
    // key: 键; *flag: 返回的标记; value: 返回的值
    int cmd_get(const char *key, int &flag, std::string &value);
    int cmd_get(const std::string &key, int &flag, std::string &value)
    {
        return cmd_get(key.c_str(), flag, value);
    }

    // ADD/SET/REPLACE/APPEND/PREPEND命令, 返回 -1:错; 0:存储失败; 1:存储成功
    // key: 键; flag: 标记; data/len: 值/长度;
    int cmd_add(const char *key, int flag, int64_t timeout, const void *data, int len);
    inline int cmd_add(const std::string &key, int flag, int64_t timeout, const std::string &data)
    {
        return cmd_add(key.c_str(), flag, timeout, data.c_str(), data.size());
    }
    int cmd_set(const char *key, int flag, int64_t timeout, const void *data, int len);
    inline int cmd_set(const std::string &key, int flag, int64_t timeout, const std::string &data)
    {
        return cmd_set(key.c_str(), flag, timeout, data.c_str(), data.size());
    }
    int cmd_replace(const char *key, int flag, int64_t timeout, const void *data, int len);
    inline int rcmd_eplace(const std::string &key, int flag, int64_t timeout, const std::string &data)
    {
        return cmd_replace(key.c_str(), flag, timeout, data.c_str(), data.size());
    }
    int cmd_append(const char *key, int flag, int64_t timeout, const void *data, int len);
    inline int cmd_append(const std::string &key, int flag, int64_t timeout, const std::string &data)
    {
        return cmd_append(key.c_str(), flag, timeout, data.c_str(), data.size());
    }
    int cmd_prepend(const char *key, int flag, int64_t timeout, const void *data, int len);
    inline int cmd_prepend(const std::string &key, int flag, int64_t timeout, const std::string &data)
    {
        return cmd_prepend(key.c_str(), flag, timeout, data.c_str(), data.size());
    }

    // INCR命令, 返回 -1: 错; >= 0: incr的结果
    int64_t cmd_incr(const char *key, uint64_t n);
    inline int64_t cmd_incr(const std::string &key, uint64_t n)
    {
        return cmd_incr(key.c_str(), n);
    }

    // DECR命令, 返回 -1: 错; >= 0: decr的结果
    int64_t cmd_decr(const char *key, uint64_t n);
    inline int64_t cmd_decr(const std::string &key, uint64_t n)
    {
        return cmd_decr(key.c_str(), n);
    }

    // DEL命令, 返回 -1: 错; 0: 不存在; 1: 删除成功
    int cmd_del(const char *key);
    inline int cmd_del(const std::string &key)
    {
        return cmd_del(key.c_str());
    }

    // FLUASH_ALL命令, 返回 -1:错; 0: 未知; 1: 成功
    int cmd_flush_all(int64_t after_second);

    // VERSION命令, -1: 错; 1:成功
    int cmd_version(std::string &version);

private:
    memcache_engine *engine_;
};

zcc_namespace_end;
#pragma pack(pop)
#endif // __cplusplus

#endif // ZCC_LIB_INCLUDE_MEMCACHE__

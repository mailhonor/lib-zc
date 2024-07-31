/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2017-02-18
 * ================================
 */

#pragma once

#ifndef ZCC_LIB_INCLUDE_AIO___
#define ZCC_LIB_INCLUDE_AIO___

#include <functional>
#include "./zcc_stdlib.h"
#include "./zcc_openssl.h"

#ifdef __cplusplus
#pragma pack(push, 4)
zcc_namespace_begin;

/* event, src/event/ ################################################### */
/* 基于epoll的高并发io模型, 包括 事件, 异步io, 定时器, io映射. 例子见 cpp_sample/event/  */

struct aio_engine;
struct aio_base_engine;
class aio_friend;

class aio
{
    friend aio_friend;

public:
    aio();
    aio(int fd, aio_base *aiobase = nullptr);
    aio(SSL *ssl, aio_base *aiobase = nullptr);
    virtual ~aio();
    void rebind_fd(int fd, aio_base *aiobase = nullptr);
    void rebind_SLL(SSL *ssl, aio_base *aiobase = nullptr);
    /* 获取 SSL 句柄 */
    void close(bool close_fd_and_release_ssl = true);
    /* 重新绑定 aio_base */
    void rebind_aio_base(aio_base *aiobase);
    /* 设置可读写超时 */
    void set_timeout(int wait_timeout);
    /* 停止 aio, 只能在所属 aio_base 运行的线程执行 */
    void disable();
    /* 获取结果, -2:超时(且没有任何数据), <0: 错, >0: 写成功,或可读的字节数 */
    int get_result();
    /* 获取 fd */
    int get_fd();
    /* 获取 SSL 句柄 */
    int get_read_cache_size();
    void get_read_cache(char *buf, int strict_len);
    void get_read_cache(std::string &bf, int strict_len);
    /* 获取缓存数据的长度 */
    int get_write_cache_size();
    void get_write_cache(char *buf, int strict_len);
    void get_write_cache(std::string &bf, int strict_len);
    /* 如果可读(或出错)则回调执行函数 callback */
    void readable(std::function<void()> callback);
    /* 如果可写(或出错)则回调执行函数 callback */
    void writeable(std::function<void()> callback);
    /* 请求读, 最多读取max_len个字节, 成功/失败/超时后回调执行callback */
    void read(int max_len, std::function<void()> callback);
    /* 请求读, 严格读取strict_len个字节, 成功/失败/超时后回调执行callback */
    void readn(int strict_len, std::function<void()> callback);
    /* */
    /* 请求读, 读到delimiter为止, 最多读取max_len个字节, 成功/失败/超时后回调执行callback */
    void read_delimiter(int delimiter, int max_len, std::function<void()> callback);
    /* 如上, 读行 */
    inline void gets(int max_len, std::function<void()> callback) { read_delimiter('\n', max_len, callback); }
    // compact data
    void get_cint(std::function<void()> callback);
    void get_cint_and_data(std::function<void()> callback);
    /* 向缓存写数据 */
    int cache_write(const void *buf, int len = -1);
    inline int cache_write(const std::string &bf) { return cache_write(bf.c_str(), bf.size()); }
    inline int cache_append(const std::string &bf) { return cache_write(bf); }
    inline int cache_append(const char *s) { return cache_write(s); }
    inline int cache_puts(const char *s) { return cache_write(s); }
    // compact data
    void cache_write_cint(int len);
    void cache_write_cint_and_data(const void *data, int len);
#ifdef _WIN64
    void cache_printf_1024(const char *fmt, ...);
#else  // _WIN64
    void __attribute__((format(gnu_printf, 2, 3))) cache_printf_1024(const char *fmt, ...);
#endif // _WIN64
    /* */
    /* 向缓存写数据, 不复制buf */
    void cache_write_direct(const void *buf, int len);
    /* 请求写, 成功/失败/超时后回调执行callback */
    void cache_flush(std::function<void()> callback);
    /* */
    /* 请求sleep, sleep秒后回调执行callback */
    void sleep(std::function<void()> callback, int timeout);
    /* 获取 aio_base */
    aio_base *get_aio_base();
    // SSL
    SSL *get_ssl();
    /* 发起tls连接, 成功/失败/超时后回调执行callback */
    void tls_connect(SSL_CTX *ctx, std::function<void()> callback);
    /* 发起tls接受, 成功/失败/超时后回调执行callback */
    void tls_accept(SSL_CTX *ctx, std::function<void()> callback);

protected:
    aio_engine *engine_{nullptr};
};

class aio_timer : public aio
{
public:
    aio_timer(aio_base *aiobase = nullptr);
    virtual ~aio_timer();
    void rebind_aio_base(aio_base *aiobase = nullptr);
    void after(std::function<void()> callback, int timeout);
};

/* event/epoll 运行框架 */
/* 默认aio_base */
extern aio_base *var_main_aio_base;

class aio_base
{
    friend aio_friend;

public:
    static aio_base *get_current_thread_aio_base();

public:
    aio_base();
    ~aio_base();
    /* 设置 aio_base 每次epoll循环需要执行的函数 */
    void set_loop_fn(std::function<void()> callback);
    /* 运行 aio_base */
    void run();
    /* 通知 aio_base 停止, 既 zaio_base_run 返回 */
    void stop_notify();
    /* 通知 aio_base, 手动打断 epoll_wait */
    void touch();
    /* sleep秒后回调执行callback, 只执行一次 callback(ctx) */
    void enter_timer(std::function<void()> callback, int timeout);

protected:
    aio_base_engine *engine_;
};

void aio_iopipe_enter(aio *client, aio *server, aio_base *aiobase, std::function<void()> after_close);

zcc_namespace_end;
#pragma pack(pop)
#endif // __cplusplus

#endif // ZCC_LIB_INCLUDE_AIO___

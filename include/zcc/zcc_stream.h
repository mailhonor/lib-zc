/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-12-09
 * ================================
 */

#pragma once

#ifndef ZCC_LIB_INCLUDE_STREAM___
#define ZCC_LIB_INCLUDE_STREAM___

#include "./zcc_stdlib.h"
#include "./zcc_openssl.h"

#ifdef __cplusplus
#pragma pack(push, 4)
zcc_namespace_begin;

// 流
class stream
{
public:
    static const int rbuf_size = 4096;
    static const int wbuf_size = 4096;
    struct stream_engine
    {
        int wait_timeout;
        short int read_buf_p1;
        short int read_buf_p2;
        short int write_buf_len;
        unsigned short int error : 1;
        unsigned short int eof : 1;
        unsigned char read_buf[rbuf_size];
        unsigned char write_buf[wbuf_size];
    };
    stream();
    virtual ~stream();
    stream &reset();

public:
    // connect 网络地址, 如 "127.0.0.1:6379,localhost:6388", "/var/run/some-domain-socket"
    virtual inline bool connect(const char *destination) { return false; }
    inline bool connect(const std::string &destination)
    {
        return connect(destination.c_str());
    }
    // openssl, 连接
    virtual int tls_connect(void *ctx) { return -1; }
    // openssl, 接受连接
    virtual int tls_accept(void *ctx) { return -1; }
    // socket
    virtual inline int get_socket() { return -1; }
    // 超时
    inline int get_timeout() { return engine_->wait_timeout; }
    // 设置超时
    virtual stream &set_timeout(int wait_timeout);
    // 带超时是否可读
    virtual int timed_read_wait(int wait_timeout);
    // 是否有真实可读数据
    virtual inline int trueDataReadabel() { return 0; }
    // 带超时是否可写
    virtual int timed_write_wait(int wait_timeout);
    // close
    virtual inline int close(bool close_fd_or_release_ssl = true) { return true; }
    // 是否已经打开/连接
    virtual inline bool is_opened() { return false; }
    // 是否已经关闭
    virtual inline bool is_closed() { return true; }
    // 是否错误
    inline bool is_error() { return (engine_->error ? true : false); }
    // 是否可读结束
    inline bool is_eof() { return (engine_->eof ? true : false); }
    // 设置错误
    inline stream &set_error(bool tf = true)
    {
        engine_->error = (tf ? 1 : 0);
        return *this;
    }
    // 设置可读结束
    inline stream &set_eof(bool tf = true)
    {
        engine_->eof = (tf ? 1 : 0);
        return *this;
    }
    // 是否异常
    inline bool is_exception()
    {
        return (((engine_->eof) || (engine_->error)) ? true : false);
    }
    // 读cache里数据的长度
    inline int get_read_cache_len()
    {
        return (engine_->read_buf_p2 - engine_->read_buf_p1);
    }
    // 读cache里数据的地址
    inline char *get_read_cache() { return (char *)(engine_->read_buf); }
    // 写cache里数据的长度
    inline int get_write_cache_len() { return (engine_->write_buf_len); }
    // 写cache里数据的地址
    inline char *get_write_cache()
    {
        engine_->write_buf[engine_->write_buf_len] = 0;
        return (char *)(engine_->write_buf);
    }
    // 读 1 个字节
    inline int getc()
    {
        return ((engine_->read_buf_p1 < engine_->read_buf_p2) ? ((int)(engine_->read_buf[engine_->read_buf_p1++])) : (getc_do()));
    }
    // 退回一个字节
    stream &ungetc();
    // 读最多max_len的字节
    int read(void *mem, int max_len);
    int read(std::string &str, int max_len);
    // 严格读取strict_len个字节
    inline int readn(int strict_len) { return readn((void *)0, strict_len); }
    int readn(void *mem, int strict_len);
    int readn(std::string &str, int strict_len);
    // 读取到分隔符delimiter为止, 最多 max_len 个字节
    int read_delimiter(void *mem, int delimiter, int max_len);
    int read_delimiter(std::string &str, int delimiter, int max_len);
    // 读行
    inline int gets(void *mem, int max_len) { return read_delimiter(mem, '\n', max_len); }
    inline int gets(std::string &str, int max_len) { return read_delimiter(str, '\n', max_len); }

    // 写一个字节
    inline int putc(int c)
    {
        return ((engine_->write_buf_len < wbuf_size) ? (engine_->write_buf[engine_->write_buf_len++] = (int)(c), (int)(c)) : (putc_do(c)));
    }
    // 刷写缓存
    int flush();
    // 写
    int write(const void *buf, int len);
    inline stream &puts(const char *s, int64_t len = -1)
    {
        write(s, len);
        return *this;
    }
    inline stream &puts(const std::string &str)
    {
        write(str.c_str(), (int)str.size());
        return *this;
    }
    inline stream &append(const char *s, int slen = -1)
    {
        write(s, slen);
        return *this;
    }
    inline stream &append(const std::string &str)
    {
        write(str.c_str(), (int)str.size());
        return *this;
    }
    // 类似fprintf, 最多 1024 字节 
#ifdef __linux__
    int __attribute__((format(gnu_printf, 2, 3))) printf_1024(const char *format, ...);
#else  // __linux__
    int printf_1024(const char *format, ...);
#endif // __attribute__
    // 下面的跳过去, 节省传输用的
    int get_cint();
    int write_cint(int len);
    int write_cint_and_int(int i);
    int write_cint_and_long(int64_t l);
    int write_cint_and_data(const void *buf, int len);
    inline int write_cint_and_pp(const char **pp, int size);
    //
    inline bool is_iostream() { return is_iostream_; }

protected:
    virtual int engine_read(void *buf, int len) = 0;
    virtual int engine_write(const void *buf, int len) = 0;

private:
    int getc_do();
    int putc_do(int ch);

protected:
    stream_engine *engine_{nullptr};
    bool is_iostream_{false};
};

// io类stream, 主要用于socket
class iostream : public stream
{
public:
    iostream();
    iostream(int fd);
    iostream(SSL *ssl);
    virtual ~iostream();
 
    // open socket
    iostream &open_socket(int fd);
    // open ssl
    iostream &open_ssl(SSL *ssl);
    // 直接连接地址
    bool connect(const char *destination);
    inline bool connect(const std::string &destination)
    {
        return connect(destination.c_str());
    }

public:
    virtual inline int get_socket() { return fd_; }
    virtual iostream &set_timeout(int wait_timeout);
    virtual int trueDataReadabel();
    virtual int timed_read_wait(int wait_timeout);
    virtual int timed_write_wait(int wait_timeout);
    virtual int close(bool close_fd_or_release_ssl = true);
    virtual inline bool is_opened()
    {
        return (fd_ > -1);
    }
    virtual inline bool is_closed()
    {
        return (fd_ < 0);
    }
    inline SSL *get_ssl() { return ssl_; }
    int tls_connect(SSL_CTX *ctx);
    int tls_accept(SSL_CTX *ctx);
    inline int tls_connect(void *ctx) { return tls_connect((SSL_CTX *)ctx); }
    inline int tls_accept(void *ctx) { return tls_accept((SSL_CTX *)ctx); }

protected:
    virtual int engine_read(void *buf, int len);
    virtual int engine_write(const void *buf, int len);
    virtual void operate_nonblocking();

protected:
    int fd_{-1};
    SSL *ssl_{nullptr};
    bool nonblocking_set_flag_{false};
    bool nonblocking_flag_{false};
};

// 本地文件stream, 类似 FILE, 尽量别用, 推荐用 FILE
// 本类存在的意义是: 协程内使用. FILE 在本框架的携程库下还是阻塞执行
class fstream : public stream
{
public:
    fstream();
#ifdef _WIN64
    fstream(HANDLE fd);
#else  // _WIN64
    fstream(int fd);
#endif // _WIN64
    virtual ~fstream();

public:
    bool open(const char *fn);
    inline bool open(const std::string &fn)
    {
        return open(fn.c_str());
    }
#ifdef _WIN64
    fstream &open(HANDLE fd);
    virtual inline HANDLE get_fd() { return fd_; }
    virtual inline HANDLE get_handle() { return fd_; }
#else  // _WIN64
    fstream &open(int fd);
    virtual inline int get_fd() { return fd_; }
    virtual inline int get_handle() { return fd_; }
#endif // _WIN64
    int close(bool close_fd_or_release_ssl = true);

protected:
    virtual int engine_read(void *buf, int len);
    virtual int engine_write(const void *buf, int len);

protected:
#ifdef _WIN64
    HANDLE fd_{INVALID_HANDLE_VALUE};
#else  // _WIN64
    int fd_{-1};
#endif // _WIN64
};

zcc_namespace_end;
#pragma pack(pop)
#endif // __cplusplus

#endif // ZCC_LIB_INCLUDE_STREAM___

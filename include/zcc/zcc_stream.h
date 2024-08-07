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
#ifdef _WIN64
#include <windows.h>
#endif //_WIN64

#ifdef __cplusplus
#pragma pack(push, 4)
zcc_namespace_begin;

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
    virtual inline int get_socket() { return -1; }
    inline int get_timeout() { return engine_->wait_timeout; }
    virtual stream &set_timeout(int wait_timeout);
    virtual int timed_read_wait(int wait_timeout);
    virtual int timed_write_wait(int wait_timeout);
    virtual inline bool is_opened() { return false; };
    virtual inline bool is_closed() { return true; };
    inline bool is_error() { return (engine_->error ? true : false); }
    inline bool is_eof() { return (engine_->eof ? true : false); }
    inline stream &set_error(bool tf = true)
    {
        engine_->error = (tf ? 1 : 0);
        return *this;
    }
    inline stream &set_eof(bool tf = true)
    {
        engine_->eof = (tf ? 1 : 0);
        return *this;
    }
    inline bool is_exception()
    {
        return (((engine_->eof) || (engine_->error)) ? true : false);
    }
    inline int get_read_cache_len()
    {
        return (engine_->read_buf_p2 - engine_->read_buf_p1);
    }
    inline char *get_read_cache() { return (char *)(engine_->read_buf); }
    inline int get_write_cache_len() { return (engine_->write_buf_len); }
    inline char *get_write_cache()
    {
        engine_->write_buf[engine_->write_buf_len] = 0;
        return (char *)(engine_->write_buf);
    }
    inline int getc()
    {
        return ((engine_->read_buf_p1 < engine_->read_buf_p2) ? ((int)(engine_->read_buf[engine_->read_buf_p1++])) : (getc_do()));
    }
    stream &ungetc();
    int read(void *mem, int max_len);
    int read(std::string &str, int max_len);
    inline int readn(int strict_len) { return readn((void *)0, strict_len); }
    int readn(void *mem, int strict_len);
    int readn(std::string &str, int strict_len);
    int read_delimiter(void *mem, int delimiter, int max_len);
    int read_delimiter(std::string &str, int delimiter, int max_len);
    inline int gets(void *mem, int max_len) { return read_delimiter(mem, '\n', max_len); }
    inline int gets(std::string &str, int max_len) { return read_delimiter(str, '\n', max_len); }
    inline int putc(int c)
    {
        return ((engine_->write_buf_len < wbuf_size) ? (engine_->write_buf[engine_->write_buf_len++] = (int)(c), (int)(c)) : (putc_do(c)));
    }
    int flush();
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
#ifdef __linux__
    int __attribute__((format(gnu_printf, 2, 3))) printf_1024(const char *format, ...);
#else  // __linux__
    int printf_1024(const char *format, ...);
#endif // __attribute__
    int get_cint();
    int write_cint(int len);
    int write_cint_and_int(int i);
    int write_cint_and_long(int64_t l);
    int write_cint_and_data(const void *buf, int len);
    inline int write_cint_and_pp(const char **pp, int size);

protected:
    virtual int engine_read(void *buf, int len) = 0;
    virtual int engine_write(const void *buf, int len) = 0;

private:
    int getc_do();
    int putc_do(int ch);

protected:
    stream_engine *engine_{nullptr};
};

class iostream : public stream
{
public:
    struct connect_options
    {
        const char *destination{nullptr};
        SSL_CTX *ssl_ctx{nullptr};
        int retry_times{1};
    };

public:
    iostream();
    iostream(int fd);
    iostream(SSL *ssl);
    virtual ~iostream();
    iostream &open_socket(int fd);
    iostream &open_ssl(SSL *ssl);
    bool connect(const char *destination);
    inline bool connect(const std::string &destination)
    {
        return connect(destination.c_str());
    }
    bool connect(const connect_options &options);

public:
    virtual inline int get_socket() { return fd_; }
    virtual iostream &set_timeout(int wait_timeout);
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

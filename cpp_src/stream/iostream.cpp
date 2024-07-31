/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2021-04-15
 * ================================
 */

#include "zcc/zcc_stream.h"

zcc_namespace_begin;

iostream &iostream::open_socket(int fd)
{
    if (fd_ != fd)
    {
        close(true);
        fd_ = fd;
    }
    operate_nonblocking();
    return *this;
}

iostream &iostream::open_ssl(SSL *ssl)
{
    if (ssl_ != ssl)
    {
        close(true);
        ssl_ = ssl;
        fd_ = openssl::SSL_get_fd(ssl);
    }
    operate_nonblocking();
    return *this;
}

bool iostream::connect(const char *destination)
{
    close(true);
    int fd = netpath_connect(destination, engine_->wait_timeout);
    if (fd < 0)
    {
        return false;
    }
    open_socket(fd);
    operate_nonblocking();
    return true;
}

bool iostream::connect(const connect_options &options)
{
    close(true);
    int fd = -1;
    for (int i = 0; i < options.retry_times; i++)
    {
        int fd = netpath_connect(options.destination, engine_->wait_timeout);
        if (fd > -1)
        {
            break;
        }
    }
    open_socket(fd);
    if (options.ssl_ctx)
    {

        if (tls_connect(options.ssl_ctx) < 0)
        {
            close(true);
            return false;
        }
    }
    operate_nonblocking();
    return true;
}

iostream::iostream()
{
    close(true);
}

iostream::iostream(int fd)
{
    open_socket(fd);
}

iostream::iostream(SSL *ssl)
{
    open_ssl(ssl);
}

iostream::~iostream()
{
    close(true);
}

int iostream::close(bool close_fd_or_release_ssl)
{
    int ret = 0;
    if (close_fd_or_release_ssl)
    {
        if (ssl_)
        {
            openssl::SSL_free(ssl_);
        }
        if (fd_ > -1)
        {
            ret = close_socket(fd_);
        }
    }
    ssl_ = 0;
    fd_ = -1;
    nonblocking_set_flag_ = false;
    nonblocking_flag_ = false;
    int timeout = engine_->wait_timeout;
    reset();
    set_timeout(timeout);
    return ret;
}

void iostream::operate_nonblocking()
{
    if (fd_ < 0)
    {
        return;
    }
    if (nonblocking_set_flag_)
    {
        if ((engine_->wait_timeout > 0) && nonblocking_flag_)
        {
            return;
        }
        if ((engine_->wait_timeout < 1) && (!nonblocking_flag_))
        {
            return;
        }
    }
    nonblocking_set_flag_ = true;
    if (engine_->wait_timeout > 0)
    {
        nonblocking_flag_ = true;
    }
    else
    {
        nonblocking_flag_ = false;
    }
    nonblocking(fd_, nonblocking_flag_);
}
iostream &iostream::set_timeout(int wait_timeout)
{
    engine_->wait_timeout = wait_timeout;
    operate_nonblocking();
    return *this;
}

int iostream::timed_read_wait(int wait_timeout)
{
    if (get_read_cache_len() > 0)
    {
        return 1;
    }
    return zcc::timed_read_wait(fd_, wait_timeout);
}

int iostream::timed_write_wait(int wait_timeout)
{
    return zcc::timed_write_wait(fd_, wait_timeout);
}

int iostream::engine_read(void *buf, int len)
{
    if (ssl_)
    {
        return openssl::timed_read(ssl_, buf, len, engine_->wait_timeout);
    }
    else
    {
        return timed_read(fd_, buf, len, engine_->wait_timeout);
    }
}

int iostream::engine_write(const void *buf, int len)
{
    if (ssl_)
    {
        return openssl::timed_write(ssl_, buf, len, engine_->wait_timeout);
    }
    else
    {
        return timed_write(fd_, buf, len, engine_->wait_timeout);
    }
}

int iostream::tls_connect(SSL_CTX *ctx)
{
    if (ssl_)
    {
        return -1;
    }

    SSL *_ssl = openssl::SSL_create(ctx, fd_);
    if (!_ssl)
    {
        return -1;
    }
    if (openssl::timed_connect(_ssl, engine_->wait_timeout) < 0)
    {
        openssl::SSL_free(_ssl);
        set_error();
        return -1;
    }
    ssl_ = _ssl;

    return 1;
}

int iostream::tls_accept(SSL_CTX *ctx)
{
    if (ssl_)
    {
        return -1;
    }
    if (get_read_cache_len() > 0)
    {
        // http://www.postfix.org/CVE-2011-0411.html
        return -1;
    }

    SSL *_ssl = openssl::SSL_create(ctx, fd_);
    if (!_ssl)
    {
        return -1;
    }
    if (openssl::timed_accept(_ssl, engine_->wait_timeout) < 0)
    {
        openssl::SSL_free(_ssl);
        set_error();
        return -1;
    }
    ssl_ = _ssl;
    fd_ = openssl::SSL_get_fd(ssl_);

    return 1;
}

zcc_namespace_end;

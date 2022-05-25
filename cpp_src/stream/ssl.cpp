/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2021-04-15
 * ================================
 */

#include "zc.h"

namespace zcc
{

ssl_iostream::ssl_iostream()
{
    ssl_ = 0;
}

ssl_iostream::~ssl_iostream()
{
    close(true);
}

int ssl_iostream::close(bool close_fd_or_release_ssl)
{
    int ret = 0;
    if (close_fd_or_release_ssl) {
        if (ssl_) {
            zopenssl_SSL_free(ssl_);
        }
        if (fd_ > -1) {
            ret = zclose(fd_);
        }
    }
    ssl_ = 0;
    fd_ = -1;
    read_wait_timeout_ = -1;
    write_wait_timeout_ = -1;
    return ret;
}

int ssl_iostream::engine_read(void *buf, int len)
{
    if (ssl_) {
        return zopenssl_timed_read(ssl_, buf, len, read_wait_timeout_, write_wait_timeout_);
    } else {
        return ztimed_read(fd_, buf, len, read_wait_timeout_);
    }
}

int ssl_iostream::engine_write(const void *buf, int len)
{
    if (ssl_) {
        return zopenssl_timed_write(ssl_, buf, len, read_wait_timeout_, write_wait_timeout_);
    } else {
        return ztimed_write(fd_, buf, len, write_wait_timeout_);
    }
}

int ssl_iostream::timed_read_wait(int read_wait_timeout)
{
    return ztimed_read_wait(fd_, read_wait_timeout_);
}

int ssl_iostream::timed_write_wait(int write_wait_timeout)
{
    return ztimed_write_wait(fd_, write_wait_timeout_);
}

ssl_iostream &ssl_iostream::open_ssl(SSL *ssl)
{
    close(true);
    ssl_ = ssl;
    fd_ = zopenssl_SSL_get_fd(ssl);
    return *this;
}

int ssl_iostream::tls_connect(SSL_CTX *ctx)
{
    if (ssl_) {
        return -1;
    }

    SSL *_ssl = zopenssl_SSL_create(ctx, fd_);
    if (!_ssl) {
        return -1;
    }
    if (zopenssl_timed_connect(_ssl, read_wait_timeout_, write_wait_timeout_) < 0) {
        zopenssl_SSL_free(_ssl);
        set_error();
        return -1;
    }
    ssl_ = _ssl;

    return 1;
}

int ssl_iostream::tls_accept(SSL_CTX *ctx)
{
    if (ssl_) {
        return -1;
    }

    SSL *_ssl = zopenssl_SSL_create(ctx, fd_);
    if (!_ssl) {
        return -1;
    }
    if (zopenssl_timed_accept(_ssl, read_wait_timeout_, write_wait_timeout_) < 0) {
        zopenssl_SSL_free(_ssl);
        set_error();
        return -1;
    }
    ssl_ = _ssl;

    return 1;
}

} /* namespace zcc */


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

static int _close(iostream &fp, bool release)
{
    int ret = 0;
    if (release) {
        if (fp.get_ssl()) {
            zopenssl_SSL_free(fp.get_ssl());
        }
        ret = zclose(fp.get_fd());
    }
    return ret;
}

static int _read(iostream &fp, void *buf, int len)
{
    if (fp.get_ssl()) {
        return zopenssl_timed_read(fp.get_ssl(), buf, len, fp.get_read_wait_timeout(), fp.get_read_wait_timeout());
    } else {
        return ztimed_read(fp.get_fd(), buf, len, fp.get_read_wait_timeout());
    }
}

static int _write(iostream &fp, const void *buf, int len)
{
    if (fp.get_ssl()) {
        return zopenssl_timed_write(fp.get_ssl(), buf, len, fp.get_write_wait_timeout(), fp.get_write_wait_timeout());
    } else {
        return ztimed_write(fp.get_fd(), buf, len, fp.get_write_wait_timeout());
    }

}

static int _timed_read_wait(iostream &fp, int read_wait_timeout)
{
    return ztimed_read_wait(fp.get_fd(), fp.get_read_wait_timeout());
}

static int _timed_write_wait(iostream &fp, int write_wait_timeout)
{
    return ztimed_write_wait(fp.get_fd(), fp.get_write_wait_timeout());
}

#define _SET_HANDLER_() { \
    engine_read_ = _read; \
    engine_write_ = _write; \
    engine_timed_read_wait_ = _timed_read_wait; \
    engine_timed_write_wait_ = _timed_write_wait; \
    engine_close_ = _close; \
}

iostream &iostream::open_ssl(SSL *ssl)
{
    close(1);
    ssl_ = ssl;
    fd_ = zopenssl_SSL_get_fd(ssl);
    _SET_HANDLER_();
    return *this;
}

int iostream::tls_connect(SSL_CTX *ctx)
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
    _SET_HANDLER_();

    return 1;
}

int iostream::tls_accept(SSL_CTX *ctx)
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
    _SET_HANDLER_();

    return 1;
}

} /* namespace zcc */


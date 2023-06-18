/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-10-19
 * ================================
 */

#include "zc.h"

typedef struct ssl_ioctx_t ssl_ioctx_t;
struct ssl_ioctx_t {
    SSL *ssl;
    int fd;
};

static const char *fp_get_type()
{
    return "_Z_SSL_TLS";
}

static int fp_close(zstream_t *fp, int release_ioctx)
{
    ssl_ioctx_t *ioctx = (ssl_ioctx_t *)(fp->ioctx);

    int ret = 0;
    if (release_ioctx) {
        if (ioctx->ssl) {
            zopenssl_SSL_free(ioctx->ssl);
        }
        ret = zclosesocket(ioctx->fd);
    }

    zfree(ioctx);

    return ret;
}

static int fp_read(zstream_t *fp, void *buf, int len)
{
    ssl_ioctx_t *ioctx = (ssl_ioctx_t *)(fp->ioctx);

    if (ioctx->ssl) {
        return zopenssl_timed_read(ioctx->ssl, buf, len, fp->read_wait_timeout, fp->read_wait_timeout);
    } else {
        return ztimed_read(ioctx->fd, buf, len, fp->read_wait_timeout);
    }
}

static int fp_write(zstream_t *fp, const void *buf, int len)
{
    ssl_ioctx_t *ioctx = (ssl_ioctx_t *)(fp->ioctx);

    if (ioctx->ssl) {
        return zopenssl_timed_write(ioctx->ssl, buf, len, fp->write_wait_timeout, fp->write_wait_timeout);
    } else {
        return ztimed_write(ioctx->fd, buf, len, fp->write_wait_timeout);
    }
}

static int fp_timed_read_wait(zstream_t *fp, int read_wait_timeout)
{
    ssl_ioctx_t *ioctx = (ssl_ioctx_t *)(fp->ioctx);

    return ztimed_read_wait(ioctx->fd, read_wait_timeout);
}

static int fp_timed_write_wait(zstream_t *fp, int write_wait_timeout)
{
    ssl_ioctx_t *ioctx = (ssl_ioctx_t *)(fp->ioctx);

    return ztimed_write_wait(ioctx->fd, write_wait_timeout);
}

static int fp_get_fd(zstream_t *fp)
{
    ssl_ioctx_t *ioctx = (ssl_ioctx_t *)(fp->ioctx);

    return ioctx->fd;
}

static zstream_engine_t fp_engine = {
    fp_get_type,
    fp_close,
    fp_read,
    fp_write,
    fp_timed_read_wait,
    fp_timed_write_wait,
    fp_get_fd
};

zstream_t *zstream_open_ssl_engine(zstream_t *fp, SSL *ssl)
{
    memset(fp, 0, sizeof(zstream_t));
    fp->read_buf_p1 = 0;
    fp->read_buf_p2 = 0;
    fp->write_buf_len = 0;
    fp->read_wait_timeout = -1;
    fp->write_wait_timeout = -1;
    fp->error = 0;
    fp->eof = 0;
    fp->engine = &fp_engine;
    ssl_ioctx_t *ioctx = (ssl_ioctx_t *)zcalloc(1, sizeof(ssl_ioctx_t));
    ioctx->ssl = ssl;
    ioctx->fd = zopenssl_SSL_get_fd(ssl);
    fp->ioctx = ioctx;
    return fp;
}

zstream_t *zstream_open_ssl(SSL *ssl)
{
    return zstream_open_ssl_engine((zstream_t *)zmalloc(sizeof(zstream_t)), ssl);
}

SSL *zstream_get_ssl(zstream_t *fp)
{
    ssl_ioctx_t *ioctx = (ssl_ioctx_t *)(fp->ioctx);
    
    return ioctx->ssl;
}

int zstream_tls_connect(zstream_t *fp, SSL_CTX *ctx)
{
    ssl_ioctx_t *ioctx = (ssl_ioctx_t *)(fp->ioctx);

    if (ioctx->ssl) {
        return -1;
    }

    SSL *_ssl = zopenssl_SSL_create(ctx, ioctx->fd);
    if (!_ssl) {
        return -1;
    }
    if (zopenssl_timed_connect(_ssl, fp->read_wait_timeout, fp->write_wait_timeout) < 0) {
        zopenssl_SSL_free(_ssl);
        fp->error = 1;
        return -1;
    }
    ioctx->ssl = _ssl;

    fp->engine = &fp_engine;

    return 1;
}

int zstream_tls_accept(zstream_t *fp, SSL_CTX *ctx)
{
    ssl_ioctx_t *ioctx = (ssl_ioctx_t *)(fp->ioctx);

    if (ioctx->ssl) {
        return -1;
    }

    SSL *_ssl = zopenssl_SSL_create(ctx, ioctx->fd);
    if (!_ssl) {
        return -1;
    }
    if (zopenssl_timed_accept(_ssl, fp->read_wait_timeout, fp->write_wait_timeout) < 0) {
        zopenssl_SSL_free(_ssl);
        fp->error = 1;
        return -1;
    }
    ioctx->ssl = _ssl;

    fp->engine = &fp_engine;

    return 1;
}

/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-10-18
 * ================================
 */

#include "libzc.h"
#include <openssl/ssl.h>
#include <openssl/err.h>

int zssl_INIT(int unused_flags)
{
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    return 0;
}

zsslctx_t *zsslctx_server_create(int unused_flags)
{
    zsslctx_t *zctx = 0;
    SSL_CTX *ctx = 0;

    ctx = SSL_CTX_new(SSLv23_server_method());
    if (!ctx) {
        return 0;
    }

    zctx = (zsslctx_t *) zcalloc(1, sizeof(zsslctx_t));
    zctx->ssl_ctx = ctx;
    zctx->server_or_client = 1;

    return zctx;

}

zsslctx_t *zsslctx_client_create(int unused_flags)
{
    zsslctx_t *zctx = 0;
    SSL_CTX *ctx = 0;

    ctx = SSL_CTX_new(SSLv23_client_method());
    if (!ctx) {
        return 0;
    }

    zctx = (zsslctx_t *) zcalloc(1, sizeof(zsslctx_t));
    zctx->ssl_ctx = ctx;
    zctx->server_or_client = 0;

    return zctx;

}

int zsslctx_set_cert(zsslctx_t * ssl_ctx, const char *cert_file, const char *key_file)
{
    ERR_clear_error();
    if ((!cert_file) || (SSL_CTX_use_certificate_chain_file(ssl_ctx->ssl_ctx, cert_file) <= 0)) {
        return (-1);
    }
    if ((!key_file) || (SSL_CTX_use_PrivateKey_file(ssl_ctx->ssl_ctx, key_file, SSL_FILETYPE_PEM) <= 0)) {
        return (-1);
    }
    if (!SSL_CTX_check_private_key(ssl_ctx->ssl_ctx)) {
        return (-1);
    }

    return 0;
}

void zsslctx_free(zsslctx_t * ctx)
{
    if (ctx->ssl_ctx) {
        SSL_CTX_free(ctx->ssl_ctx);
    }
    zfree(ctx);
}

void zssl_get_error(unsigned long *ecode, char *buf, int buf_len)
{
    unsigned long ec;
    ec = ERR_get_error();
    if (ecode) {
        *ecode = ec;
    }

    if (buf) {
        ERR_error_string_n(ec, buf, buf_len);
    }
}

zssl_t *zssl_create(zsslctx_t * ctx, int fd)
{
    zssl_t *zssl;
    SSL *ssl;

    ssl = SSL_new(ctx->ssl_ctx);
    SSL_set_fd(ssl, fd);

    zssl = (zssl_t *) zcalloc(1, sizeof(zssl_t));
    zssl->ssl = ssl;
    zssl->fd = fd;
    zssl->server_or_client = ctx->server_or_client;

    return zssl;
}

void *zssl_detach_ssl(zssl_t * zssl)
{
    SSL *ssl;

    ssl = zssl->ssl;

    zssl->ssl = 0;

    return ssl;
}

void zssl_free(zssl_t * ssl)
{
    if (ssl->ssl) {
        SSL_shutdown(ssl->ssl);
        SSL_free(ssl->ssl);
    }
    zfree(ssl);
}

static inline int zssl_timed_do(zssl_t * zssl, int (*hsfunc) (SSL *), int (*rfunc) (SSL *, void *, int), int (*wfunc) (SSL *, const void *, int), void *buf, int num, int timeout)
{
    int ret;
    int status;
    int err;
    SSL *ssl = zssl->ssl;
    long start_time;
    long left_time;
    int fd = zssl->fd;

    start_time = ztimeout_set(timeout);

    for (;;) {
        if (hsfunc) {
            status = hsfunc(ssl);
        } else if (rfunc) {
            status = rfunc(ssl, buf, num);
        } else if (wfunc) {
            status = wfunc(ssl, buf, num);
        } else {
            zfatal("zssl_timed_do: nothing to do here");
        }
        err = SSL_get_error(ssl, status);

        switch (err) {
        case SSL_ERROR_WANT_WRITE:
            if ((left_time = ztimeout_left(start_time)) < 1) {
                return -1;
            }
            if ((ret = zwrite_wait(fd, left_time)) < 0) {
                return -1;
            }
            break;
        case SSL_ERROR_WANT_READ:
            if ((left_time = ztimeout_left(start_time)) < 1) {
                return -1;
            }
            if ((ret = zread_wait(fd, left_time)) < 0) {
                return (ret);
            }
            break;

        default:
            zdebug("zssl_timed_do: unexpected SSL_ERROR code %d", err);
        case SSL_ERROR_SSL:
            zdebug("zssl_timed_do: SSL_ERROR_SSL");
            /* FIXME */
        case SSL_ERROR_ZERO_RETURN:
        case SSL_ERROR_NONE:
            errno = 0;
        case SSL_ERROR_SYSCALL:
            return (status);
        }
    }

    return 0;
}

int zssl_connect(zssl_t * ssl, int timeout)
{
    return zssl_timed_do(ssl, SSL_connect, 0, 0, 0, 0, timeout);
}

int zssl_accept(zssl_t * ssl, int timeout)
{
    return zssl_timed_do(ssl, SSL_accept, 0, 0, 0, 0, timeout);
}

int zssl_shutdown(zssl_t * ssl, int timeout)
{
    return zssl_timed_do(ssl, SSL_shutdown, 0, 0, 0, 0, timeout);
}

int zssl_read(zssl_t * ssl, void *buf, int len, int timeout)
{
    return zssl_timed_do(ssl, 0, SSL_read, 0, buf, len, timeout);
}

int zssl_write(zssl_t * ssl, void *buf, int len, int timeout)
{
    return zssl_timed_do(ssl, 0, 0, SSL_write, buf, len, timeout);
}

void *___zopenssl_create(zsslctx_t * ctx, int fd)
{
    SSL *ssl;

    ssl = SSL_new(ctx->ssl_ctx);
    SSL_set_fd(ssl, fd);

    return ssl;
}

void zopenssl_free(void *ssl)
{
    SSL_shutdown(ssl);
    SSL_free(ssl);
}

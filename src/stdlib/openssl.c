/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-10-18
 * ================================
 */

#include "zc.h"
#include <openssl/ssl.h>
#include <openssl/err.h>

#define zdebug(fmt, args...) {if(zvar_openssl_debug){zinfo(fmt, ##args);}}

int zvar_openssl_debug = 0;
void zopenssl_init(void)
{
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
}

SSL_CTX *zopenssl_create_SSL_CTX_server(void)
{
     return SSL_CTX_new(SSLv23_server_method());
}


SSL_CTX *zopenssl_create_SSL_CTX_client(void)
{
     return SSL_CTX_new(SSLv23_client_method());
}

int zopenssl_SSL_CTX_set_cert(SSL_CTX *ctx, const char *cert_file, const char *key_file)
{
    ERR_clear_error();
    if ((!cert_file) || (SSL_CTX_use_certificate_chain_file(ctx, cert_file) <= 0)) {
        return (-1);
    }
    if ((!key_file) || (SSL_CTX_use_PrivateKey_file(ctx, key_file, SSL_FILETYPE_PEM) <= 0)) {
        return (-1);
    }
    if (!SSL_CTX_check_private_key(ctx)) {
        return (-1);
    }

    return 0;
}

void zopenssl_SSL_CTX_free(SSL_CTX * ctx)
{
    SSL_CTX_free(ctx);
}

void zopenssl_get_error(unsigned long *ecode, char *buf, int buf_len)
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

SSL *zopenssl_create_SSL(SSL_CTX * ctx, int fd)
{
    SSL *ssl;
    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, fd);
    return ssl;
}

void zopenssl_SSL_free(SSL * ssl)
{
    SSL_shutdown(ssl);
    SSL_free(ssl);
}

int zopenssl_SSL_get_fd(SSL *ssl)
{
    return SSL_get_fd(ssl);
}

static inline int zopenssl_timed_do(SSL *ssl, int (*hsfunc) (SSL *), int (*rfunc) (SSL *, void *, int), int (*wfunc) (SSL *, const void *, int), void *buf, int num, long timeout)
{
    int ret;
    int status;
    int err;
    long start_time;
    long left_time;
    int fd = SSL_get_fd(ssl);

    start_time = ztimeout_set(timeout);

    for (;;) {
        if (hsfunc) {
            status = hsfunc(ssl);
        } else if (rfunc) {
            status = rfunc(ssl, buf, num);
        } else if (wfunc) {
            status = wfunc(ssl, buf, num);
        } else {
            zfatal("zopenssl_timed_do: nothing to do here");
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
            zdebug("zopenssl_timed_do: unexpected SSL_ERROR code %d", err);
        case SSL_ERROR_SSL:
            zdebug("zopenssl_timed_do: SSL_ERROR_SSL");
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

int zopenssl_connect(SSL * ssl, long timeout)
{
    return zopenssl_timed_do(ssl, SSL_connect, 0, 0, 0, 0, timeout);
}

int zopenssl_accept(SSL * ssl, long timeout)
{
    return zopenssl_timed_do(ssl, SSL_accept, 0, 0, 0, 0, timeout);
}

int zopenssl_shutdown(SSL * ssl, long timeout)
{
    return zopenssl_timed_do(ssl, SSL_shutdown, 0, 0, 0, 0, timeout);
}

int zopenssl_read(SSL * ssl, void *buf, int len, long timeout)
{
    return zopenssl_timed_do(ssl, 0, SSL_read, 0, buf, len, timeout);
}

int zopenssl_write(SSL * ssl, const void *buf, int len, long timeout)
{
    return zopenssl_timed_do(ssl, 0, 0, SSL_write, (void *)buf, len, timeout);
}

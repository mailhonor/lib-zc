/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2017-07-18
 * ================================
 */

#include "zc.h"
#include <errno.h>
#include <poll.h>
#include <openssl/ssl.h>

typedef struct fd_attrs_t fd_attrs_t;
struct fd_attrs_t {
    SSL *ssl;
    int fd;
    unsigned char want_read:1;
    unsigned char want_write:1;
    unsigned char error_or_closed:1;
};

static void fd_attrs_init(fd_attrs_t *fa)
{
    memset(fa, 0, sizeof(fd_attrs_t));
    fa->ssl = 0;
    fa->fd = -1;
    fa->want_read = 0;
    fa->want_write = 0;
    fa->error_or_closed = 0;
}

static void fd_attrs_fini(fd_attrs_t *fa)
{
    if (fa->ssl) {
        zopenssl_SSL_free(fa->ssl);
        fa->ssl = 0;
    }
    if (fa->fd != -1) {
        zclose(fa->fd);
        fa->fd = -1;
    }
}

void fd_attrs_prepare_get_data(fd_attrs_t *fa, struct pollfd *pf)
{
    pf->fd = fa->fd;
    if (fa->want_read) {
        pf->events = POLLIN;
    } else if (fa->want_write) {
        pf->events = POLLOUT;
    } else {
        zfatal("unknown want event");
    }
    pf->revents = 0;
}

int fd_attrs_try_read(fd_attrs_t *fa, char *buf, int size)
{
    int ret;
    fa->want_read = 0;
    fa->want_write = 0;
    if (fa->ssl) {
        ret = SSL_read(fa->ssl, buf, size);
        if (ret > 0) {
            return ret;
        }
        int rr = SSL_get_error(fa->ssl, ret);
        if (rr == SSL_ERROR_WANT_READ) {
            fa->want_read = 1;
        } else if (rr == SSL_ERROR_WANT_WRITE) {
            fa->want_write = 1;
        } else {
            fa->error_or_closed = 1;
        }
        return ret;
    } else {
        ret = read(fa->fd, buf, size);
        if (ret == 0) {
            fa->error_or_closed = 1;
        }
        if (ret >= 0) {
            return ret;
        }
        int ec = errno;
        if ((ec != EAGAIN) && (ec != EINTR)) {
            fa->error_or_closed = 1;
            return -1;
        }
        fa->want_read = 1;
        return -1;
    }
}

int fd_attrs_strict_write(fd_attrs_t *fa, char *buf, int size)
{
    int ret;
    size_t wrotelen = 0;
    while(wrotelen < size) {
        if (fa->ssl) {
            ret = SSL_write(fa->ssl, buf + wrotelen, size - wrotelen);
            if (ret > 0) {
                wrotelen += ret;
                continue;
            }
            int rr = SSL_get_error(fa->ssl, ret);
            if (rr == SSL_ERROR_WANT_READ) {
                ztimed_read_wait_millisecond(fa->fd, 10 * 1000);
                continue;
            } else if (rr == SSL_ERROR_WANT_WRITE) {
                ztimed_write_wait_millisecond(fa->fd, 10 * 1000);
                continue;
            } else {
                fa->error_or_closed = 1;
                return 0;
            }
        } else {
            ret = write(fa->fd, buf + wrotelen, size - wrotelen);
            if (ret >= 0) {
                wrotelen += ret;
                continue;
            }
            int ec = errno;
            if ((ec != EAGAIN) && (ec != EINTR)) {
                fa->error_or_closed = 1;
                return 0;
            }
            ztimed_write_wait_millisecond(fa->fd, 10 * 1000);
            continue;
        }
    }
    return 1;
}

typedef struct fd_attrs_go_t fd_attrs_go_t;
struct fd_attrs_go_t {
    fd_attrs_t fass0;
    fd_attrs_t fass1;
    void (*after_close)(void *ctx);
    void *ctx;
    char rbuf[4096+1];
};

static void *coroutine_go_iopipe_go(void *ctx)
{
    fd_attrs_go_t *fgo = (fd_attrs_go_t *)ctx;
    int need_stop = 0;
    int no_pool = 0;
    struct pollfd pollfds[2];
    fd_attrs_t *fass0 = &(fgo->fass0);
    fd_attrs_t *fass1 = &(fgo->fass1);
    fd_attrs_t *fass_w;
    char *rbuf = fgo->rbuf;
    int rlen, poll_ret;

    if (fass0->ssl) {
        pollfds[0].revents = POLLIN;
        no_pool = 1;
    }
    if (fass1->ssl) {
        pollfds[1].revents = POLLIN;
        no_pool = 1;
    }

    while(1) {
        rlen = 0;
        fass_w = 0;
        if (no_pool) {
            poll_ret = 1;
        } else {
            fd_attrs_prepare_get_data(fass0, pollfds);
            fd_attrs_prepare_get_data(fass1, pollfds+1);
            poll_ret = poll(pollfds, 2, 10 * 1000);
        }
        switch (poll_ret) {
        case -1:
            if (errno != EINTR) {
                need_stop = 1;
                break;
            }
            continue;
        case 0:
            continue;
        default:
            if (pollfds[0].revents & (POLLIN | POLLOUT)) {
                rlen = fd_attrs_try_read(fass0, rbuf, 4096);
                if (rlen > 0) {
                    pollfds[0].revents = POLLIN;
                    pollfds[1].revents = 0;
                    no_pool = 1;
                    fass_w = &(fgo->fass1);
                    break;
                }
                if (rlen == 0) {
                    need_stop = 1;
                    break;
                }
            }
            if ((rlen <= 0) && (pollfds[1].revents & (POLLIN | POLLOUT))) {
                rlen = fd_attrs_try_read(fass1, rbuf, 4096);
                if (rlen > 0) {
                    fass_w = &(fgo->fass0);
                    pollfds[0].revents = 0;
                    pollfds[1].revents = POLLIN;
                    no_pool = 1;
                    break;
                }
                if (rlen == 0) {
                    need_stop = 1;
                    break;
                }
            }
            no_pool = 0;
            break;
        }
        if (need_stop) {
            break;
        }

        if (fass0->error_or_closed || fass1->error_or_closed) {
            break;
        }

        if (rlen < 0) {
            continue;
        }
        if (fass_w && (!fd_attrs_strict_write(fass_w, rbuf, rlen))) {
            break;
        }

        no_pool = 0;
        fass0->want_read = 1;
        fass0->want_write = 0;
        fass1->want_read = 1;
        fass1->want_write = 0;
    }
    if (fgo->after_close) {
        fgo->after_close(fgo->ctx);
    }
    fd_attrs_fini(&(fgo->fass0));
    fd_attrs_fini(&(fgo->fass1));
    zfree(fgo);
    return 0;
}

void zcoroutine_go_iopipe(int fd1, SSL *ssl1, int fd2, SSL *ssl2, void (*after_close)(void *ctx), void *ctx)
{
    fd_attrs_go_t *fgo = (fd_attrs_go_t *)zmalloc(sizeof(fd_attrs_go_t));
    fd_attrs_init(&(fgo->fass0));
    fd_attrs_init(&(fgo->fass1));

    fgo->fass0.fd = fd1;
    fgo->fass0.ssl = ssl1;
    fgo->fass0.want_read = 1;
    fgo->fass1.fd = fd2;
    fgo->fass1.ssl = ssl2;
    fgo->fass1.want_read = 1;
    fgo->after_close = after_close;
    fgo->ctx = ctx;

    zcoroutine_go(coroutine_go_iopipe_go, fgo, 16 * 1024);
}

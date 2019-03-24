/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-11-08
 * ================================
 */

#include "zc.h"
#include <openssl/ssl.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>

#define ZIOPIPE_RBUF_SIZE	    	4096
#define ZIOPIPE_BASE_LOCK(iopb)		zpthread_lock(&((iopb)->locker))
#define ZIOPIPE_BASE_UNLOCK(iopb)	zpthread_unlock(&((iopb)->locker))

typedef struct ziopipe_t ziopipe_t;
typedef struct ziopipe_part_t ziopipe_part_t;
typedef struct ziopipe_linker_t ziopipe_linker_t;

struct ziopipe_part_t {
    unsigned char is_client_or_server:1;
    unsigned char read_want_read:1;
    unsigned char read_want_write:1;
    unsigned char write_want_read:1;
    unsigned char write_want_write:1;
    unsigned char ssl_error:1;
    unsigned char event_in:1;
    unsigned char et_read:1;
    unsigned char et_write:1;
    unsigned char et_hup:1;
    int rbuf_p1:16;
    int rbuf_p2:16;
    int fd;
    char *rbuf;
    SSL *ssl;
};

struct ziopipe_t {
    ziopipe_part_t client;
    ziopipe_part_t server;
    ziopipe_after_close_fn_t after_close;
    void *context;
};

struct ziopipe_linker_t {
    int cfd;
    int sfd;
    SSL *cssl;
    SSL *sssl;
    ziopipe_linker_t *prev;
    ziopipe_linker_t *next;
    ziopipe_after_close_fn_t after_close;
    void *context;
};

struct ziopipe_base_t {
    unsigned int break_flag:1;
    int epoll_fd;
    int epoll_event_count;
    struct epoll_event *epoll_event_list;
    pthread_mutex_t locker;
    ziopipe_linker_t *set_list_head;
    ziopipe_linker_t *set_list_tail;
    zmpiece_t *rbuf_mpool;
    zmpiece_t *iop_mpool;
    ziopipe_t eventfd_iop;
};

static void ziopipe_set_event(ziopipe_base_t * iopb, ziopipe_part_t * part, int enable)
{
    int fd, event_in, e_events;
    struct epoll_event evt;

    fd = part->fd;
    e_events = EPOLLHUP | EPOLLRDHUP | EPOLLERR | EPOLLET | EPOLLIN| EPOLLOUT;

    event_in = part->event_in;
    part->event_in = enable;
    if (enable == 0) {
        if (event_in) {
            if (epoll_ctl(iopb->epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1) {
                zfatal("ziopipe_set_event: fd %d: DEL  error: %m", fd);
            }
        }
    } else if (event_in != enable) {
        evt.events = e_events;
        evt.data.ptr = part;
        if (epoll_ctl(iopb->epoll_fd, EPOLL_CTL_ADD, fd, &evt) == -1) {
            zfatal("ziopipe_set_event: fd %d: ADD error: %m", fd);
        }
    }
}

static inline int try_ssl_write(ziopipe_part_t * part, void *buf, int len)
{
    int rlen, status, eo;

    rlen = SSL_write(part->ssl, buf, len);
    if (rlen < 1) {
        eo = errno;
        status = SSL_get_error(part->ssl, rlen);
       if (status == SSL_ERROR_WANT_WRITE) {
           part->write_want_write = 0;
           part->write_want_read = 0;
            part->write_want_write = 1;
        } else if (status == SSL_ERROR_WANT_READ) {
           part->write_want_write = 0;
           part->write_want_read = 0;
            part->write_want_read = 1;
        } else if (status == SSL_ERROR_SYSCALL) {
            if (eo && (eo != EAGAIN)) {
                part->ssl_error = 1;
            }
        } else {
            part->ssl_error = 1;
        }
        return -1;
    }

    return rlen;
}

static inline int try_ssl_read(ziopipe_part_t * part, void *buf, int len)
{
    int rlen, status, eo;

    rlen = SSL_read(part->ssl, buf, len);
    if (rlen < 1) {
        eo = errno;
        status = SSL_get_error(part->ssl, rlen);
        if (status == SSL_ERROR_WANT_WRITE) {
            part->read_want_write = 0;
            part->read_want_read = 0;
            part->read_want_write = 1;
        } else if (status == SSL_ERROR_WANT_READ) {
            part->read_want_write = 0;
            part->read_want_read = 0;
            part->read_want_read = 1;
        } else if (status == SSL_ERROR_SYSCALL) {
            if (eo && (eo != EAGAIN)) {
                part->ssl_error = 1;
            }
        } else {
            part->ssl_error = 1;
        }
        return -1;
    }
    return rlen;
}

ziopipe_base_t *ziopipe_base_create(void)
{
    ziopipe_base_t *iopb;
    int efd;

    iopb = (ziopipe_base_t *) zcalloc(1, sizeof(ziopipe_base_t));

    pthread_mutex_init(&(iopb->locker), 0);

    iopb->iop_mpool = zmpiece_create(sizeof(ziopipe_t));
    iopb->rbuf_mpool = zmpiece_create(ZIOPIPE_RBUF_SIZE);

    iopb->epoll_fd = epoll_create(1024);
    zclose_on_exec(iopb->epoll_fd, 1);

    iopb->epoll_event_count = 4096;
    iopb->epoll_event_list = (struct epoll_event *)zmalloc(sizeof(struct epoll_event) * iopb->epoll_event_count);

    efd = eventfd(0, 0);
    znonblocking(efd, 1);
    zclose_on_exec(efd, 1);

    iopb->eventfd_iop.client.fd = efd;
    iopb->eventfd_iop.client.is_client_or_server = 0;
    ziopipe_set_event(iopb, &(iopb->eventfd_iop.client), 1);

    return iopb;
}

void ziopipe_base_notify_stop(ziopipe_base_t * iopb)
{
    iopb->break_flag = 1;
}

static inline void write_read_loop(ziopipe_base_t *iopb, ziopipe_part_t *part, ziopipe_part_t *part_a, int len)
{
    int have_data = 1, wlen, rlen;
    if (len < 1) {
        have_data = 0;
    }
    while(1) {
        if (have_data == 1) {
            if (part->ssl) {
                wlen = try_ssl_write(part, part_a->rbuf + part_a->rbuf_p1, len);
            } else {
                wlen = (int)write(part->fd, part_a->rbuf + part_a->rbuf_p1, (size_t) len);
            }
            if (wlen > 0) {
                part_a->rbuf_p1 += wlen;
                if (len == wlen) {
                    part_a->rbuf_p1 = part_a->rbuf_p2 = 0;
                    have_data = 0;
                    continue;
                }
            } else if (part->ssl) {
                if (errno == EAGAIN) {
                    if (part->write_want_write) {
                        part->et_write = 0;
                    }
                    if (part->write_want_read) {
                        part->et_read = 0;
                    }
                }
                return;
            } else {
                if (errno == EAGAIN) {
                    part->et_write = 0;
                }
                return;
            }
        } else {
            if (part_a->rbuf==0) {
                part_a->rbuf = (char *)zmpiece_alloc_one(iopb->rbuf_mpool);
            }
            if (part_a->ssl) {
                rlen = try_ssl_read(part_a, part_a->rbuf, ZIOPIPE_RBUF_SIZE);
            } else {
                rlen = read(part_a->fd, part_a->rbuf, ZIOPIPE_RBUF_SIZE);
            }
            if (rlen > 0) {
                part_a->rbuf_p2 = rlen;
                len = rlen;
                have_data = 1;
                continue;
            }
            zmpiece_free_one(iopb->rbuf_mpool, part_a->rbuf);
            part_a->rbuf = 0;
            if (part_a->ssl) {
                if (errno == EAGAIN) {
                    if (part_a->read_want_write) {
                        part_a->et_write = 0;
                    }
                    if (part_a->read_want_read) {
                        part_a->et_read = 0;
                    }
                }
                return;
            } else {
                if (errno == EAGAIN) {
                    part_a->et_read = 0;
                }
                return;
            }
        }
    }
}

static inline void read_write_loop(ziopipe_base_t *iopb, ziopipe_part_t *part, ziopipe_part_t *part_a, int len)
{
    int have_data = 1, rlen, wlen;

    if (len == 0) {
        part->rbuf_p1 = part->rbuf_p2 = 0;
        have_data = 0;
    }
    if (!(part->rbuf)) {
        part->rbuf = (char *)zmpiece_alloc_one(iopb->rbuf_mpool);
    }

    while(1) {
        if (have_data == 0) {
            part->rbuf_p1 = 0;
            if (part->ssl) {
                rlen = try_ssl_read(part, part->rbuf, ZIOPIPE_RBUF_SIZE);
            } else {
                rlen = read(part->fd, part->rbuf, ZIOPIPE_RBUF_SIZE);
            }

            if (rlen > 0) {
                part->rbuf_p2 = rlen;
                have_data = 1;
                continue;
            } else if (part->ssl) {
                if (errno == EAGAIN) {
                    if (part->read_want_write) {
                        part->et_write = 0;
                    }
                    if (part->read_want_read) {
                        part->et_read = 0;
                    }
                }
                return;
            } else {
                if (errno == EAGAIN) {
                    part->et_read = 0;
                }
                return;
            }
        } else {
            rlen =  part->rbuf_p2 - part->rbuf_p1;
            if (part_a->ssl) {
                wlen = try_ssl_write(part_a, part->rbuf + part->rbuf_p1, rlen);
            } else {
                wlen = (int)write(part_a->fd, part->rbuf + part->rbuf_p1, (size_t) rlen);
            }
            if (wlen > 0) {
                if (rlen == wlen) {
                    part->rbuf_p1 = part->rbuf_p2 = 0;
                    have_data = 0;
                    continue;
                }
                have_data = 1;
                continue;
            }
            if (part_a->ssl) {
                if (errno == EAGAIN) {
                    if (part_a->read_want_write) {
                        part_a->et_write = 0;
                    }
                    if (part_a->read_want_read) {
                        part_a->et_read = 0;
                    }
                }
                return;
            } else {
                if (errno == EAGAIN) {
                    part_a->et_write = 0;
                }
                return;
            }
            return;
        }
    }
}

int ziopipe_base_run(ziopipe_base_t * iopb)
{
    int i, nfds, events, is_client_or_server, e_err;
    struct epoll_event *ev;
    ziopipe_t *iop;
    ziopipe_part_t *part, *part_a, *part_client, *part_server;
    int efd;
    ziopipe_linker_t *linker, *h, *t;

#define  ___no_indent_while_beign   while(1) {
#define  ___no_indent_while_end     }

    efd = iopb->eventfd_iop.client.fd;

    ___no_indent_while_beign;

    while (iopb->set_list_head) {
        ZIOPIPE_BASE_LOCK(iopb);
        linker = iopb->set_list_head;
        if (!linker) {
            ZIOPIPE_BASE_UNLOCK(iopb);
            break;
        }
        h = (ziopipe_linker_t *) (iopb->set_list_head);
        t = (ziopipe_linker_t *) (iopb->set_list_tail);
        ZMLINK_DETACH(h, t, linker, prev, next);
        iopb->set_list_head = h;
        iopb->set_list_tail = t;
        ZIOPIPE_BASE_UNLOCK(iopb);

        int cfd, sfd;
        SSL *cssl, *sssl;
        ziopipe_after_close_fn_t after_close;
        void *context;
        cfd = linker->cfd;
        sfd = linker->sfd;
        cssl = linker->cssl;
        sssl = linker->sssl;
        after_close = linker->after_close;
        context = linker->context;
        iop = (ziopipe_t *) linker;
        memset(iop, 0, sizeof(ziopipe_t));
        iop->client.fd = cfd;
        iop->client.ssl = cssl;
        iop->client.is_client_or_server = 0;
        iop->server.fd = sfd;
        iop->server.ssl = sssl;
        iop->server.is_client_or_server = 1;
        if (cssl) {
            iop->client.read_want_read = 1;
        }
        if (sssl) {
            iop->server.read_want_read = 1;
        }
        iop->after_close = after_close;
        iop->context = context;
        ziopipe_set_event(iopb, &(iop->client), 1);
        ziopipe_set_event(iopb, &(iop->server), 1);
    }

    nfds = epoll_wait(iopb->epoll_fd, iopb->epoll_event_list, iopb->epoll_event_count, 10 * 1000);
    if (nfds == -1) {
        if (errno != EINTR) {
            zfatal("ziopipe_base_run: epoll_wait: %m");
        }
        return -1;
    }

    for (i = 0; i < nfds; i++) {
        ev = iopb->epoll_event_list + i;
        events = ev->events;
        part = (ziopipe_part_t *) (ev->data.ptr);
        is_client_or_server = part->is_client_or_server;
        if (!is_client_or_server) {
            iop = ZCONTAINER_OF(part, ziopipe_t, client);
        } else {
            iop = ZCONTAINER_OF(part, ziopipe_t, server);
        }

        part_client = &(iop->client);
        part_server = &(iop->server);
        part = part_server;
        part_a = part_client;
        if (!is_client_or_server) {
            part = part_client;
            part_a = part_server;
        }

        if (part_client->fd == efd) {
            if (!(events & EPOLLOUT)) {
                uint64_t u;
                if (read(efd, &u, sizeof(uint64_t))) ;
            }
            continue;
        }

        e_err = 0;
        if (events & EPOLLHUP) {
            e_err = 1;
        }
        if (events & EPOLLRDHUP) {
            e_err = 1;
        }
        if (events & EPOLLERR) {
            e_err = 1;
        }
        if (part->ssl && part->ssl_error) {
            e_err = 1;
        }
        if (events & EPOLLOUT) {
            part->et_write = 1;
        }
        if (events & EPOLLIN) {
            part->et_read = 1;
        }
#define _debug_part(p)	printf("client_or_server: %ld, %d, %d, %d, %d\n", p->is_client_or_server, p->read_want_read, p->read_want_write, p->write_want_read, p->write_want_write)

#define _ssl_w(p) (((p->write_want_write) &&(p->et_write))||((p->write_want_read) &&(p->et_read)))
#define _ssl_r(p) (((p->read_want_write) &&(p->et_write))||((p->read_want_read) &&(p->et_read)))
#define _www(p) ((p->ssl) && (_ssl_w(p))) || ((!(p->ssl)) && (p->et_write))
#define _rrr(p) ((p->ssl) && (_ssl_r(p))) || ((!(p->ssl)) && (p->et_read))
#if 0
        int _rp = _rrr(part), _rpa=_rrr(part_a);
        int _wp = _www(part), _wpa=_www(part_a);
        if (_rp || (part->rbuf_p2 - part->rbuf_p1 > 0) ) {
            printf("BBB\n");
            int len = part->rbuf_p2 - part->rbuf_p1;
            if (_wpa) {
                read_write_loop(iopb, part, part_a, len);
            }
            printf("BBB over\n");
        }
        if (_wp) {
            printf("AAA\n");
            int len = part_a->rbuf_p2 - part_a->rbuf_p1;
            if ((len > 0) || _rpa) {
                write_read_loop(iopb, part, part_a, len);
            }
            printf("AAA over\n");
        }
#else
            read_write_loop(iopb, part, part_a, part->rbuf_p2 - part->rbuf_p1);

            write_read_loop(iopb, part, part_a, part_a->rbuf_p2 - part_a->rbuf_p1);
#endif
        if (e_err) {
            int len;
            len = part->rbuf_p2 > part->rbuf_p1;
            if (len > 0) {
                if (part_a->ssl) {
                    try_ssl_write(part_a, part->rbuf + part->rbuf_p1, len);
                } else {
                    if (write(part_a->fd, part->rbuf + part->rbuf_p1, len));
                }
            }
            ziopipe_set_event(iopb, part_client, 0);
            ziopipe_set_event(iopb, part_server, 0);
            if (part_client->rbuf) {
                zmpiece_free_one(iopb->rbuf_mpool, part_client->rbuf);
            }
            if (part_server->rbuf) {
                zmpiece_free_one(iopb->rbuf_mpool, part_server->rbuf);
            }
            if (part->ssl) {
                SSL_free(part->ssl);
            }
            if (part_a->ssl) {
                SSL_free(part_a->ssl);
            }
            close(part_client->fd);
            close(part_server->fd);
            if (iop->after_close) {
                iop->after_close(iop->context);
            }
            zmpiece_free_one(iopb->iop_mpool, iop);
            continue;
        }
    }

    if (iopb->break_flag) {
        return 0;
    }

    ___no_indent_while_end;

    return 0;
}

void ziopipe_base_free(ziopipe_base_t * iopb)
{
    return;
}

void ziopipe_enter(ziopipe_base_t * iopb, int client_fd, SSL *client_ssl, int server_fd, SSL *server_ssl, ziopipe_after_close_fn_t after_close, const void *context)
{
    uint64_t u;
    ziopipe_linker_t *linker, *h, *t;

    znonblocking(client_fd, 1);
    znonblocking(server_fd, 1);

    linker = (ziopipe_linker_t *) zmpiece_alloc_one(iopb->iop_mpool);

    memset(linker, 0, sizeof(ziopipe_t));
    linker->cfd = client_fd;
    linker->sfd = server_fd;
    linker->cssl = client_ssl;
    linker->sssl = server_ssl;
    linker->after_close = after_close;
    linker->context = (void *)context;

    ZIOPIPE_BASE_LOCK(iopb);
    h = (ziopipe_linker_t *) (iopb->set_list_head);
    t = (ziopipe_linker_t *) (iopb->set_list_tail);
    ZMLINK_APPEND(h, t, linker, prev, next);
    iopb->set_list_head = h;
    iopb->set_list_tail = t;
    ZIOPIPE_BASE_UNLOCK(iopb);

    if (write(iopb->eventfd_iop.client.fd, &u, sizeof(uint64_t))) ;
}

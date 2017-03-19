/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-11-08
 * ================================
 */

#include "libzc.h"
#include <openssl/ssl.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>

#define ZIOPIPE_RBUF_SIZE	    	4096
#define ZIOPIPE_BASE_LOCK(iopb)		(pthread_mutex_lock(iopb->locker))
#define ZIOPIPE_BASE_UNLOCK(iopb)	(pthread_mutex_unlock(iopb->locker))

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
    unsigned char old_events;
    int rbuf_p1:16;
    int rbuf_p2:16;
    int fd;
    char *rbuf;
    void *ssl;                  /* SSL* */
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
    ziopipe_linker_t *last;
    ziopipe_linker_t *next;
    ziopipe_after_close_fn_t after_close;
    void *context;
};

struct ziopipe_base_t {
    unsigned int break_flag:1;
    int epoll_fd;
    int epoll_event_count;
    struct epoll_event *epoll_event_list;
    void *locker;
    void *set_list_head;
    void *set_list_tail;
    zmcot_t *rbuf_mpool;
    zmcot_t *iop_mpool;
    ziopipe_t eventfd_iop;
};

static void ziopipe_set_event(ziopipe_base_t * iopb, ziopipe_part_t * part, int events)
{
    int fd, old_events, e_events;
    struct epoll_event evt;

    fd = part->fd;
    e_events = EPOLLHUP | EPOLLRDHUP | EPOLLERR;

    old_events = part->old_events;
    part->old_events = events;
    if (events == 0) {
        if (old_events) {
            if (epoll_ctl(iopb->epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1) {
                zfatal("ziopipe_set_event: fd %d: DEL  error: %m", fd);
            }
        }
    } else if (old_events != events) {
        if (events & ZEV_READ) {
            e_events |= EPOLLIN;
        }
        if (events & ZEV_WRITE) {
            e_events |= EPOLLOUT;
        }
        if (events & ZEV_PERSIST) {
            e_events |= EPOLLET;
        }
        evt.events = e_events;
        evt.data.ptr = part;
        if (epoll_ctl(iopb->epoll_fd, (old_events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD), fd, &evt) == -1) {
            zfatal("ziopipe_set_event: fd %d: %s error: %m", fd, (old_events ? "MOD" : "ADD"));
        }
    }
}

static inline int try_ssl_write(ziopipe_part_t * part, void *buf, int len)
{
    int rlen, status;

    part->write_want_write = 0;
    part->write_want_read = 0;

    rlen = SSL_write(part->ssl, buf, len);
    if (rlen < 1) {
        status = SSL_get_error(part->ssl, rlen);
        if (status == SSL_ERROR_WANT_WRITE) {
            part->write_want_write = 1;
        } else if (status == SSL_ERROR_WANT_READ) {
            part->write_want_read = 1;
        } else {
            part->ssl_error = 1;
        }
        return -1;
    }

    return rlen;
}

static inline int try_ssl_read(ziopipe_part_t * part, void *buf, int len)
{
    int rlen, status;

    part->read_want_write = 0;
    part->read_want_read = 0;

    rlen = SSL_read(part->ssl, buf, len);
    if (rlen < 1) {
        status = SSL_get_error(part->ssl, rlen);
        if (status == SSL_ERROR_WANT_WRITE) {
            part->read_want_write = 1;
        } else if (status == SSL_ERROR_WANT_READ) {
            part->read_want_read = 1;
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

    iopb = (ziopipe_base_t *) zcalloc(1, sizeof(ziopipe_base_t) + sizeof(pthread_mutex_t));

    iopb->locker = (pthread_mutex_t *) ((char *)iopb + sizeof(ziopipe_base_t));
    pthread_mutex_init(iopb->locker, 0);

    iopb->iop_mpool = zmcot_create(sizeof(ziopipe_t));
    iopb->rbuf_mpool = zmcot_create(ZIOPIPE_RBUF_SIZE);

    iopb->epoll_fd = epoll_create(1024);
    zclose_on_exec(iopb->epoll_fd, 1);

    iopb->epoll_event_count = 32;
    iopb->epoll_event_list = (struct epoll_event *)zmalloc(sizeof(struct epoll_event) * iopb->epoll_event_count);

    efd = eventfd(0, 0);
    znonblocking(efd, 1);
    iopb->eventfd_iop.client.fd = efd;
    iopb->eventfd_iop.client.is_client_or_server = 0;
    ziopipe_set_event(iopb, &(iopb->eventfd_iop.client), ZEV_READ);

    return iopb;
}

void ziopipe_base_set_break(ziopipe_base_t * iopb)
{
    iopb->break_flag = 1;
}

int ziopipe_base_run(ziopipe_base_t * iopb)
{
    int i, nfds, zev, events, is_client_or_server, e_err;
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
        linker = (ziopipe_linker_t *) (iopb->set_list_head);
        if (!linker) {
            ZIOPIPE_BASE_UNLOCK(iopb);
            break;
        }
        h = (ziopipe_linker_t *) (iopb->set_list_head);
        t = (ziopipe_linker_t *) (iopb->set_list_tail);
        ZMLINK_DETACH(h, t, linker, last, next);
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
        ziopipe_set_event(iopb, &(iop->client), ZEV_READ);
        ziopipe_set_event(iopb, &(iop->server), ZEV_READ);
    }

    nfds = epoll_wait(iopb->epoll_fd, iopb->epoll_event_list, iopb->epoll_event_count, 10000);
    if (nfds == -1) {
        if (errno != EINTR) {
            zfatal("zbase_dispatch: epoll_wait: %m");
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
            uint64_t u;
            if (read(efd, &u, sizeof(uint64_t))) ;
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
        if (e_err) {
            int len;
            len = part->rbuf_p2 > part->rbuf_p2;
            if (len > 0) {
                if (part_a->ssl) {
                    try_ssl_write(part_a, part->rbuf + part->rbuf_p1, len);
                } else {
                    if (write(part_a->fd, part->rbuf + part->rbuf_p1, len)) ;
                }
            }
            ziopipe_set_event(iopb, part_client, 0);
            ziopipe_set_event(iopb, part_server, 0);
            if (part_client->rbuf) {
                zmcot_free_one(iopb->rbuf_mpool, part_client->rbuf);
            }
            if (part_server->rbuf) {
                zmcot_free_one(iopb->rbuf_mpool, part_server->rbuf);
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
            zmcot_free_one(iopb->iop_mpool, iop);
            continue;
        }
        zev = 0;

        if (events & EPOLLOUT) {
            zev |= ZEV_WRITE;
        }
        if (events & EPOLLIN) {
            zev |= ZEV_READ;
        }
#define _debug_part(p)	printf("client_or_server: %ld, %d, %d, %d, %d\n", p->is_client_or_server, p->read_want_read, p->read_want_write, p->write_want_read, p->write_want_write)

#define _ssl_w (((part->write_want_write) &&(zev & ZEV_WRITE))||((part->write_want_read) &&(zev & ZEV_READ)))
        if (((part->ssl) && (_ssl_w)) || ((!(part->ssl)) && (zev & ZEV_WRITE))) {
            int len, wlen;
            len = part_a->rbuf_p2 - part_a->rbuf_p1;
            if (len > 0) {
                if (part->ssl) {
                    wlen = try_ssl_write(part, part_a->rbuf + part_a->rbuf_p1, len);
                } else {
                    wlen = (int)write(part->fd, part_a->rbuf + part_a->rbuf_p1, (size_t) len);
                }
                if (wlen > 0) {
                    part_a->rbuf_p1 += wlen;
                    if (len == wlen) {
                        part_a->rbuf_p1 = part_a->rbuf_p2 = 0;
                        zmcot_free_one(iopb->rbuf_mpool, part_a->rbuf);
                        part_a->rbuf = 0;

                    }
                }
            }
        }
#define _ssl_r (((part->read_want_write) &&(zev & ZEV_WRITE))||((part->read_want_read) &&(zev & ZEV_READ)))
        if (((part->ssl) && (_ssl_r)) || ((!(part->ssl)) && (zev & ZEV_READ))) {
            int rlen;
            if (part->rbuf_p2 - part->rbuf_p1 == 0) {
                part->rbuf_p1 = part->rbuf_p2 = 0;
                if (!(part->rbuf)) {
                    part->rbuf = (char *)zmcot_alloc_one(iopb->rbuf_mpool);
                }
                if (part->ssl) {
                    rlen = try_ssl_read(part, part->rbuf, ZIOPIPE_RBUF_SIZE);
                } else {
                    rlen = read(part->fd, part->rbuf, ZIOPIPE_RBUF_SIZE);
                }
                if (rlen > 0) {
                    part->rbuf_p2 = rlen;
                }
            }
        }

        int eev = 0, eev_a = 0;
        if (part->ssl) {
            if (part->read_want_read || part->write_want_read) {
                eev |= ZEV_READ;
            }
            if (part->read_want_write || part->write_want_write) {
                eev |= ZEV_WRITE;
            }
            if (part_a->rbuf_p2 - part_a->rbuf_p1 > 0) {
                if ((part->write_want_read == 0) && (part->write_want_write == 0)) {
                    part->write_want_write = 1;
                    eev |= ZEV_WRITE;
                }
            }
        } else {
            if (part_a->rbuf_p2 - part_a->rbuf_p1 > 0) {
                eev |= ZEV_WRITE;
            }
        }
        if (part_a->ssl) {
            if (part_a->read_want_read || part_a->write_want_read) {
                eev_a |= ZEV_READ;
            }
            if (part_a->read_want_write || part_a->write_want_write) {
                eev_a |= ZEV_WRITE;
            }
            if (part->rbuf_p2 - part->rbuf_p1 > 0) {
                if ((part_a->write_want_read == 0) && (part_a->write_want_write == 0)) {
                    part_a->write_want_write = 1;
                    eev_a |= ZEV_WRITE;
                }
            }
        } else {
            if (part->rbuf_p2 - part->rbuf_p1 > 0) {
                eev_a |= ZEV_WRITE;
            }
        }
        if (!eev && !eev_a) {
            eev = ZEV_READ;
            if (part->ssl) {
                part->read_want_read = 0;
                part->read_want_write = 0;
                part->write_want_read = 0;
                part->write_want_write = 0;
                part->read_want_read = 1;
            }
            eev_a = ZEV_READ;
            if (part_a->ssl) {
                part_a->read_want_read = 0;
                part_a->read_want_write = 0;
                part_a->write_want_read = 0;
                part_a->write_want_write = 0;
                part_a->read_want_read = 1;
            }
        }
        ziopipe_set_event(iopb, part, eev);
        ziopipe_set_event(iopb, part_a, eev_a);

    }
    if (nfds == iopb->epoll_event_count && nfds < 4096) {
        iopb->epoll_event_count *= 2;
        iopb->epoll_event_list = (struct epoll_event *)zrealloc(iopb->epoll_event_list, sizeof(struct epoll_event) * (iopb->epoll_event_count));
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

/* void ziopipe_enter(ziopipe_base_t * iopb, int client_fd, SSL * client_ssl, int server_fd, SSL * server_ssl) */
void ziopipe_enter(ziopipe_base_t * iopb, int client_fd, void *client_ssl, int server_fd, void *server_ssl, ziopipe_after_close_fn_t after_close, void *context)
{
    uint64_t u;
    ziopipe_linker_t *linker, *h, *t;

    znonblocking(client_fd, 1);
    znonblocking(server_fd, 1);

    linker = (ziopipe_linker_t *) zmcot_alloc_one(iopb->iop_mpool);

    memset(linker, 0, sizeof(ziopipe_t));
    linker->cfd = client_fd;
    linker->sfd = server_fd;
    linker->cssl = (SSL *) client_ssl;
    linker->sssl = (SSL *) server_ssl;
    linker->after_close = after_close;
    linker->context = context;

    ZIOPIPE_BASE_LOCK(iopb);
    h = (ziopipe_linker_t *) (iopb->set_list_head);
    t = (ziopipe_linker_t *) (iopb->set_list_tail);
    ZMLINK_APPEND(h, t, linker, last, next);
    iopb->set_list_head = h;
    iopb->set_list_tail = t;
    ZIOPIPE_BASE_UNLOCK(iopb);

    if (write(iopb->eventfd_iop.client.fd, &u, sizeof(uint64_t))) ;
}

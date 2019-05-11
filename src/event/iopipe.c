/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2015-11-08
 * ================================
 */

#include "zc.h"
#include <errno.h>
#include <openssl/ssl.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>

#pragma pack(push, 1)

/* {{{ declare, macro */
#define zvar_epoll_event_count  4096
#define zvar_iopipe_rbuf_size   4096
#define zvar_iopipe_retry_size  10
#define ZIOPIPE_BASE_LOCK(iopb)		if (pthread_mutex_lock(&(iopb->locker))) { zfatal("mutex:%m"); }
#define ZIOPIPE_BASE_UNLOCK(iopb)	if (pthread_mutex_unlock(&(iopb->locker))) { zfatal("mutex:%m"); }

typedef struct ziopipe_t ziopipe_t;
typedef struct ziopipe_part_t ziopipe_part_t;
typedef struct ziopipe_linker_t ziopipe_linker_t;
typedef struct ziopipe_retry_t ziopipe_retry_t;

struct ziopipe_part_t {
    unsigned char is_client_or_server:1;
    unsigned char error_or_closed:1;
    unsigned char event_in:1;
    int rbuf_p1:16;
    int rbuf_p2:16;
    int fd;
    char rbuf[zvar_iopipe_rbuf_size+1];
    SSL *ssl;
};

struct ziopipe_t {
    ziopipe_part_t client;
    ziopipe_part_t server;
    ziopipe_after_close_fn_t after_close;
    void *context;
    long timeout;
    ziopipe_t *prev;
    ziopipe_t *next;
    short int retry_idx;
    unsigned char error_queue_in:1;
};

struct ziopipe_retry_t {
    ziopipe_t *head;
    ziopipe_t *tail;
    int count;
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
    struct epoll_event epoll_event_list[zvar_epoll_event_count];
    ziopipe_t *ziopipe_error_vector[zvar_epoll_event_count];
    pthread_mutex_t locker;
    ziopipe_linker_t *enter_list_head;
    ziopipe_linker_t *enter_list_tail;
    ziopipe_retry_t retry_vector[zvar_iopipe_retry_size];
    long retry_stamp;
    short int retry_idx;
    ziopipe_t eventfd_iop;
    long after_peer_closed_timeout;
    int count;
};

/* }}} */

/* {{{ event set/unset */
static void ziopipe_set_event(ziopipe_base_t * iopb, ziopipe_part_t * part)
{
    if (part->event_in) {
        return;
    }
    struct epoll_event epev;
    epev.events = EPOLLET | EPOLLIN | EPOLLOUT;
    epev.data.ptr = part;
    if (epoll_ctl(iopb->epoll_fd, EPOLL_CTL_ADD, part->fd, &epev) == -1) {
        zfatal("fd %d: ADD error: %m", part->fd);
    }
    part->event_in = 1;
}

static void ziopipe_unset_event(ziopipe_base_t * iopb, ziopipe_part_t * part)
{
    if (!(part->event_in)) {
        return;
    }
    if (epoll_ctl(iopb->epoll_fd, EPOLL_CTL_DEL, part->fd, NULL) == -1) {
        zfatal("fd %d: DEL error: %m", part->fd);
    }
    part->event_in = 0;
}
/* }}} */

/* {{{ ziopipe_try_release */
static inline void ziopipe_part_release(ziopipe_base_t *iopb, ziopipe_part_t *part)
{
    if (part->error_or_closed && (part->fd !=-1 )) {
        ziopipe_unset_event(iopb, part);
        if (part->ssl) {
            zopenssl_SSL_free(part->ssl);
            part->ssl = 0;
        }
        zclose(part->fd);
        part->fd = -1;
    }
}

static void ziopipe_try_release(ziopipe_base_t *iopb, ziopipe_t *iop)
{
    ziopipe_part_t *part_client = &(iop->client);
    ziopipe_part_t *part_server = &(iop->server);
    int need_free = 0;
    if ((part_client->error_or_closed==1) && (part_server->error_or_closed==1)){
        need_free = 1;
    }
    if ((part_client->error_or_closed==1) && (part_client->rbuf_p2 == part_client->rbuf_p1)) {
        need_free = 1;
    }
    if ((part_server->error_or_closed==1) && (part_server->rbuf_p2 == part_server->rbuf_p1)) {
        need_free = 1;
    }
    if ((!need_free) && iop->timeout && (ztimeout_set_millisecond(0) > iop->timeout)) {
        need_free = 1;
    }
    if (need_free) {
        part_client->error_or_closed = 1;
        part_server->error_or_closed = 1;
    }
    ziopipe_part_release(iopb, part_client);
    ziopipe_part_release(iopb, part_server);

    if (!need_free) {
        return;
    }
    
    ZMLINK_DETACH(iopb->retry_vector[iop->retry_idx].head,iopb->retry_vector[iop->retry_idx].tail,iop,prev,next);
    iopb->retry_vector[iop->retry_idx].count --;
    iopb->count --;
    if (iop->after_close) {
        iop->after_close(iop->context);
    }
    zfree(iop);
}
/* }}} */

/* {{{ read/write loop */
static inline int ziopipe_ssl_write(ziopipe_part_t * part, void *buf, int len)
{
    int wlen = -1, status;
    if (part->error_or_closed) {
        return -1;
    }
    while(1) {
        wlen = SSL_write(part->ssl, buf, len);
        if (wlen > 0) {
            break;
        }
        status = SSL_get_error(part->ssl, wlen);
        if (status == SSL_ERROR_WANT_WRITE) {
            wlen = -1;
        } else if (status == SSL_ERROR_WANT_READ) {
            wlen = -1;
        } else {
            part->error_or_closed = 1;
        }
        break;
    }
    return wlen;
}

static inline int ziopipe_ssl_read(ziopipe_part_t * part, void *buf, int len)
{
    int rlen = -1, status;
    if (part->error_or_closed) {
        return -1;
    }
    while(1) {
        rlen = SSL_read(part->ssl, buf, len);
        if (rlen > 0) {
            break;
        }
        status = SSL_get_error(part->ssl, rlen);
        if (status == SSL_ERROR_WANT_WRITE) {
            rlen = -1;
        } else if (status == SSL_ERROR_WANT_READ) {
            rlen = -1;
        } else {
            part->error_or_closed = 1;
            break;
        }
        break;
    }
    return rlen;
}

static inline ssize_t ziopipe_sys_write(ziopipe_part_t * part, const void *buf, size_t count)
{
    int wlen = -1, eo;
    if (part->error_or_closed) {
        return -1;
    }
    while(1) {
        errno = 0;
        wlen = write(part->fd, buf, count);
        if (wlen > 0) {
            break;
        }
        if (wlen == 0) {
            wlen = -1;
            break;
        }
        eo = errno;
        if (eo == EINTR) {
            continue;
        }
        if (eo == EAGAIN) {
            break;
        }
        part->error_or_closed = 1;
        break;
    }

    return wlen;
}

static inline ssize_t ziopipe_sys_read(ziopipe_part_t * part, void *buf, size_t count)
{
    int rlen = -1, eo;
    if (part->error_or_closed) {
        return -1;
    }
    while(1) {
        errno = 0;
        rlen = read(part->fd, buf, count);
        if (rlen == 0) {
            part->error_or_closed = 1;
        }
        if (rlen >= 0) {
            break;
        }
        eo = errno;
        if (eo == EINTR) {
            continue;
        }
        if (eo == EAGAIN) {
            break;
        }
        part->error_or_closed = 1;
        break;
    }
    return rlen;
}

static void ziopipe_read_write_loop(ziopipe_base_t *iopb, ziopipe_part_t *part, ziopipe_part_t *part_a)
{
    int rbuf_len, wrote_len;
    while(1) {
        rbuf_len = part->rbuf_p2 - part->rbuf_p1;
        if (rbuf_len == 0) {
            part->rbuf_p1 = part->rbuf_p2 = 0;
            if (part->ssl) {
                rbuf_len = ziopipe_ssl_read(part, part->rbuf, zvar_iopipe_rbuf_size);
            } else {
                rbuf_len = (int)ziopipe_sys_read(part, part->rbuf, zvar_iopipe_rbuf_size);
            }

            if (rbuf_len > 0) {
                part->rbuf_p2 = rbuf_len;
                continue;
            } else {
                break;
            }
        } else {
            if (part_a->ssl) {
                wrote_len = ziopipe_ssl_write(part_a, part->rbuf + part->rbuf_p1, rbuf_len);
            } else {
                wrote_len = (int)ziopipe_sys_write(part_a, part->rbuf + part->rbuf_p1, (size_t)rbuf_len);
            }
            if (wrote_len > 0) {
                if (rbuf_len == wrote_len) {
                    part->rbuf_p1 += wrote_len;
                }
                continue;
            } else {
                break;
            }
        }
    }
}
/* }}} */

/* {{{ ziopipe_enter_list_checker */
static inline void ziopipe_enter_list_checker(ziopipe_base_t *iopb)
{
    while (iopb->enter_list_head) {
        ZIOPIPE_BASE_LOCK(iopb);
        ziopipe_linker_t *linker = iopb->enter_list_head;
        if (!linker) {
            ZIOPIPE_BASE_UNLOCK(iopb);
            break;
        }
        ZMLINK_DETACH(iopb->enter_list_head, iopb->enter_list_tail, linker, prev, next);
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
        ziopipe_t *iop = (ziopipe_t *) linker;
        memset(iop, 0, sizeof(ziopipe_t));

        iop->client.fd = cfd;
        iop->client.ssl = cssl;
        iop->client.is_client_or_server = 0;

        iop->server.fd = sfd;
        iop->server.ssl = sssl;
        iop->server.is_client_or_server = 1;

        iop->after_close = after_close;
        iop->context = context;

        ziopipe_set_event(iopb, &(iop->client));
        ziopipe_set_event(iopb, &(iop->server));

        do {
            int min_count = 1024 * 1024 * 100;
            int idx = 0;
            ziopipe_retry_t *rt = iopb->retry_vector + 0;
            for (int i = 0; i < zvar_iopipe_retry_size; i++) {
                if (iopb->retry_vector[i].count < min_count) {
                    rt = iopb->retry_vector + i;
                    min_count = rt->count;
                    idx = i;
                }
            }
            iop->retry_idx = idx;
            ZMLINK_APPEND(rt->head, rt->tail, iop, prev, next);
            rt->count ++;
            iopb->count ++;
        } while(0);
    }
}
/* }}} */

/* {{{ retry and timeout */
static inline void ziopipe_retry_vector(ziopipe_base_t *iopb)
{
    if (ztimeout_set_millisecond(0) - iopb->retry_stamp < 100) {
        return;
    }
    int idx = iopb->retry_idx;
    iopb->retry_idx++;
    if (iopb->retry_idx == zvar_iopipe_retry_size) {
        iopb->retry_idx = 0;
    }
    ziopipe_t *iop, *iop_next;
    for (iop = iopb->retry_vector[idx].head; iop; iop = iop_next) {
        iop_next = iop->next;
        ziopipe_part_t *part_client = &(iop->client);
        ziopipe_part_t *part_server = &(iop->server);
        if (part_server->error_or_closed==0) {
            ziopipe_read_write_loop(iopb, part_client, part_server);
        }
        if (part_client->error_or_closed==0) {
            ziopipe_read_write_loop(iopb, part_server, part_client);
        }
        if ((part_client->error_or_closed) || (part_server->error_or_closed)) {
            if (iop->timeout == 0) {
                iop->timeout = ztimeout_set_millisecond(iopb->after_peer_closed_timeout);
            }
        }
        ziopipe_try_release(iopb, iop);
    }
    iopb->retry_stamp = ztimeout_set_millisecond(0);
}
/* }}} */

/* {{{ ziopipe_base_create ziopipe_base_free */
ziopipe_base_t *ziopipe_base_create(void)
{
    ziopipe_base_t *iopb = (ziopipe_base_t *) calloc(1, sizeof(ziopipe_base_t));

    pthread_mutex_init(&(iopb->locker), 0);

    iopb->epoll_fd = epoll_create(1024);
    zclose_on_exec(iopb->epoll_fd, 1);

    int efd = eventfd(0, 0);
    znonblocking(efd, 1);
    zclose_on_exec(efd, 1);

    iopb->eventfd_iop.client.fd = efd;
    iopb->eventfd_iop.client.is_client_or_server = 0;
    ziopipe_set_event(iopb, &(iopb->eventfd_iop.client));

    iopb->after_peer_closed_timeout = 10 * 1000;

    return iopb;
}

void ziopipe_base_free(ziopipe_base_t *iopb)
{
    zclose(iopb->epoll_fd);
    zclose(iopb->eventfd_iop.client.fd);

    ziopipe_linker_t *hn, *h;
    for (h = iopb->enter_list_head;h;h=hn) {
        hn = h->next;
        zopenssl_SSL_free(h->cssl);
        zclose(h->cfd);
        zopenssl_SSL_free(h->sssl);
        zclose(h->sfd);
    }

    for (int idx = 0; idx < zvar_iopipe_retry_size; idx ++) {
        ziopipe_t *iop, *iop_next;
        for (iop = iopb->retry_vector[idx].head; iop; iop = iop_next) {
            iop_next = iop->next;
            iop->client.error_or_closed = 0;
            iop->server.error_or_closed = 0;
            ziopipe_try_release(iopb, iop);
        }
    }

    pthread_mutex_destroy(&(iopb->locker));
    zfree(iopb);
}
/* }}} */

/* {{{ ziopipe_base_run */
void ziopipe_base_run(ziopipe_base_t * iopb)
{
#define  ___no_indent_while_beign   while(1) {
#define  ___no_indent_while_end     }

    int efd = iopb->eventfd_iop.client.fd;

    ___no_indent_while_beign;

    ziopipe_enter_list_checker(iopb);

    ziopipe_retry_vector(iopb);

    int nfds = epoll_wait(iopb->epoll_fd, iopb->epoll_event_list, zvar_epoll_event_count, 100);
    if (nfds == -1) {
        if (errno != EINTR) {
            zfatal("ziopipe_base_run: epoll_wait: %m");
        }
        continue;
    }

    int ziopipe_error_count = 0;
    for (int i = 0; i < nfds; i++) {
        ziopipe_t *iop;
        ziopipe_part_t *part_client, *part_server;
        struct epoll_event *epev = iopb->epoll_event_list + i;
        unsigned int events = epev->events;
        ziopipe_part_t *part = (ziopipe_part_t *) (epev->data.ptr);
        int is_client_or_server = part->is_client_or_server;
        if (!is_client_or_server) {
            iop = ZCONTAINER_OF(part, ziopipe_t, client);
        } else {
            iop = ZCONTAINER_OF(part, ziopipe_t, server);
        }

        part_client = &(iop->client);
        part_server = &(iop->server);

        if (part_client->fd == efd) {
            if (!(events & EPOLLOUT)) {
                uint64_t u;
                if (read(efd, &u, sizeof(uint64_t))) {
                }
            }
            continue;
        }

        if (part_server->error_or_closed==0) {
            ziopipe_read_write_loop(iopb, part_client, part_server);
        }
        if (part_client->error_or_closed==0) {
            ziopipe_read_write_loop(iopb, part_server, part_client);
        }

        if (part_client->error_or_closed || part_server->error_or_closed) {
            if (iop->error_queue_in == 0) {
                iop->error_queue_in = 1;
                iopb->ziopipe_error_vector[ziopipe_error_count++] = iop;
            }
        }
    }

    for (int i=0;i<ziopipe_error_count;i++) {
        ziopipe_t *iop = iopb->ziopipe_error_vector[i];
        if (iop->timeout == 0) {
            iop->timeout = ztimeout_set(0) + iopb->after_peer_closed_timeout;
        }
        iop->error_queue_in = 0;
        ziopipe_try_release(iopb, iop);
    }

    if (iopb->break_flag) {
        return;
    }

    ___no_indent_while_end;
}
/* }}} */

/* {{{ enter */
void ziopipe_enter(ziopipe_base_t * iopb, int client_fd, SSL *client_ssl, int server_fd, SSL *server_ssl, ziopipe_after_close_fn_t after_close, const void *context)
{
    ziopipe_linker_t *linker, *h, *t;

    znonblocking(client_fd, 1);
    znonblocking(server_fd, 1);

    linker = (ziopipe_linker_t *) malloc(sizeof(ziopipe_t));

    memset(linker, 0, sizeof(ziopipe_linker_t));
    linker->cfd = client_fd;
    linker->sfd = server_fd;
    linker->cssl = client_ssl;
    linker->sssl = server_ssl;
    linker->after_close = after_close;
    linker->context = (void *)context;

    ZIOPIPE_BASE_LOCK(iopb);
    h = (ziopipe_linker_t *) (iopb->enter_list_head);
    t = (ziopipe_linker_t *) (iopb->enter_list_tail);
    ZMLINK_APPEND(h, t, linker, prev, next);
    iopb->enter_list_head = h;
    iopb->enter_list_tail = t;
    ZIOPIPE_BASE_UNLOCK(iopb);

    uint64_t u = 1;
    if (write(iopb->eventfd_iop.client.fd, &u, sizeof(uint64_t))) {
    }
}
/* }}} */

/* {{{ count, notify after_peer_closed_timeout */
int ziopipe_base_get_count(ziopipe_base_t *iopb)
{
    return iopb->count;
}

void ziopipe_base_stop_notify(ziopipe_base_t *iopb)
{
    iopb->break_flag = 1;
}

void ziopipe_base_after_peer_closed_timeout(ziopipe_base_t *iopb, int timeout)
{
    iopb->after_peer_closed_timeout = 1000L * (timeout<0?10:timeout);
}
/* }}} */
#pragma pack(pop)

/* Local variables:
* End:
* vim600: fdm=marker
*/

/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-11-05
 * ================================
 */

#include "libzc.h"
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/un.h>

#define ZAIO_CB_MAGIC                       0XF0U
#define ZAIO_CB_TYPE_NONE                   0X00U
#define ZAIO_CB_READ                        0X10U
#define ZAIO_CB_TYPE_READ                   0X11U
#define ZAIO_CB_TYPE_READ_N                 0X12U
#define ZAIO_CB_TYPE_READ_DELIMETER         0X13U
#define ZAIO_CB_WRITE                       0X20U
#define ZAIO_CB_TYPE_WRITE                  0X21U
#define ZAIO_CB_TYPE_SLEEP                  0X31U
#define ZAIO_CB_TYPE_SSL_INIT               0X41U

#define lock_evbase(eb)     { if((eb)->locker_context) { (eb)->lock(eb);} }
#define unlock_evbase(eb)   { if((eb)->locker_context) { (eb)->unlock(eb);} }

/* ################################################################## */
/* ev/event/trigger */

int zev_set(zev_t * ev, int events, zev_cb_t callback)
{
    int fd, old_events, e_events;
    struct epoll_event epv;
    zevbase_t *eb;

    eb = ev->evbase;
    fd = zev_get_fd(ev);
    e_events = EPOLLHUP | EPOLLRDHUP | EPOLLERR;

    ev->callback = callback;
    old_events = ev->events;
    ev->events = events;
    if (callback == 0) {
        events = 0;
    }

    lock_evbase(eb);
    if (events == 0) {
        if (old_events) {
            if (epoll_ctl(eb->epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1) {
                zfatal("zev_set: fd %d: DEL  error: %m", fd);
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
        epv.events = e_events;
        epv.data.ptr = ev;
        if (epoll_ctl(eb->epoll_fd, (old_events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD), fd, &epv) == -1) {
            zfatal("zev_set: fd %d: %s error: %m", fd, (old_events ? "MOD" : "ADD"));
        }
    }
    unlock_evbase(eb);

    return 0;
}

int zev_unset(zev_t * ev)
{
    return zev_set(ev, 0, 0);
}

zev_t *zev_create(void)
{
    return (zev_t *) zcalloc(1, sizeof(zev_t));
}

void zev_free(zev_t * ev)
{
    zfree(ev);
}

void zev_init(zev_t * ev, zevbase_t * eb, int fd)
{
    memset(ev, 0, sizeof(zev_t));
    ev->aio_type = ZEV_TYPE_EVENT;
    ev->evbase = eb;
    ev->fd = fd;
}

void zev_fini(zev_t * ev)
{
    zev_set(ev, 0, 0);
}

static inline int zev_action(zev_t * ev)
{
    zev_cb_t callback = ev->callback;

    if (callback) {
        (callback) (ev);
    } else {
        zfatal("zev: not found callback");
    }

    return 0;
}

/* ################################################################## */
/* aio */

#define epoll_event_count  4096
static int zaio_event_set(zaio_t * aio, int ev_type, int timeout);
static int ZAIO_P2_MAX = ZAIO_RWBUF_SIZE - 1;
static void zaio_ready_do(zaio_t * aio);

static int zaio_timer_cmp(zrbtree_node_t * n1, zrbtree_node_t * n2)
{
    zaio_t *e1, *e2;
    int r;

    e1 = ZCONTAINER_OF(n1, zaio_t, rbnode_time);
    e2 = ZCONTAINER_OF(n2, zaio_t, rbnode_time);
    r = e1->timeout - e2->timeout;
    if (!r) {
        r = e1->fd - e2->fd;
    }
    if (r > 0) {
        return 1;
    } else if (r < 0) {
        return -1;
    }
    return 0;
}

static inline int zaio_timer_check(zevbase_t * eb)
{
    zaio_t *aio;
    zaio_cb_t callback;
    zrbtree_node_t *rn;
    long delay;

    if (!zrbtree_have_data(&(eb->aio_timer_tree))) {
        return delay;
    }

    while (1) {
        lock_evbase(eb);
        rn = zrbtree_first(&(eb->aio_timer_tree));
        if (!rn) {
            unlock_evbase(eb);
            return 10 * 1000;
        }
        aio = ZCONTAINER_OF(rn, zaio_t, rbnode_time);
        delay = ztimeout_left(aio->timeout);
        if (delay > 0) {
            unlock_evbase(eb);
            return delay;
        }
        callback = aio->callback;
        aio->recv_events = 0;
        aio->ret = -2;
        if (aio->rw_type == ZAIO_CB_TYPE_SLEEP) {
            aio->ret = 1;
        }
        if (aio->in_time) {
            zrbtree_detach(&(eb->aio_timer_tree), rn);
        }
        aio->in_time = 0;
        unlock_evbase(eb);

        if (callback) {
            (callback) (aio);
        } else {
            zfatal("zaio: not found callback");
        }
    }

    return 10 * 1000;
}

static inline void ___zaio_cache_shift(zaio_t * aio, zaio_rwbuf_list_t * ioc, void *data, int len)
{
    int rlen, i, olen = len;
    char *buf = (char *)data;
    char *cdata;
    zaio_rwbuf_t *rwb;
    zevbase_t *eb = zaio_get_base(aio);

    while (len > 0) {
        rwb = ioc->head;
        rlen = rwb->p2 - rwb->p1 + 1;
        if (data) {
            if (len >= rlen) {
                i = rlen;
            } else {
                i = len;
            }
            cdata = rwb->data + rwb->p1;
            while (i--) {
                *buf++ = *cdata++;
            }
        }
        if (len >= rlen) {
            rwb = rwb->next;
            lock_evbase(eb);
            zmcot_free_one(eb->aio_rwbuf_mpool, ioc->head);
            unlock_evbase(eb);
            ioc->head = rwb;
            len -= rlen;
        } else {
            rwb->p1 += len;
            len = 0;
        }
    }
    if (ioc->head == 0) {
        ioc->tail = 0;
    }
    ioc->len -= olen;
}

static void ___zaio_cache_first_line(zaio_t * aio, zaio_rwbuf_list_t * ioc, char **data, int *len)
{
    zaio_rwbuf_t *rwb;

    rwb = ioc->head;
    if (!rwb) {
        *data = 0;
        *len = 0;
        return;
    }
    *data = (rwb->data + rwb->p1);
    *len = (rwb->p2 - rwb->p1 + 1);
}

static inline void ___zaio_cache_append(zaio_t * aio, zaio_rwbuf_list_t * ioc, void *data, int len)
{
    char *buf = (char *)data;
    char *cdata;
    int i, p2;
    zaio_rwbuf_t *rwb;
    zevbase_t *eb = zaio_get_base(aio);

    rwb = ioc->tail;
    p2 = 0;
    cdata = 0;
    if (rwb) {
        p2 = rwb->p2;
        cdata = rwb->data;
    }
    for (i = 0; i < len; i++) {
        if (!rwb || (p2 == ZAIO_P2_MAX)) {
            lock_evbase(eb);
            rwb = (zaio_rwbuf_t *) zmcot_alloc_one(eb->aio_rwbuf_mpool);
            unlock_evbase(eb);
            rwb->next = 0;
            rwb->p1 = 0;
            rwb->p2 = 0;
            if (ioc->tail) {
                ioc->tail->next = rwb;
                ioc->tail->p2 = p2;
            } else {
                ioc->head = rwb;
            }
            ioc->tail = rwb;
            cdata = rwb->data;
            p2 = rwb->p2;
        } else {
            p2++;
        }
        cdata[p2] = buf[i];
    }
    rwb->p2 = p2;
    ioc->len += len;
}

static inline int zaio_try_ssl_read(zaio_t * aio, void *buf, int len)
{
    int rlen, status;

    aio->ssl->read_want_write = 0;
    aio->ssl->read_want_read = 0;

    rlen = SSL_read(aio->ssl->ssl, buf, len);
    if (rlen < 1) {
        status = SSL_get_error(aio->ssl->ssl, rlen);
        if (status == SSL_ERROR_WANT_WRITE) {
            aio->ssl->read_want_write = 1;
        } else if (status == SSL_ERROR_WANT_READ) {
            aio->ssl->read_want_read = 1;
        } else {
            aio->ssl->error = 1;
        }
        return -1;
    }
    return rlen;
}

static inline int zaio_try_ssl_write(zaio_t * aio, void *buf, int len)
{
    int rlen, status;

    aio->ssl->write_want_write = 0;
    aio->ssl->write_want_read = 0;

    rlen = SSL_write(aio->ssl->ssl, buf, len);
    if (rlen < 1) {
        status = SSL_get_error(aio->ssl->ssl, rlen);
        if (status == SSL_ERROR_WANT_WRITE) {
            aio->ssl->write_want_write = 1;
        } else if (status == SSL_ERROR_WANT_READ) {
            aio->ssl->write_want_read = 1;
        } else {
            aio->ssl->error = 1;
        }
        return -1;
    }

    return rlen;
}

static int zaio_try_ssl_connect(zaio_t * aio)
{
    int rlen, status;

    aio->ssl->write_want_write = 0;
    aio->ssl->write_want_read = 0;

    rlen = SSL_connect(aio->ssl->ssl);
    if (rlen < 1) {
        status = SSL_get_error(aio->ssl->ssl, rlen);
        if (status == SSL_ERROR_WANT_WRITE) {
            aio->ssl->write_want_write = 1;
        } else if (status == SSL_ERROR_WANT_READ) {
            aio->ssl->write_want_read = 1;
        } else {
            aio->ssl->error = 1;
        }
        return -1;
    }

    return 1;
}

static int zaio_try_ssl_accept(zaio_t * aio)
{
    int rlen, status;

    aio->ssl->read_want_write = 0;
    aio->ssl->read_want_read = 0;

    rlen = SSL_accept(aio->ssl->ssl);
    if (rlen < 1) {
        status = SSL_get_error(aio->ssl->ssl, rlen);
        if (status == SSL_ERROR_WANT_WRITE) {
            aio->ssl->read_want_write = 1;
        } else if (status == SSL_ERROR_WANT_READ) {
            aio->ssl->read_want_read = 1;
        } else {
            aio->ssl->error = 1;
        }
        return -1;
    }

    return 1;
}

static inline int zaio_ssl_init___inner(zaio_t * aio, zaio_cb_t callback, int timeout)
{
    zaio_ssl_t *zssl = aio->ssl;
    int rlen;

    aio->rw_type = ZAIO_CB_TYPE_SSL_INIT;
    aio->callback = callback;

    if (zssl->server_or_client) {
        rlen = zaio_try_ssl_accept(aio);
    } else {
        rlen = zaio_try_ssl_connect(aio);
    }

    if (rlen > 0) {
        zaio_event_set(aio, 2, timeout);
        zaio_ready_do(aio);
        return 0;
    }

    zaio_event_set(aio, 1, timeout);

    return 0;
}

int zaio_ssl_init(zaio_t * aio, zsslctx_t * ctx, zaio_cb_t callback, int timeout)
{
    zevbase_t *eb = zaio_get_base(aio);
    zaio_ssl_t *zssl;
    SSL *ssl;

    ssl = SSL_new(ctx->ssl_ctx);
    SSL_set_fd(ssl, aio->fd);

    zssl = (zaio_ssl_t *) zcalloc(1, sizeof(zaio_ssl_t));
    zssl->ssl = ssl;
    zssl->server_or_client = ctx->server_or_client;

    aio->ssl = zssl;

    aio->rw_type = ZAIO_CB_TYPE_SSL_INIT;
    aio->callback = callback;
    aio->ret = timeout;

    lock_evbase(eb);
    ZMLINK_APPEND(eb->queue_head, eb->queue_tail, aio, queue_prev, queue_next);
    unlock_evbase(eb);
    if (eb->locker_context) {
        zevbase_notify(eb);
    }

    return 0;
}

void zaio_ssl_fini(zaio_t * aio)
{
    if (!(aio->ssl)) {
        return;
    }
    if (aio->ssl->ssl) {
        SSL_shutdown(aio->ssl->ssl);
        SSL_free(aio->ssl->ssl);
    }
    zfree(aio->ssl);
    aio->ssl = 0;
}

zaio_ssl_t *zaio_ssl_detach(zaio_t * aio)
{
    zaio_ssl_t *zs;

    zs = aio->ssl;
    aio->ssl = 0;

    return zs;
}

int zaio_ssl_attach(zaio_t * aio, zaio_ssl_t * zssl)
{
    aio->ssl = zssl;

    return 0;
}

void *___zaio_ssl_detach_ssl(zaio_ssl_t * assl)
{
    SSL *ssl;

    if (!assl->ssl) {
        return 0;
    }

    ssl = assl->ssl;
    assl->ssl = 0;

    return ssl;
}

int zaio_fetch_rbuf(zaio_t * aio, char *buf, int len)
{
    ___zaio_cache_shift(aio, &(aio->read_cache), buf, len);

    return 0;
}

static void zaio_ready_do(zaio_t * aio)
{
    zaio_cb_t callback;
    int rw_type;

    callback = aio->callback;
    rw_type = aio->rw_type;

    if (!callback) {
        zfatal("zaio: not found callback");
    }

    if (rw_type == ZAIO_CB_TYPE_SLEEP) {
        aio->ret = 1;
    }
    if (aio->is_local == 0) {
        zaio_event_set(aio, 0, -2);
    }

    (callback) (aio);
}

zaio_t *zaio_create(void)
{
    return (zaio_t *) zcalloc(1, sizeof(zaio_t));
}

void zaio_free(zaio_t * aio)
{
    zfree(aio);
}

void zaio_init(zaio_t * aio, zevbase_t * eb, int fd)
{
    memset(aio, 0, sizeof(zaio_t));
    aio->aio_type = ZEV_TYPE_AIO;
    aio->fd = fd;
    aio->evbase = eb;
}

void zaio_fini(zaio_t * aio)
{
    zaio_event_set(aio, 0, -2);

    if (aio->read_cache.len > 0) {
        ___zaio_cache_shift(aio, &(aio->read_cache), 0, aio->read_cache.len);
    }
    if (aio->write_cache.len > 0) {
        ___zaio_cache_shift(aio, &(aio->write_cache), 0, aio->write_cache.len);
    }
    if (aio->ssl) {
        zaio_ssl_fini(aio);
    }
}

void zaio_set_local_mode(zaio_t * aio)
{
    aio->is_local = 1;
}

static int zaio_event_set(zaio_t * aio, int ev_type, int timeout)
{
    zevbase_t *eb = zaio_get_base(aio);
    int fd, events, old_events, e_events;
    struct epoll_event evt;
    zrbtree_t *timer_tree = &(eb->aio_timer_tree);
    zrbtree_node_t *rn;
    int rw_type;

    /* timeout */
    rn = &(aio->rbnode_time);
    if (timeout > 0) {
        if (aio->in_time) {
            zrbtree_detach(timer_tree, rn);
        }
        aio->timeout = ztimeout_set(timeout);
        zrbtree_attach(timer_tree, rn);
        aio->in_time = 1;
        aio->enable_time = 1;
    } else if (timeout == 0) {
        if (aio->in_time) {
            zrbtree_detach(timer_tree, rn);
        }
        aio->in_time = 0;
        aio->enable_time = 0;
    } else if (timeout == -1) {
        if (aio->enable_time) {
            if (aio->in_time == 0) {
                zrbtree_attach(timer_tree, rn);
            }
            aio->in_time = 1;
        }
    } else {                    /* if (timeout == -2) */
        /* inner use */
        if (aio->in_time) {
            zrbtree_detach(timer_tree, rn);
        }
        aio->in_time = 0;
    }

    /* event */
    if (ev_type != 2) {
        fd = aio->fd;
        rw_type = aio->rw_type;
        events = 0;
        if (ev_type == 1) {
            /* compute the events */
            if (!(aio->ssl)) {
                if ((rw_type & ZAIO_CB_MAGIC) == ZAIO_CB_READ) {
                    events |= ZEV_READ;
                }
                if ((rw_type & ZAIO_CB_MAGIC) == ZAIO_CB_WRITE) {
                    events |= ZEV_WRITE;
                }
                if (aio->write_cache.len > 0) {
                    events |= ZEV_WRITE;
                }
            } else {
                if (aio->ssl->read_want_read || aio->ssl->write_want_read) {
                    events |= ZEV_READ;
                }
                if (aio->ssl->read_want_write || aio->ssl->write_want_write) {
                    events |= ZEV_WRITE;
                }
                if (aio->write_cache.len > 0) {
                    if ((aio->ssl->write_want_read == 0) && (aio->ssl->write_want_write == 0)) {
                        aio->ssl->write_want_write = 1;
                        events |= ZEV_WRITE;
                    }
                }
            }
        }
        e_events = EPOLLHUP | EPOLLRDHUP | EPOLLERR;
        old_events = aio->events;
        aio->events = events;

        if (events == 0) {
            if (old_events) {
                if (epoll_ctl(eb->epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1) {
                    zfatal("zaio_event_set: fd %d: DEL  error: %m", fd);
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
            evt.data.ptr = aio;
            if (epoll_ctl(eb->epoll_fd, (old_events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD), fd, &evt) == -1) {
                zfatal("zaio_event_set: fd %d: %s error: %m", fd, (old_events ? "MOD" : "ADD"));
            }
        }
    }

    return 0;
}

int zaio_attach(zaio_t * aio, zaio_cb_t callback)
{
    zevbase_t *eb = zaio_get_base(aio);

    aio->callback = callback;

    lock_evbase(eb);
    ZMLINK_APPEND(eb->extern_queue_head, eb->extern_queue_tail, aio, queue_prev, queue_next);
    unlock_evbase(eb);

    zevbase_notify(eb);

    return 0;
}

static inline int zaio_read___innner(zaio_t * aio, int max_len, zaio_cb_t callback, int timeout)
{
    int magic, rlen;
    char buf[10240];

    aio->rw_type = ZAIO_CB_TYPE_READ;
    aio->callback = callback;
    aio->read_magic_len = max_len;

    if (max_len < 1) {
        aio->ret = 0;
        zaio_event_set(aio, 2, timeout);
        zaio_ready_do(aio);
        return 0;
    }

    while (1) {
        if (aio->read_cache.len == 0) {
            magic = -1;
        } else if (aio->read_cache.len > max_len) {
            magic = max_len;
        } else {
            magic = aio->read_cache.len;
        }

        if (magic > 0) {
            aio->ret = magic;
            zaio_event_set(aio, 2, timeout);
            zaio_ready_do(aio);
            return 0;
        }
        if (!(aio->ssl)) {
            rlen = read(aio->fd, buf, 10240);
        } else {
            rlen = zaio_try_ssl_read(aio, buf, 10240);
        }
        if (rlen < 1) {
            break;
        }
        ___zaio_cache_append(aio, &(aio->read_cache), buf, rlen);
    }

    zaio_event_set(aio, 1, timeout);

    return 0;
}

int zaio_read(zaio_t * aio, int max_len, zaio_cb_t callback, int timeout)
{
    zevbase_t *eb = zaio_get_base(aio);

    aio->rw_type = ZAIO_CB_TYPE_READ;
    aio->callback = callback;
    aio->read_magic_len = max_len;
    aio->ret = timeout;

    lock_evbase(eb);
    ZMLINK_APPEND(eb->queue_head, eb->queue_tail, aio, queue_prev, queue_next);
    unlock_evbase(eb);
    if (eb->locker_context) {
        zevbase_notify(eb);
    }

    return 0;
}

static inline int zaio_read_n___inner(zaio_t * aio, int strict_len, zaio_cb_t callback, int timeout)
{
    int magic, rlen;
    char buf[10240];

    aio->rw_type = ZAIO_CB_TYPE_READ_N;
    aio->callback = callback;
    aio->read_magic_len = strict_len;

    if (strict_len < 1) {
        aio->ret = 0;
        zaio_event_set(aio, 2, timeout);
        zaio_ready_do(aio);
        return 0;
    }

    while (1) {
        if (aio->read_cache.len < strict_len) {
            magic = -1;
        } else {
            magic = strict_len;
        }
        if (magic > 0) {
            aio->ret = magic;
            zaio_event_set(aio, 2, timeout);
            zaio_ready_do(aio);
            return 0;
        }
        if (!(aio->ssl)) {
            rlen = read(aio->fd, buf, 10240);
        } else {
            rlen = zaio_try_ssl_read(aio, buf, 10240);
        }
        if (rlen < 1) {
            break;
        }
        ___zaio_cache_append(aio, &(aio->read_cache), buf, rlen);
    }

    zaio_event_set(aio, 1, timeout);

    return 0;
}

int zaio_read_n(zaio_t * aio, int strict_len, zaio_cb_t callback, int timeout)
{
    zevbase_t *eb = zaio_get_base(aio);

    aio->delimiter = '\0';
    aio->rw_type = ZAIO_CB_TYPE_READ_N;
    aio->callback = callback;
    aio->read_magic_len = strict_len;
    aio->ret = timeout;

    lock_evbase(eb);
    ZMLINK_APPEND(eb->queue_head, eb->queue_tail, aio, queue_prev, queue_next);
    unlock_evbase(eb);
    if (eb->locker_context) {
        zevbase_notify(eb);
    }

    return 0;
}

static inline int ___zaio_read_delimiter_check(zaio_t * aio, unsigned char delimiter, int max_len)
{
    int magic, i, end;
    char *buf;
    zaio_rwbuf_t *rwb;

    if (aio->read_cache.len == 0) {
        return -1;
    }

    magic = 0;
    for (rwb = aio->read_cache.head; rwb; rwb = rwb->next) {
        buf = rwb->data;
        end = rwb->p2 + 1;
        for (i = rwb->p1; i != end; i++) {
            magic++;
            if (buf[i] == delimiter) {
                return magic;
            }
            if (magic == max_len) {
                return magic;
            }
        }
    }

    return -1;

}

static inline int zaio_read_delimiter___inner(zaio_t * aio, char delimiter, int max_len, zaio_cb_t callback, int timeout)
{
    int magic, rlen;
    char buf[10240 + 10];
    char *data;

    aio->rw_type = ZAIO_CB_TYPE_READ_DELIMETER;
    aio->callback = callback;
    aio->read_magic_len = max_len;
    aio->delimiter = delimiter;

    if (max_len < 1) {
        aio->ret = 0;
        zaio_event_set(aio, 2, timeout);
        zaio_ready_do(aio);
        return 0;
    }
    magic = ___zaio_read_delimiter_check(aio, (unsigned char)delimiter, max_len);
    while (1) {
        if (magic > 0) {
            aio->ret = magic;
            zaio_event_set(aio, 2, timeout);
            zaio_ready_do(aio);
            return 0;
        }
        if (!(aio->ssl)) {
            rlen = read(aio->fd, buf, 10240);
        } else {
            rlen = zaio_try_ssl_read(aio, buf, 10240);
        }
        if (rlen < 1) {
            break;
        }
        ___zaio_cache_append(aio, &(aio->read_cache), buf, rlen);
        data = memchr(buf, aio->delimiter, rlen);
        if (data) {
            magic = aio->read_cache.len - rlen + (data - buf + 1);
            if (magic > aio->read_magic_len) {
                magic = aio->read_magic_len;
            }
        } else {
            if (aio->read_magic_len <= aio->read_cache.len) {
                magic = aio->read_magic_len;
            } else {
                magic = -1;
            }
        }
    }

    zaio_event_set(aio, 1, timeout);

    return 0;
}

int zaio_read_delimiter(zaio_t * aio, char delimiter, int max_len, zaio_cb_t callback, int timeout)
{
    zevbase_t *eb = zaio_get_base(aio);

    aio->rw_type = ZAIO_CB_TYPE_READ_DELIMETER;
    aio->delimiter = delimiter;
    aio->read_magic_len = max_len;
    aio->callback = callback;
    aio->ret = timeout;

    lock_evbase(eb);
    ZMLINK_APPEND(eb->queue_head, eb->queue_tail, aio, queue_prev, queue_next);
    unlock_evbase(eb);
    if (eb->locker_context) {
        zevbase_notify(eb);
    }

    return 0;
}

int zaio_write_cache_append(zaio_t * aio, void *buf, int len)
{
    if (len < 1) {
        return 0;
    }
    ___zaio_cache_append(aio, &(aio->write_cache), buf, len);

    return len;
}

int zaio_printf(zaio_t * aio, char *fmt, ...)
{
    va_list ap;
    char buf[1024000];
    int len;

    va_start(ap, fmt);
    len = zvsnprintf(buf, 1024000, fmt, ap);
    va_end(ap);

    zaio_write_cache_append(aio, buf, len);
    return 0;
}

int zaio_puts(zaio_t * aio, char *s)
{
    zaio_write_cache_append(aio, s, strlen(s));

    return 0;
}

static inline int zaio_write_cache_flush___inner(zaio_t * aio, zaio_cb_t callback, int timeout)
{
    aio->ret = 1;
    aio->rw_type = ZAIO_CB_TYPE_WRITE;
    aio->callback = callback;

    zaio_event_set(aio, 1, timeout);

    return 0;
}

int zaio_write_cache_flush(zaio_t * aio, zaio_cb_t callback, int timeout)
{
    zevbase_t *eb = zaio_get_base(aio);

    aio->rw_type = ZAIO_CB_TYPE_WRITE;
    aio->callback = callback;
    aio->ret = timeout;

    lock_evbase(eb);
    ZMLINK_APPEND(eb->queue_head, eb->queue_tail, aio, queue_prev, queue_next);
    unlock_evbase(eb);
    if (eb->locker_context) {
        zevbase_notify(eb);
    }

    return 0;
}

int zaio_write_cache_get_len(zaio_t * aio)
{
    return aio->write_cache.len;
}

static inline int zaio_sleep___inner(zaio_t * aio, zaio_cb_t callback, int timeout)
{
    aio->rw_type = ZAIO_CB_TYPE_SLEEP;
    aio->callback = callback;
    zaio_event_set(aio, 0, timeout);

    return 0;
}

int zaio_sleep(zaio_t * aio, zaio_cb_t callback, int timeout)
{
    zevbase_t *eb = zaio_get_base(aio);

    aio->rw_type = ZAIO_CB_TYPE_SLEEP;
    aio->callback = callback;
    aio->ret = timeout;

    lock_evbase(eb);
    ZMLINK_APPEND(eb->queue_head, eb->queue_tail, aio, queue_prev, queue_next);
    unlock_evbase(eb);
    if (eb->locker_context) {
        zevbase_notify(eb);
    }

    return 0;
}

static inline int zaio_action_read_once(zaio_t * aio, int rw_type, char *buf)
{
    int rlen, rlen_t;
    char *data;

    rlen_t = aio->read_magic_len - aio->read_cache.len;
    if (rlen_t < 10240) {
        rlen_t = 10240;
    }
    if (!(aio->ssl)) {
        rlen = read(aio->fd, buf, rlen_t);
    } else {
        rlen = zaio_try_ssl_read(aio, buf, rlen_t);
    }
    if (rlen < 1) {
        return -1;
    }
    ___zaio_cache_append(aio, &(aio->read_cache), buf, rlen);

    if (rw_type == ZAIO_CB_TYPE_READ) {
        if (aio->read_cache.len >= aio->read_magic_len) {
            aio->ret = aio->read_magic_len;
        } else {
            aio->ret = aio->read_cache.len;
        }
        return 0;
    }
    if (rw_type == ZAIO_CB_TYPE_READ_N) {
        if (aio->read_cache.len >= aio->read_magic_len) {
            aio->ret = aio->read_magic_len;
            return 0;
        } else {
            return 1;
        }
    }
    if (rw_type == ZAIO_CB_TYPE_READ_DELIMETER) {
        data = memchr(buf, aio->delimiter, rlen);
        if (data) {
            //aio->ret = aio->read_cache.len - rlen + (data - buf + 1);
            aio->ret = aio->read_cache.len + (data - buf + 1) - rlen;
            if (aio->ret > aio->read_magic_len) {
                aio->ret = aio->read_magic_len;
            }
            return 0;
        } else {
            if (aio->read_magic_len <= aio->read_cache.len) {
                aio->ret = aio->read_magic_len;
                return 0;
            }
            return 1;
        }
    }

    return 0;
}

static inline int zaio_action_read(zaio_t * aio, int rw_type)
{
    char buf[1100000];
    int ret;

    if ((aio->ssl) && (!aio->ssl->session_init)) {
        ret = zaio_try_ssl_accept(aio);
        if (ret > 0) {
            aio->ssl->session_init = 1;
            aio->ret = 1;
            return 0;
        }

        return 1;
    }

    while (1) {
        ret = zaio_action_read_once(aio, rw_type, buf);
        if (ret == -1) {
            return 1;
        }
        if (ret == 0) {
            return 0;
        }
    }

    return 0;
}

static inline int zaio_action_write(zaio_t * aio, int rw_type)
{
    int wlen, rlen, len;
    char *data;

    if ((aio->ssl) && (!aio->ssl->session_init)) {
        rlen = zaio_try_ssl_connect(aio);
        if (rlen > 0) {
            aio->ssl->session_init = 1;
            aio->ret = 1;
            return 0;
        }

        return 1;
    }
    while (1) {
        ___zaio_cache_first_line(aio, &(aio->write_cache), &data, &len);
        if (len == 0) {
            aio->ret = 1;
            return 0;
        }
        wlen = len;
        if (!(aio->ssl)) {
            rlen = write(aio->fd, data, wlen);
        } else {
            rlen = zaio_try_ssl_write(aio, data, wlen);
        }
        if (rlen < 1) {
            return 1;
        }
        ___zaio_cache_shift(aio, &(aio->write_cache), 0, rlen);
    }

    return 1;
}

static inline int zaio_action(zaio_t * aio)
{
    int rw_type;
    int events, transfer;
    zaio_ssl_t *ssl = aio->ssl;

    transfer = 1;
    rw_type = aio->rw_type;
    events = aio->recv_events;

    if (events & ZEV_EXCEPTION) {
        aio->ret = -1;
        transfer = 0;
        goto transfer;
    }
#define _ssl_w (((ssl->write_want_write) &&(events & ZEV_WRITE))||((ssl->write_want_read) &&(events & ZEV_READ)))
#define _ssl_r (((ssl->read_want_write) &&(events & ZEV_WRITE))||((ssl->read_want_read) &&(events & ZEV_READ)))
    if (((aio->ssl) && (_ssl_w)) || ((!(aio->ssl)) && (events & ZEV_WRITE))) {
        transfer = zaio_action_write(aio, rw_type);
        if (rw_type & ZAIO_CB_WRITE) {
            goto transfer;
        }
        transfer = 1;
        goto transfer;
    }

    if (((aio->ssl) && (_ssl_r)) || ((!(aio->ssl)) && (events & ZEV_READ))) {
        transfer = zaio_action_read(aio, rw_type);
        goto transfer;
    }

    aio->ret = -1;
    transfer = 0;

  transfer:
    if (aio->ssl && aio->ssl->error) {
        aio->ret = -1;
        transfer = 0;
    }

    if (transfer) {
        zaio_event_set(aio, 1, -2);
        return 1;
    }

    zaio_ready_do(aio);

    return 0;
}

/* ################################################################## */
/* timer */

static int zevtimer_cmp(zrbtree_node_t * n1, zrbtree_node_t * n2)
{
    zevtimer_t *t1, *t2;
    long r;

    t1 = ZCONTAINER_OF(n1, zevtimer_t, rbnode_time);
    t2 = ZCONTAINER_OF(n2, zevtimer_t, rbnode_time);

    r = t1->timeout - t2->timeout;

    if (!r) {
        r = (char *)(n1) - (char *)(n2);
    }
    if (r > 0) {
        return 1;
    } else if (r < 0) {
        return -1;
    }

    return 0;
}

static int inline zevtimer_check(zevbase_t * eb)
{
    zevtimer_t *timer;
    zrbtree_node_t *rn;
    long delay;
    zevtimer_cb_t callback;
    zrbtree_t *timer_tree = &(eb->general_timer_tree);

    if (!zrbtree_have_data(timer_tree)) {
        return delay;
    }
    while (1) {
        rn = zrbtree_first(timer_tree);
        if (!rn) {
            return 10 * 1000;
        }
        timer = ZCONTAINER_OF(rn, zevtimer_t, rbnode_time);
        delay = ztimeout_left(timer->timeout);
        if (delay > 0) {
            return delay;
        }
        callback = timer->callback;
        zrbtree_detach(timer_tree, &(timer->rbnode_time));
        timer->in_time = 0;

        if (callback) {
            callback(timer);
        }
    }

    return 10 * 1000;
}

zevtimer_t *zevtimer_create(void)
{
    return (zevtimer_t *) zcalloc(1, sizeof(zevtimer_t));
}

void zevtimer_free(zevtimer_t * timer)
{
    zfree(timer);
}

void zevtimer_init(zevtimer_t * timer, zevbase_t * eb)
{
    memset(timer, 0, sizeof(zevtimer_t));
    timer->evbase = eb;
}

void zevtimer_fini(zevtimer_t * timer)
{
    zevtimer_stop(timer);
}

int zevtimer_start(zevtimer_t * timer, zevtimer_cb_t callback, int timeout)
{
    zevbase_t *eb = timer->evbase;
    zrbtree_t *timer_tree = &(eb->general_timer_tree);
    zrbtree_node_t *rn = &(timer->rbnode_time);

    if (timeout > 0) {
        if (timer->in_time) {
            zrbtree_detach(timer_tree, rn);
        }
        timer->callback = callback;
        timer->timeout = ztimeout_set(timeout);
        zrbtree_attach(timer_tree, rn);
        timer->in_time = 1;
    } else {
        if (timer->in_time) {
            zrbtree_detach(timer_tree, rn);
        }
        timer->in_time = 0;
    }

    return 0;
}

int zevtimer_stop(zevtimer_t * timer)
{
    return zevtimer_start(timer, 0, 0);
}

/* ################################################################## */
/* evbase */

static int zevbase_notify_reader(zev_t * eb)
{
    uint64_t u;

    return read(zev_get_fd(eb), &u, sizeof(uint64_t));
}

int zevbase_notify(zevbase_t * eb)
{
    uint64_t u = 1;

    return write(eb->eventfd_event.fd, &u, sizeof(uint64_t));
}

zevbase_t *zevbase_create(void)
{
    zevbase_t *eb;
    int eventfd_fd;

    eb = (zevbase_t *) zcalloc(1, sizeof(zevbase_t));

    eb->aio_rwbuf_mpool = zmcot_create(sizeof(zaio_rwbuf_t));

    zrbtree_init(&(eb->general_timer_tree), zevtimer_cmp);
    zrbtree_init(&(eb->aio_timer_tree), zaio_timer_cmp);

    eb->epoll_fd = epoll_create(1024);
    zclose_on_exec(eb->epoll_fd, 1);

    eb->epoll_event_list = (struct epoll_event *)zmalloc(sizeof(struct epoll_event) * epoll_event_count);

    eventfd_fd = eventfd(0, 0);
    zclose_on_exec(eventfd_fd, 1);
    znonblocking(eventfd_fd, 1);

    zev_init(&(eb->eventfd_event), eb, eventfd_fd);
    zev_set(&(eb->eventfd_event), ZEV_READ, zevbase_notify_reader);

    return eb;
}

static inline int zevbase_extern_queue_checker(zevbase_t * eb)
{
    zaio_t *aio;

    if (!eb->extern_queue_head) {
        return 0;
    }

    while (1) {
        lock_evbase(eb);
        aio = eb->extern_queue_head;
        if (!aio) {
            unlock_evbase(eb);
            return 0;
        }
        ZMLINK_DETACH(eb->extern_queue_head, eb->extern_queue_tail, aio, queue_prev, queue_next);
        unlock_evbase(eb);

        if (aio->callback) {
            (aio->callback) (aio);
        }
    }

    return 0;
}

static inline int zevbase_queue_checker(zevbase_t * eb)
{
    zaio_t *aio;
    int rw_type;

    if (!eb->queue_head) {
        return 0;
    }

    while (1) {
        lock_evbase(eb);
        aio = eb->queue_head;
        if (!aio) {
            unlock_evbase(eb);
            return 0;
        }
        ZMLINK_DETACH(eb->queue_head, eb->queue_tail, aio, queue_prev, queue_next);
        unlock_evbase(eb);

        rw_type = aio->rw_type;

        if (rw_type == ZAIO_CB_TYPE_READ) {
            zaio_read___innner(aio, aio->read_magic_len, aio->callback, aio->ret);
        } else if (rw_type == ZAIO_CB_TYPE_READ_N) {
            zaio_read_n___inner(aio, aio->read_magic_len, aio->callback, aio->ret);
        } else if (rw_type == ZAIO_CB_TYPE_READ_DELIMETER) {
            zaio_read_delimiter___inner(aio, aio->delimiter, aio->read_magic_len, aio->callback, aio->ret);
        } else if (rw_type == ZAIO_CB_TYPE_WRITE) {
            zaio_write_cache_flush___inner(aio, aio->callback, aio->ret);
        } else if (rw_type == ZAIO_CB_TYPE_SLEEP) {
            zaio_sleep___inner(aio, aio->callback, aio->ret);
        } else if (rw_type == ZAIO_CB_TYPE_SSL_INIT) {
            zaio_ssl_init___inner(aio, aio->callback, aio->ret);
        } else {
            zfatal("zevbase: unknown cb");
        }
    }

    return 0;
}

int zevbase_dispatch(zevbase_t * eb, long delay)
{
    int i, nfds, events, recv_events;
    struct epoll_event *epv;
    zaio_t *aio;
    zev_t *ev;
    long delay_tmp;

    if ((delay < 1) || (delay > 10 * 1000)) {
        delay = 10 * 1000;
    }

    if (eb->extern_queue_head) {
        zevbase_extern_queue_checker(eb);
    }
    if (eb->queue_head) {
        zevbase_queue_checker(eb);
    }

    if (zrbtree_have_data(&(eb->general_timer_tree))) {
        delay_tmp = zevtimer_check(eb);
        if (delay_tmp < delay) {
            delay = delay_tmp;
        }
    }

    if (zrbtree_have_data(&(eb->aio_timer_tree))) {
        delay_tmp = zaio_timer_check(eb);
        if (delay_tmp < delay) {
            delay = delay_tmp;
        }
    }

    nfds = epoll_wait(eb->epoll_fd, eb->epoll_event_list, epoll_event_count, delay);
    if (nfds == -1) {
        if (errno != EINTR) {
            zfatal("zbase_dispatch: epoll_wait: %m");
        }
        return -1;
    }

    for (i = 0; i < nfds; i++) {
        epv = eb->epoll_event_list + i;
        events = epv->events;
        recv_events = 0;
        if (events & EPOLLHUP) {
            recv_events |= ZEV_HUP;
        }
        if (events & EPOLLRDHUP) {
            recv_events |= ZEV_RDHUP;
        }
        if (events & EPOLLERR) {
            recv_events |= ZEV_ERROR;
        }
        if (events & EPOLLIN) {
            recv_events |= ZEV_READ;
        }
        if (events & EPOLLOUT) {
            recv_events |= ZEV_WRITE;
        }
        aio = (zaio_t *) (epv->data.ptr);
        if (aio->aio_type == ZEV_TYPE_EVENT) {
            ev = (zev_t *) aio;
            ev->recv_events = recv_events;
            zev_action(ev);
        } else if (1 || (aio->aio_type == ZEV_TYPE_AIO)) {
            aio->recv_events = recv_events;
            zaio_action(aio);
        }
    }

    return 0;
}

void zevbase_free(zevbase_t * eb)
{
    zev_fini(&(eb->eventfd_event));
    close(eb->eventfd_event.fd);
    close(eb->epoll_fd);
    zfree(eb->epoll_event_list);
    zmcot_free(eb->aio_rwbuf_mpool);
    if (eb->locker_context) {
        eb->locker_fini(eb);
    }
    zfree(eb);
}

/* ################################################################## */

zevbase_t *zvar_evbase = 0;
void zvar_evbase_init(void)
{
    if (!zvar_evbase) {
        zvar_evbase = zevbase_create();
    }
}

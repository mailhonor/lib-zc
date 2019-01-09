/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2015-11-05
 * ================================
 */

#include "zc.h"
#include <pthread.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/time.h>
#include <sys/un.h>
#include <errno.h>

#pragma pack(push, 4)

static int zaio_event_set(zaio_t * aio, int ev_type, int timeout);
static void zaio_ready_do(zaio_t * aio);

static int ___timeout_left_millisecond(long stamp)
{
    long r;
    struct timeval tv;
    gettimeofday(&tv, 0);
    r = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    long timeout = stamp - r + 1;
    if (timeout > (3600 * 24 * 1 * 1000)) {
        timeout = (3600 * 24 * 1 * 1000);
    }
    return timeout;
}

/* {{{ global vars, declare, macro, etc */
#define zpthread_lock(l)    {if(pthread_mutex_lock((pthread_mutex_t *)(l))){zfatal("mutex:%m");}}
#define zpthread_unlock(l)  {if(pthread_mutex_unlock((pthread_mutex_t *)(l))){zfatal("mutex:%m");}}

#define lock_evbase(eb) {if((eb)->plocker_flag){ zpthread_lock(&((eb)->plocker)); }}
#define unlock_evbase(eb) {if((eb)->plocker_flag){ zpthread_unlock(&((eb)->plocker)); }}

#define zaio_append_queue(eb, aio) \
    ZMLINK_APPEND(eb->queue_head, eb->queue_tail, &(aio->rbnode_time), zrbtree_left, zrbtree_right)

#define zvar_event_none                      0X00U
#define zvar_event_read                      0X01U
#define zvar_event_write                     0X02U
#define zvar_event_rdwr                      0X03U
#define zvar_event_rdhup                     0X10U
#define zvar_event_hup                       0X20U
#define zvar_event_error                     0X40U
#define zvar_event_errors                    0X70U
#define zvar_event_timeout                   0X80U
#define zvar_event_exception                 0XF0U
#define zvar_event_type_event                0X01
#define zvar_event_type_aio                  0X02

#define zvar_aio_magic                       0XF0U
#define zvar_aio_type_none                   0X00U
#define zvar_aio_read                        0X10U
#define zvar_aio_type_read                   0X11U
#define zvar_aio_type_read_n                 0X12U
#define zvar_aio_type_read_delimeter         0X13U
#define zvar_aio_type_read_size_data         0X14U
#define zvar_aio_write                       0X20U
#define zvar_aio_type_write                  0X21U
#define zvar_aio_type_sleep                  0X31U
#define zvar_aio_type_ssl_init               0X41U

#define zvar_aio_event_disable               0
#define zvar_aio_event_enable                1
#define zvar_aio_event_reserve               2

#define zvar_epoll_event_size              1024
/* }}} */

/* {{{ struct zeio_t, zaio_t, zetimer_t, zetimer_t, zevent_base_t */
typedef void (*zeio_cb_t) (zeio_t *eio);
typedef void (*zaio_cb_t) (zaio_t *aio);
typedef void (*zetimer_cb_t) (zetimer_t *et);
typedef struct zaio_rwbuf_t zaio_rwbuf_t;
typedef struct zaio_rwbuf_list_t zaio_rwbuf_list_t;
typedef struct zaio_rwbuf_longbuf_t zaio_rwbuf_longbuf_t;

struct zeio_t {
    unsigned char aio_type:3;
    unsigned char is_local:1;
    unsigned char events;
    unsigned char revents;
    int fd;
    void (*callback) (zeio_t *eio);
    void *context;
    zevent_base_t *evbase;
};

#define zvar_aio_rwbuf_size   4096
struct zaio_rwbuf_t {
    zaio_rwbuf_t *next;
    unsigned int long_flag:1;
    unsigned int p1:15;
    unsigned int p2:15;
    char data[zvar_aio_rwbuf_size];
};
struct zaio_rwbuf_longbuf_t {
    size_t p1;
    size_t p2;
    char *data;
};
struct zaio_rwbuf_list_t {
    int len;
    zaio_rwbuf_t *head;
    zaio_rwbuf_t *tail;
};
struct zaio_t {
    unsigned char aio_type:3;
    unsigned char is_local:1;
    unsigned char in_timeout:1;
    unsigned char want_read:1;
    unsigned char is_size_data:1;
    unsigned char events;
    unsigned char revents;
    unsigned char action_type;
    char delimiter;
    int fd;
    int read_magic_len;
    int ret;
    void (*callback) (zaio_t *aio);
    zaio_rwbuf_list_t read_cache;
    zaio_rwbuf_list_t write_cache;
    long timeout;
    zrbtree_node_t rbnode_time;
    unsigned char ssl_server_or_client:1;
    unsigned char ssl_session_init:1;
    unsigned char ssl_read_want_read:1;
    unsigned char ssl_read_want_write:1;
    unsigned char ssl_write_want_read:1;
    unsigned char ssl_write_want_write:1;
    unsigned char ssl_error_or_closed:1;
    SSL *ssl;
    void *context;
    zevent_base_t *evbase;
};

struct zetimer_t {
    void (*callback) (zetimer_t *et);
    long timeout;
    unsigned char in_time:1;
    unsigned char is_local:1;
    void *context;
    zrbtree_node_t rbnode_time;
    zevent_base_t *evbase;
};

struct zevent_base_t {
    int epoll_fd;
    struct epoll_event epool_event_vector[zvar_epoll_event_size];
    zrbtree_t aio_timeout_tree;
    zrbtree_t etimer_timeout_tree;
    zeio_t *eventfd_eio;
    void (*loop_fn)(zevent_base_t *eb);
    void *context;
    zrbtree_node_t *queue_head;
    zrbtree_node_t *queue_tail;
    pthread_mutex_t plocker;
    unsigned char plocker_flag:1;
};
/* }}} */

/* {{{ zetimer_t, event timer */
static int zetimer_timeout_cmp(zrbtree_node_t * n1, zrbtree_node_t * n2)
{
    zetimer_t *t1, *t2;
    long r;

    t1 = ZCONTAINER_OF(n1, zetimer_t, rbnode_time);
    t2 = ZCONTAINER_OF(n2, zetimer_t, rbnode_time);

    r = t1->timeout - t2->timeout;

    if (!r) {
        r = (char *)t1 - (char *)t2;
    }
    if (r > 0) {
        return 1;
    } else if (r < 0) {
        return -1;
    }

    return 0;
}

zetimer_t *zetimer_create(zevent_base_t *evbase)
{
    zetimer_t *etimer = (zetimer_t *)zcalloc(1, sizeof(zetimer_t));
    etimer->evbase = evbase;
    return etimer;
}

void zetimer_free(zetimer_t *et)
{
    zetimer_stop(et);
    zfree(et);
}

void zetimer_start(zetimer_t *et, void (*callback)(zetimer_t *et), int timeout)
{
    zevent_base_t *eb = et->evbase;
    zrbtree_t *timer_tree = &(eb->etimer_timeout_tree);
    zrbtree_node_t *rn = &(et->rbnode_time);

    if (timeout < 0) {
        timeout = 0;
    }
    lock_evbase(eb);
    if (timeout > 0) {
        if (et->in_time) {
            zrbtree_detach(timer_tree, rn);
        }
        et->callback = callback;
        et->timeout = ztimeout_set_millisecond(1000L * timeout);
        zrbtree_attach(timer_tree, rn);
        et->in_time = 1;
    } else {
        if (et->in_time) {
            zrbtree_detach(timer_tree, rn);
        }
        et->in_time = 0;
    }
    unlock_evbase(eb);
    if (!(et->is_local)) {
        zevent_base_notify(eb);
    }
}

void zetimer_stop(zetimer_t *et)
{
    zetimer_start(et, 0, 0);
}

void zetimer_set_local(zetimer_t *et)
{
    et->is_local = 1;
}

void zetimer_set_context(zetimer_t *et, const void *ctx)
{
    et->context = (void *)ctx;
}

void *zetimer_get_context(zetimer_t *et)
{
    return et->context;
}

zevent_base_t *zetimer_get_event_base(zetimer_t *et)
{
    return et->evbase;
}

static inline int zetimer_timeout_check(zevent_base_t * eb)
{
    zetimer_t *timer;
    zrbtree_node_t *rn;
    long delay = 1 * 1000;
    zetimer_cb_t callback;
    zrbtree_t *timer_tree = &(eb->etimer_timeout_tree);

    if (!zrbtree_have_data(timer_tree)) {
        return delay;
    }
    while (1) {
        if (zvar_proc_stop) {
            return 1 * 1000;
        }
        lock_evbase(eb);
        rn = zrbtree_first(timer_tree);
        if (!rn) {
            unlock_evbase(eb);
            return 1 * 1000;
        }
        timer = ZCONTAINER_OF(rn, zetimer_t, rbnode_time);
        delay = ___timeout_left_millisecond(timer->timeout);
        if (delay > 0) {
            unlock_evbase(eb);
            return delay;
        }
        callback = timer->callback;
        zrbtree_detach(timer_tree, &(timer->rbnode_time));
        timer->in_time = 0;
        unlock_evbase(eb);

        if (callback) {
            (callback) (timer);
        }
    }
    return 10 * 1000;
}
/* }}} */

/* {{{ zeio_t */
zeio_t *zeio_create(int fd, zevent_base_t *evbase)
{
    zeio_t *eio = (zeio_t *)calloc(1, sizeof(zeio_t));
    eio->aio_type = zvar_event_type_event;
    eio->fd = fd;
    eio->evbase = evbase;
    return eio;
}

void zeio_free(zeio_t *eio, int close_fd)
{
    zeio_disable(eio);
    if (close_fd) {
        zclose(eio->fd);
    }
    zfree(eio);
}

void zeio_set_local(zeio_t *eio)
{
    eio->is_local = 1;
}

int zeio_get_result(zeio_t *eio)
{
    if (eio->revents & (zvar_event_error | zvar_event_hup)) {
        return -1;
    }
    if (eio->revents & (zvar_event_rdhup)) {
        return 0;
    }
    if (eio->revents & (zvar_event_read|zvar_event_write)) {
        return 1;
    }
    return 1;
}

int zeio_get_fd(zeio_t *eio)
{
    return eio->fd;
}

static void zeio_enable_event(zeio_t *eio, unsigned char events, void (*callback)(zeio_t *eio))
{
    zevent_base_t *eb = eio->evbase;
    int fd = eio->fd;
    struct epoll_event epev;
    unsigned char old_events = eio->events;
    unsigned int ep_events = EPOLLRDHUP;

    eio->revents = 0;
    eio->callback = callback;
    if (callback == 0) {
        events = 0;
    }
    eio->events = events;

    lock_evbase(eb);
    if (events == 0) {
        if (old_events) {
            if (epoll_ctl(eb->epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1) {
                zfatal("enable_event: fd %d: DEL(%m)", fd);
            }
        }
    } else if (old_events != events) {
        if (events & zvar_event_read) {
            ep_events |= EPOLLIN;
        }
        if (events & zvar_event_write) {
            ep_events |= EPOLLOUT;
        }
        epev.events = ep_events;
        epev.data.ptr = eio;
        if (epoll_ctl(eb->epoll_fd, (old_events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD), fd, &epev) == -1) {
            zfatal("enable_event: fd %d: %s error: %m", fd, (old_events ? "MOD" : "ADD"));
        }
    }
    unlock_evbase(eb);
}

void zeio_enable_read(zeio_t *eio, void (*callback)(zeio_t *eio))
{
    zeio_enable_event(eio, zvar_event_read, callback);
}

void zeio_enable_write(zeio_t *eio, void (*callback)(zeio_t *eio))
{
    zeio_enable_event(eio, zvar_event_write, callback);
}

void zeio_disable(zeio_t *eio)
{
    zeio_enable_event(eio, 0, 0);
}

void zeio_set_context(zeio_t *eio, const void *ctx)
{
    eio->context = (void *)ctx;
}

void *zeio_get_context(zeio_t *eio)
{
    return eio->context;
}

zevent_base_t *zeio_get_event_base(zeio_t *eio)
{
    return eio->evbase;
}

/* }}} */

/* {{{ rwbuf op */
static void ___zaio_cache_shift(zaio_t * aio, zaio_rwbuf_list_t * ioc, void *data, int len)
{
    int rlen, i, olen = len;
    char *buf = (char *)data;
    char *cdata;
    zaio_rwbuf_t *rwb;

    rwb = ioc->head;
    if (rwb && rwb->long_flag) {
        /* only write */
        zaio_rwbuf_longbuf_t *lb = (zaio_rwbuf_longbuf_t *)(rwb->data);
        lb->p1 += len;
        if (lb->p1 == lb->p2) {
            ioc->head = rwb->next;
            free(rwb);
            if (ioc->head == 0) {
                ioc->tail = 0;
            }
        }
        ioc->len -= olen;
        return;
    }

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
            free(ioc->head);
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
    if (rwb->long_flag) {
        zaio_rwbuf_longbuf_t *lb = (zaio_rwbuf_longbuf_t *)(rwb->data);
        *data = lb->data + lb->p1;
        *len = lb->p2 - lb->p1 + 1;
    } else {
        *data = (rwb->data + rwb->p1);
        *len = (rwb->p2 - rwb->p1 + 1);
    }
}

static void ___zaio_cache_append(zaio_t * aio, zaio_rwbuf_list_t * ioc, const void *data, int len)
{
    char *buf = (char *)data;
    char *cdata;
    int i, p2;
    zaio_rwbuf_t *rwb = ioc->tail;
    p2 = 0;
    cdata = 0;
    if (rwb) {
        p2 = rwb->p2;
        cdata = rwb->data;
    }
    for (i = 0; i < len; i++) {
        if (!rwb || (p2 == zvar_aio_rwbuf_size - 1)) {
            rwb = (zaio_rwbuf_t *)malloc(sizeof(zaio_rwbuf_t));
            rwb->long_flag = 0;
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
/* }}} */

/* {{{ ssl read/write/connect/accept/init */
static inline int zaio_ssl_read(zaio_t * aio, void *buf, int len)
{
    int rlen, status;

    aio->ssl_read_want_write = 0;
    aio->ssl_read_want_read = 0;

    rlen = SSL_read(aio->ssl, buf, len);
    if (rlen < 1) {
        status = SSL_get_error(aio->ssl, rlen);
        if (status == SSL_ERROR_WANT_WRITE) {
            aio->ssl_read_want_write = 1;
            rlen = -1;
        } else if (status == SSL_ERROR_WANT_READ) {
            aio->ssl_read_want_read = 1;
            rlen = -1;
        } else {
            aio->ssl_error_or_closed = 1;
        }
    }
    return rlen;
}

static inline int zaio_ssl_write(zaio_t * aio, void *buf, int len)
{
    int rlen, status;

    aio->ssl_write_want_write = 0;
    aio->ssl_write_want_read = 0;

    rlen = SSL_write(aio->ssl, buf, len);
    if (rlen < 1) {
        status = SSL_get_error(aio->ssl, rlen);
        if (status == SSL_ERROR_WANT_WRITE) {
            aio->ssl_write_want_write = 1;
            rlen = -1;
        } else if (status == SSL_ERROR_WANT_READ) {
            aio->ssl_write_want_read = 1;
            rlen = -1;
        } else {
            aio->ssl_error_or_closed = 1;
        }
    }

    return rlen;
}

static inline int zaio_ssl_connect(zaio_t * aio)
{
    int rlen, status;

    aio->ssl_write_want_write = 0;
    aio->ssl_write_want_read = 0;

    rlen = SSL_connect(aio->ssl);
    if (rlen < 1) {
        status = SSL_get_error(aio->ssl, rlen);
        if (status == SSL_ERROR_WANT_WRITE) {
            aio->ssl_write_want_write = 1;
        } else if (status == SSL_ERROR_WANT_READ) {
            aio->ssl_write_want_read = 1;
        } else {
            aio->ssl_error_or_closed = 1;
        }
    }

    return rlen;
}

static inline int zaio_ssl_accept(zaio_t * aio)
{
    int rlen, status;

    aio->ssl_read_want_write = 0;
    aio->ssl_read_want_read = 0;

    rlen = SSL_accept(aio->ssl);
    if (rlen < 1) {
        status = SSL_get_error(aio->ssl, rlen);
        if (status == SSL_ERROR_WANT_WRITE) {
            aio->ssl_read_want_write = 1;
        } else if (status == SSL_ERROR_WANT_READ) {
            aio->ssl_read_want_read = 1;
        } else {
            aio->ssl_error_or_closed = 1;
        }
    }

    return rlen;
}

static inline int zaio_ssl_init___inner(zaio_t * aio, zaio_cb_t callback, int timeout)
{
    int rlen;

    aio->action_type = zvar_aio_type_ssl_init;
    aio->callback = callback;

    if (aio->ssl_server_or_client) {
        rlen = zaio_ssl_accept(aio);
    } else {
        rlen = zaio_ssl_connect(aio);
    }

    if (rlen >= 0) {
        aio->ssl_session_init = 1;
        aio->ret = rlen;
        zaio_event_set(aio, zvar_aio_event_reserve, timeout);
        zaio_ready_do(aio);
        return 0;
    }

    zaio_event_set(aio, zvar_aio_event_enable, timeout);
    return 0;
}

static void zaio_ssl_init(zaio_t *aio, SSL_CTX * ctx, zaio_cb_t callback, int timeout, int server_or_client)
{
    zevent_base_t *eb = aio->evbase;

    aio->ssl = zopenssl_SSL_create(ctx, aio->fd);
    aio->ssl_server_or_client = (server_or_client?1:0);

    aio->action_type = zvar_aio_type_ssl_init;
    aio->callback = callback;
    aio->ret = timeout;

    lock_evbase(eb);
    zaio_append_queue(eb, aio);
    unlock_evbase(eb);
    if (eb->plocker_flag) {
        zevent_base_notify(eb);
    }
}
/* }}} */

/* {{{ zaio_ready_do */
static void zaio_ready_do(zaio_t * aio)
{
    zaio_cb_t callback;
    int action_type;

    callback = aio->callback;
    action_type = aio->action_type;

    if (!callback) {
        zfatal("aio: no callback");
    }

    if (action_type == zvar_aio_type_sleep) {
        aio->ret = 1;
    }
    if (aio->is_local == 0) {
        zaio_event_set(aio, zvar_aio_event_disable, 0);
    }

    aio->action_type = zvar_aio_type_none;
    callback(aio);
}
/* }}} */

/* {{{ zaio_event_set */
static int zaio_event_set(zaio_t * aio, int ev_type, int timeout)
{
    zevent_base_t *eb = aio->evbase;
    zrbtree_t *timer_tree = &(eb->aio_timeout_tree);

    /* timeout */
    if (timeout < 0) {
        timeout = zvar_max_timeout;
    }
    do {
        zrbtree_node_t *rn;
        rn = &(aio->rbnode_time);
        if (timeout > 0) {
            lock_evbase(eb);
            if (aio->in_timeout) {
                zrbtree_detach(timer_tree, rn);
            }
            aio->timeout = ztimeout_set_millisecond(1000L * timeout);
            zrbtree_attach(timer_tree, rn);
            aio->in_timeout = 1;
            unlock_evbase(eb);
        } else if (timeout == 0) {
            if (aio->in_timeout) {
                lock_evbase(eb);
                zrbtree_detach(timer_tree, rn);
                unlock_evbase(eb);
            }
            aio->in_timeout = 0;
        }
    } while(0);

    /* event */
    do {
        if (ev_type == zvar_aio_event_reserve) {
            break;
        }
        struct epoll_event epev;
        unsigned char old_events = aio->events, events;
        unsigned int ep_events;
        int fd = aio->fd;
        int action_type = aio->action_type;
        events = 0;
        if (ev_type == zvar_aio_event_enable) {
            /* compute the events */
            if (!(aio->ssl)) {
                if (action_type & zvar_aio_read) {
                    events |= zvar_event_read;
                }
                if (action_type & zvar_aio_write) {
                    events |= zvar_event_write;
                }
            } else {
                if (aio->ssl_read_want_read || aio->ssl_write_want_read) {
                    events |= zvar_event_read;
                }
                if (aio->ssl_read_want_write || aio->ssl_write_want_write) {
                    events |= zvar_event_write;
                }
            }
        }
        ep_events = EPOLLHUP | EPOLLERR;
        aio->events = events;

        if (events == 0) {
            if (old_events) {
                if (epoll_ctl(eb->epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1) {
                    zfatal("zaio_event_set: fd %d: DEL  ssl_error_or_closed: %m", fd);
                }
            }
        } else if (old_events != events) {
            if (events & zvar_event_read) {
                ep_events |= EPOLLIN;
            }
            if (events & zvar_event_write) {
                ep_events |= EPOLLOUT;
            }
            epev.events = ep_events;
            epev.data.ptr = aio;
            if (epoll_ctl(eb->epoll_fd, (old_events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD), fd, &epev) == -1) {
                zfatal("zaio_event_set: fd %d: %s ssl_error_or_closed: %m", fd, (old_events ? "MOD" : "ADD"));
            }
        }
    } while(0);

    return 0;
}
/* }}} */

/* {{{ zaio_read___inner */
static void zaio_read___inner(zaio_t * aio, int max_len, zaio_cb_t callback, int timeout)
{
    int magic, rlen;
    char buf[10240 + 10];

    aio->action_type = zvar_aio_type_read;
    aio->callback = callback;
    aio->read_magic_len = max_len;

    if (max_len < 1) {
        aio->ret = 0;
        zaio_event_set(aio, zvar_aio_event_reserve, timeout);
        zaio_ready_do(aio);
        return;
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
            zaio_event_set(aio, zvar_aio_event_reserve, timeout);
            zaio_ready_do(aio);
            return;
        }
        if (!(aio->ssl)) {
            rlen = read(aio->fd, buf, 10240);
        } else {
            rlen = zaio_ssl_read(aio, buf, 10240);
        }
        if (rlen == 0) {
            aio->ret = 0;
            zaio_event_set(aio, zvar_aio_event_disable, 0);
            zaio_ready_do(aio);
            return;
        }
        if (rlen < 0) {
            break;
        }
        ___zaio_cache_append(aio, &(aio->read_cache), buf, rlen);
    }

    zaio_event_set(aio, zvar_aio_event_enable, timeout);
}
/* }}} */

/* {{{ zaio_read_n___inner */
static void zaio_read_n___inner(zaio_t * aio, int strict_len, zaio_cb_t callback, int timeout)
{
    int magic, rlen;
    char buf[10240 + 10];

    aio->action_type = zvar_aio_type_read_n;
    aio->callback = callback;
    aio->read_magic_len = strict_len;

    if (strict_len < 1) {
        aio->ret = 0;
        zaio_event_set(aio, zvar_aio_event_reserve, timeout);
        zaio_ready_do(aio);
        return;
    }
    while (1) {
        if (aio->read_cache.len < strict_len) {
            magic = -1;
        } else {
            magic = strict_len;
        }
        if (magic > 0) {
            aio->ret = magic;
            zaio_event_set(aio, zvar_aio_event_reserve, timeout);
            zaio_ready_do(aio);
            return;
        }
        if (!(aio->ssl)) {
            rlen = read(aio->fd, buf, 10240);
        } else {
            rlen = zaio_ssl_read(aio, buf, 10240);
        }
        if (rlen == 0) {
            aio->ret = (aio->read_cache.len?0:-1);
            zaio_event_set(aio, zvar_aio_event_reserve, timeout);
            zaio_ready_do(aio);
            return;
        }
        if (rlen < 0) {
            break;
        }
        ___zaio_cache_append(aio, &(aio->read_cache), buf, rlen);
    }

    zaio_event_set(aio, zvar_aio_event_enable, timeout);
}
/* }}} */

/* {{{ zaio_read_size_data___inner */
static inline int ___zaio_read_size_data_check(zaio_t * aio)
{
    int magic = 0, end, ch, shift = 0, ci = 0, size = 0;
    unsigned char *buf;
    char tmpbuf[10];
    zaio_rwbuf_t *rwb;

    if (aio->read_cache.len == 0) {
        return -1;
    }

    for (rwb = aio->read_cache.head; rwb; rwb = rwb->next) {
        buf = (unsigned char *)(rwb->data);
        end = rwb->p2 + 1;
        for (ci = rwb->p1; ci != end; ci++) {
            magic++;
            ch = buf[ci];
            size |= ((ch & 0177) << shift);
            if (ch & 0200) {
                goto over;
            }
            if (magic > 4) {
                return -2;
            }
            shift += 7;
        }
    }
    return -1;
over:
    ___zaio_cache_shift(aio, &(aio->read_cache), tmpbuf, magic);
    return size;
}

static void zaio_read_size_data___inner(zaio_t * aio, zaio_cb_t callback, int timeout)
{
    int magic, rlen;
    char buf[10240 + 10];

    aio->action_type = zvar_aio_type_read_size_data;
    aio->callback = callback;

    while (1) {
        if (aio->read_magic_len == 0) {
           rlen = ___zaio_read_size_data_check(aio);
           if ((rlen == -2) || (rlen == 0)) {
               aio->ret = (rlen?-1:0);
               zaio_event_set(aio, zvar_aio_event_reserve, timeout);
               zaio_ready_do(aio);
               return;
           } else if (rlen > 0) {
               aio->read_magic_len = rlen;
               continue;
           }
        } else {
            if (aio->read_cache.len < aio->read_magic_len) {
                magic = -1;
            } else {
                magic = aio->read_magic_len;
            }
            if (magic > 0) {
                aio->ret = magic;
                zaio_event_set(aio, zvar_aio_event_reserve, timeout);
                zaio_ready_do(aio);
                return;
            }
        }
        if (!(aio->ssl)) {
            rlen = read(aio->fd, buf, 10240);
        } else {
            rlen = zaio_ssl_read(aio, buf, 10240);
        }
        if (rlen == 0) {
            aio->ret = -1;
            zaio_event_set(aio, zvar_aio_event_reserve, timeout);
            zaio_ready_do(aio);
            return;
        }
        if (rlen < 1) {
            break;
        }
        ___zaio_cache_append(aio, &(aio->read_cache), buf, rlen);
    }

    zaio_event_set(aio, zvar_aio_event_enable, timeout);
}
/* }}} */

/* {{{ zaio_read_delimiter___inner */
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

static void zaio_read_delimiter___inner(zaio_t * aio, char delimiter, int max_len, zaio_cb_t callback, int timeout)
{
    int magic, rlen;
    char buf[10240 + 10];
    char *data;

    aio->action_type = zvar_aio_type_read_delimeter;
    aio->callback = callback;
    aio->read_magic_len = max_len;
    aio->delimiter = delimiter;

    if (max_len < 1) {
        aio->ret = 0;
        zaio_event_set(aio, zvar_aio_event_reserve, timeout);
        zaio_ready_do(aio);
        return;
    }
    magic = ___zaio_read_delimiter_check(aio, (unsigned char)delimiter, max_len);
    while (1) {
        if (magic > 0) {
            aio->ret = magic;
            zaio_event_set(aio, zvar_aio_event_reserve, timeout);
            zaio_ready_do(aio);
            return;
        }
        if (!(aio->ssl)) {
            rlen = read(aio->fd, buf, 10240);
        } else {
            rlen = zaio_ssl_read(aio, buf, 10240);
        }
        if (rlen == 0) {
            aio->ret = (magic>0?-1:0);
            zaio_event_set(aio, zvar_aio_event_disable, 0);
            zaio_ready_do(aio);
            return;
        }
        if (rlen < 0) {
            break;
        }
        ___zaio_cache_append(aio, &(aio->read_cache), buf, rlen);
        data = (char *)memchr(buf, aio->delimiter, rlen);
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

    zaio_event_set(aio, zvar_aio_event_enable, timeout);
}
/* }}} */

/* {{{ zaio_write_cache_flush___inner */
static inline void zaio_write_cache_flush___inner(zaio_t * aio, zaio_cb_t callback, int timeout)
{
    aio->ret = 1;
    aio->action_type = zvar_aio_type_write;
    aio->callback = callback;

    int wlen, retlen, len;
    char *data;

    while (1) {
        ___zaio_cache_first_line(aio, &(aio->write_cache), &data, &len);
        if (len == 0) {
            aio->ret = 1;
            zaio_event_set(aio, zvar_aio_event_reserve, timeout);
            zaio_ready_do(aio);
            return;
        }
        wlen = len;
        if (!(aio->ssl)) {
            retlen = write(aio->fd, data, wlen);
            if (retlen < 0) {
                if (errno == EPIPE) {
                    aio->ret = -1;
                    zaio_event_set(aio, zvar_aio_event_disable, 0);
                    zaio_ready_do(aio);
                    return;
                }
            }
#if 0
            if (retlen == 0){
                aio->ret = -1;
                zaio_event_set(aio, zvar_aio_event_disable, 0);
                zaio_ready_do(aio);
                return;
            }
#endif
        } else {
            retlen = zaio_ssl_write(aio, data, wlen);
            if ((retlen == 0) || aio->ssl_error_or_closed){
                aio->ret = -1;
                zaio_event_set(aio, zvar_aio_event_disable, 0);
                zaio_ready_do(aio);
                return;
            }
        }
        if (retlen < 1) {
            break;
        }
        ___zaio_cache_shift(aio, &(aio->write_cache), 0, retlen);
    }

    zaio_event_set(aio, zvar_aio_event_enable, timeout);
}
/* }}} */

/* {{{ zaio_sleep___inner */
static inline void zaio_sleep___inner(zaio_t * aio, zaio_cb_t callback, int timeout)
{
    aio->action_type = zvar_aio_type_sleep;
    aio->callback = callback;
    zaio_event_set(aio, zvar_aio_event_disable, timeout);
}
/* }}} */

/* {{{ zaio_action */
static void zaio_action(zaio_t * aio)
{
    unsigned char action_type = aio->action_type;
    if (action_type == zvar_aio_type_read) {
        zaio_read___inner(aio, aio->read_magic_len, aio->callback, aio->ret);
    } else if (action_type == zvar_aio_type_read_n) {
        zaio_read_n___inner(aio, aio->read_magic_len, aio->callback, aio->ret);
    } else if (action_type == zvar_aio_type_read_size_data) {
        zaio_read_size_data___inner(aio, aio->callback, aio->ret);
    } else if (action_type == zvar_aio_type_read_delimeter) {
        zaio_read_delimiter___inner(aio, aio->delimiter, aio->read_magic_len, aio->callback, aio->ret);
    } else if (action_type == zvar_aio_type_write) {
        zaio_write_cache_flush___inner(aio, aio->callback, aio->ret);
    } else if (action_type == zvar_aio_type_sleep) {
        zaio_sleep___inner(aio, aio->callback, aio->ret);
    } else if (action_type == zvar_aio_type_ssl_init) {
        zaio_ssl_init___inner(aio, aio->callback, aio->ret);
    } else {
        zfatal("evbase: unknown cb");
    }
}
/* }}} */

/* {{{ zaio_create, zaio_free */
zaio_t *zaio_create(int fd, zevent_base_t *evbase)
{
    zaio_t *aio = (zaio_t *)calloc(1, sizeof(zaio_t));
    aio->aio_type = zvar_event_type_aio;
    aio->fd = fd;
    aio->evbase = evbase;
    aio->ret = 1;
    return aio;
}

void zaio_free(zaio_t *aio, int close_fd_and_release_ssl)
{
    zaio_event_set(aio, zvar_aio_event_disable, 0);

    if (aio->read_cache.len > 0) {
        ___zaio_cache_shift(aio, &(aio->read_cache), 0, aio->read_cache.len);
    }
    if (aio->write_cache.len > 0) {
        ___zaio_cache_shift(aio, &(aio->write_cache), 0, aio->write_cache.len);
    }
    if (close_fd_and_release_ssl) {
        if (aio->ssl) {
            zopenssl_SSL_free(aio->ssl);
            aio->ssl = 0;
        }
        zclose(aio->fd);
    }
    zfree(aio);
}
/* }}} */

/* {{{ async_io::bind, set_local, get_result, get_fd, context, get_cache_size, get_event_base */
void zaio_set_local(zaio_t *aio)
{
    aio->is_local = 1;
}

int zaio_get_result(zaio_t *aio)
{
    return aio->ret;
}

int zaio_get_fd(zaio_t *aio)
{
    return aio->fd;
}

SSL *zaio_get_ssl(zaio_t *aio)
{
    return aio->ssl;
}

void zaio_set_context(zaio_t *aio, const void *ctx)
{
    aio->context = (void *)ctx;
}

void *zaio_get_context(zaio_t *aio)
{
    return aio->context;
}

int zaio_get_cache_size(zaio_t *aio)
{
    return aio->write_cache.len;
}

zevent_base_t *zaio_get_event_base(zaio_t *aio)
{
    return aio->evbase;
}

/* }}} */

/* {{{  async_io::tls_connect, tls_accept */
void zaio_tls_connect(zaio_t *aio, SSL_CTX * ctx, void (*callback)(zaio_t *aio), int timeout)
{
    zaio_ssl_init(aio, ctx, callback, timeout, 0);
}

void zaio_tls_accept(zaio_t *aio, SSL_CTX * ctx, void (*callback)(zaio_t *aio), int timeout)
{
    zaio_ssl_init(aio, ctx, callback, timeout, 1);
}
/* }}} */

/* {{{ zaio_fetch_rbuf */
void zaio_fetch_rbuf(zaio_t *aio, zbuf_t *bf, int strict_len)
{
    zbuf_need_space(bf, strict_len+1);
    char *buf = zbuf_data(bf);
    ___zaio_cache_shift(aio, &(aio->read_cache), buf, strict_len);
    bf->len += strict_len;
    zbuf_terminate(bf);
}

void zaio_fetch_rbuf_data(zaio_t *aio, void *data, int strict_len)
{
    ___zaio_cache_shift(aio, &(aio->read_cache), data, strict_len);
    ((char *)data)[strict_len] = 0;
}
/* }}} */

/* {{{ async_io::read */
void zaio_read(zaio_t *aio, int max_len, void (*callback)(zaio_t *aio), int timeout)
{
    zevent_base_t *eb = aio->evbase;

    aio->action_type = zvar_aio_type_read;
    aio->callback = callback;
    aio->read_magic_len = max_len;
    aio->ret = timeout;

    lock_evbase(eb);
    zaio_append_queue(eb, aio);
    unlock_evbase(eb);
    if (eb->plocker_flag) {
        zevent_base_notify(eb);
    }
}

void zaio_readn(zaio_t *aio, int strict_len, void (*callback)(zaio_t *aio), int timeout)
{
    zevent_base_t *eb = aio->evbase;

    aio->action_type = zvar_aio_type_read_n;
    aio->callback = callback;
    aio->read_magic_len = strict_len;
    aio->ret = timeout;

    lock_evbase(eb);
    zaio_append_queue(eb, aio);
    unlock_evbase(eb);
    if (eb->plocker_flag) {
        zevent_base_notify(eb);
    }
}

void zaio_read_size_data(zaio_t *aio, void (*callback)(zaio_t *aio), int timeout)
{
    zevent_base_t *eb = aio->evbase;

    aio->delimiter = 0;
    aio->action_type = zvar_aio_type_read_size_data;
    aio->callback = callback;
    aio->read_magic_len = 0;
    aio->ret = timeout;

    lock_evbase(eb);
    zaio_append_queue(eb, aio);
    unlock_evbase(eb);
    if (eb->plocker_flag) {
        zevent_base_notify(eb);
    }
}

void zaio_gets_delimiter(zaio_t *aio, int delimiter, int max_len, void (*callback)(zaio_t *aio), int timeout)
{
    zevent_base_t *eb = aio->evbase;

    aio->action_type = zvar_aio_type_read_delimeter;
    aio->delimiter = delimiter;
    aio->read_magic_len = max_len;
    aio->callback = callback;
    aio->ret = timeout;

    lock_evbase(eb);
    zaio_append_queue(eb, aio);
    unlock_evbase(eb);
    if (eb->plocker_flag) {
        zevent_base_notify(eb);
    }
}

void zaio_gets(zaio_t *aio, int max_len, void (*callback)(zaio_t *aio), int timeout)
{
    zaio_gets_delimiter(aio, '\n', max_len, callback, timeout);
}
/* }}} */

/* {{{ async_io::cache_write_do */
void zaio_cache_printf_1024(zaio_t *aio, const char *fmt, ...)
{
    va_list ap;
    char buf[1024+1];
    int len;

    va_start(ap, fmt);
    len = vsnprintf(buf, 1024, fmt, ap);
    va_end(ap);

    zaio_cache_write(aio, buf, len);
}

void zaio_cache_puts(zaio_t *aio, const char *s)
{
    zaio_cache_write(aio, s, strlen(s));
}

void zaio_cache_write(zaio_t *aio, const void *buf, int len)
{
    if (len < 1) {
        return;
    }
    ___zaio_cache_append(aio, &(aio->write_cache), buf, len);
}

void zaio_cache_write_size_data(zaio_t *aio, const void *buf, int len)
{
    char sbuf[32];
    size_t ret = zsize_data_put_size(len, sbuf);
    zaio_cache_write(aio, sbuf, ret);
    zaio_cache_write(aio, buf, len);
}

void zaio_cache_write_direct(zaio_t *aio, const void *buf, int len)
{
    zaio_rwbuf_t *rwb;

    if (len < 1) {
        return;
    }
 
    rwb = (zaio_rwbuf_t *) calloc(1, sizeof(zaio_rwbuf_t));
    rwb->next = 0;
    rwb->long_flag = 1;
    rwb->p1 = 0;
    rwb->p2 = 0;

    zaio_rwbuf_longbuf_t *lb = (zaio_rwbuf_longbuf_t *)(rwb->data);
    lb->data = (char *)buf;
    lb->p1 = 0;
    lb->p2 = len - 1;

    if (aio->write_cache.head) {
        aio->write_cache.head->next = rwb;
        aio->write_cache.tail = rwb;
    } else {
        aio->write_cache.head = rwb;
        aio->write_cache.tail = rwb;
    }
}

void zaio_cache_flush(zaio_t *aio, void (*callback)(zaio_t *aio), int timeout)
{
    zevent_base_t *eb = aio->evbase;

    aio->action_type = zvar_aio_type_write;
    aio->callback = callback;
    aio->ret = timeout;

    lock_evbase(eb);
    zaio_append_queue(eb, aio);
    unlock_evbase(eb);
    if (eb->plocker_flag) {
        zevent_base_notify(eb);
    }
}
/* }}} */

/* {{{ async_io::sleep */
void zaio_sleep(zaio_t *aio, void (*callback)(zaio_t *aio), int timeout)
{
    zevent_base_t *eb = aio->evbase;

    aio->action_type = zvar_aio_type_sleep;
    aio->callback = callback;
    aio->ret = timeout;

    lock_evbase(eb);
    zaio_append_queue(eb, aio);
    unlock_evbase(eb);
    if (eb->plocker_flag) {
        zevent_base_notify(eb);
    }
}
/* }}} */

/* {{{ zaio_queue_checker */
static inline void zaio_queue_checker(zevent_base_t * eb)
{
    zaio_t *aio;
    zrbtree_node_t *rn;

    if (!eb->queue_head) {
        return;
    }

    while (1) {
        if (zvar_proc_stop) {
            break;
        }
        lock_evbase(eb);
        rn = eb->queue_head;
        if (!rn) {
            unlock_evbase(eb);
            return;
        }
        aio = ZCONTAINER_OF(rn, zaio_t, rbnode_time);
        ZMLINK_DETACH(eb->queue_head, eb->queue_tail, rn, zrbtree_left, zrbtree_right);
        unlock_evbase(eb);
        zaio_action(aio);
    }
}
/* }}} */

/* {{{ zaio_timeout_cmp */
static int zaio_timeout_cmp(zrbtree_node_t * n1, zrbtree_node_t * n2)
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
/* }}} */

/* {{{ zaio_timeout_check */
long zaio_timeout_check(zevent_base_t * eb)
{
    zaio_t *aio;
    zaio_cb_t callback;
    zrbtree_node_t *rn;
    long delay = 1 * 1000;

    if (!zrbtree_have_data(&(eb->aio_timeout_tree))) {
        return delay;
    }

    while (1) {
        if (zvar_proc_stop) {
            return 1 * 1000;
        }
        lock_evbase(eb);
        rn = zrbtree_first(&(eb->aio_timeout_tree));
        if (!rn) {
            unlock_evbase(eb);
            return 1 * 1000;
        }
        aio = ZCONTAINER_OF(rn, zaio_t, rbnode_time);
        delay = ___timeout_left_millisecond(aio->timeout);
        if (delay > 0) {
            unlock_evbase(eb);
            return delay;
        }
        callback = aio->callback;
        aio->revents = 0;
        aio->ret = -2;
        if (aio->action_type == zvar_aio_type_sleep) {
            aio->ret = 1;
        }
        zrbtree_detach(&(eb->aio_timeout_tree), rn);
        aio->in_timeout = 0;
        unlock_evbase(eb);

        if (callback) {
            callback(aio);
        } else {
            zfatal("aio: not found callback");
        }
    }
    return 1 * 1000;
}
/* }}} */

/* {{{ zaio_list_do for others */
void zaio_list_append(zaio_t **list_head, zaio_t **list_tail, zaio_t *aio)
{
    zrbtree_node_t *h = 0, *t = 0, *n = &(aio->rbnode_time);
    h = t = 0;
    if (*list_head) {
        h = &((*list_head)->rbnode_time);
    }
    if (*list_tail) {
        t = &((*list_tail)->rbnode_time);
    }
    ZMLINK_APPEND(h, t, n, zrbtree_left, zrbtree_right);
    if (h == 0) {
        *list_head = 0;
    } else {
        *list_head = (ZCONTAINER_OF(h, zaio_t, rbnode_time));
    }
    if (t == 0) {
        *list_tail = 0;
    } else {
        *list_tail = (ZCONTAINER_OF(t, zaio_t, rbnode_time));
    }
}

void zaio_list_detach(zaio_t **list_head, zaio_t **list_tail, zaio_t *aio)
{
    zrbtree_node_t *h = 0, *t = 0, *n = &(aio->rbnode_time);
    h = t = 0;
    if (*list_head) {
        h = &((*list_head)->rbnode_time);
    }
    if (*list_tail) {
        t = &((*list_tail)->rbnode_time);
    }
    ZMLINK_DETACH(h, t, n, zrbtree_left, zrbtree_right);
    if (h == 0) {
        *list_head = 0;
    } else {
        *list_head = (ZCONTAINER_OF(h, zaio_t, rbnode_time));
    }
    if (t == 0) {
        *list_tail = 0;
    } else {
        *list_tail = (ZCONTAINER_OF(t, zaio_t, rbnode_time));
    }
}
/* }}} */

/* {{{ zevent_base_t */
zevent_base_t *zvar_default_event_base;
static void zevent_base_notify_reader(zeio_t *eio)
{
    uint64_t u;
    read(eio->fd, &u, sizeof(uint64_t));
}

zevent_base_t *zevent_base_create()
{
    zevent_base_t *eb = (zevent_base_t *)calloc(1, sizeof(zevent_base_t));
    zrbtree_init(&(eb->etimer_timeout_tree), zetimer_timeout_cmp);
    zrbtree_init(&(eb->aio_timeout_tree), zaio_timeout_cmp);

    eb->epoll_fd = epoll_create(1024);
    zclose_on_exec(eb->epoll_fd, 1);

    eb->plocker_flag = 1;
    pthread_mutex_init(&(eb->plocker), 0);

    int eventfd_fd = eventfd(0, 0);
    zclose_on_exec(eventfd_fd, 1);
    znonblocking(eventfd_fd, 1);

    eb->eventfd_eio = zeio_create(eventfd_fd, eb);
    zeio_enable_read(eb->eventfd_eio, zevent_base_notify_reader);

    return eb;
}

void zevent_base_free(zevent_base_t *eb)
{
    zeio_free(eb->eventfd_eio, 1);
    zclose(eb->epoll_fd);
    pthread_mutex_destroy(&(eb->plocker));
    zfree(eb);
}

void zevent_base_notify(zevent_base_t *eb)
{
    uint64_t u = 1;
    write(eb->eventfd_eio->fd, &u, sizeof(uint64_t));
}

void zevent_base_set_local(zevent_base_t *eb)
{
    eb->plocker_flag = 0;
}

zbool_t zevent_base_dispatch(zevent_base_t *eb)
{
    int nfds;
    struct epoll_event *epev;
    zeio_t *eio;
    zaio_t *aio;
    int delay = 1 * 1000, delay_tmp;

    if (zvar_proc_stop) {
        return 0;
    }

    if (eb->queue_head) {
        zaio_queue_checker(eb);
    }

    if (zrbtree_have_data(&(eb->etimer_timeout_tree))) {
        delay_tmp = zetimer_timeout_check(eb);
        if (delay_tmp < delay) {
            delay = delay_tmp;
        }
    }
    if (zrbtree_have_data(&(eb->aio_timeout_tree))) {
        delay_tmp = zaio_timeout_check(eb);
        if (delay_tmp < delay) {
            delay = delay_tmp;
        }
    }

    if (zvar_proc_stop) {
        return 0;
    }
    nfds = epoll_wait(eb->epoll_fd, eb->epool_event_vector, zvar_epoll_event_size, delay);
    if (nfds == -1) {
        if (errno != EINTR) {
            zfatal("zevent_base_dispath: epoll_wait: %m");
            return 0;
        }
    }

    for (int i = 0; i < nfds; i++) {
        if (zvar_proc_stop) {
            return 0;
        }
        epev = eb->epool_event_vector + i;
        unsigned int events = epev->events;
        unsigned char revents = 0;
        if (events & EPOLLHUP) {
            revents |= zvar_event_hup;
        }
        if (events & EPOLLRDHUP) {
            revents |= zvar_event_rdhup;
        }
        if (events & EPOLLERR) {
            revents |= zvar_event_error;
        }
        if (events & EPOLLIN) {
            revents |= zvar_event_read;
        }
        if (events & EPOLLOUT) {
            revents |= zvar_event_write;
        }
        aio = (zaio_t *) (epev->data.ptr);
        if (aio->aio_type == zvar_event_type_event) {
            eio = (zeio_t *) aio;
            eio->revents = revents;
            zeio_cb_t callback = eio->callback;
            if (callback) {
                callback(eio);
            } else {
                zfatal("zeio_t: not found callback");
            }

        } else if (1 || (aio->aio_type == zvar_event_type_aio)) {
            aio->revents = revents;
            zaio_action(aio);
        }
    }
    return 1;
}

/* }}} */

#pragma pack(pop)

/* Local variables:
* End:
* vim600: fdm=marker
*/

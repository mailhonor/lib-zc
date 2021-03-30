/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2015-11-05
 * ================================
 */

#include <openssl/ssl.h>
#include "zc.h"
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/time.h>
#include <sys/un.h>
#include <errno.h>

#pragma GCC diagnostic ignored "-Wpacked-not-aligned"

#pragma pack(push, 4)

/* {{{ macro */

#define event_read                      0X01
#define event_write                     0X02

#define aio_rwbuf_size                  4096

#define zinline inline __attribute__((always_inline))

#define _zpthread_lock(l)    {if(pthread_mutex_lock((pthread_mutex_t *)(l))){zfatal("FATAL mutex:%m");}}
#define _zpthread_unlock(l)  {if(pthread_mutex_unlock((pthread_mutex_t *)(l))){zfatal("FATAL mutex:%m");}}

#define _base_lock_extern(eb) {_zpthread_lock(&((eb)->extern_plocker));}
#define _base_unlock_extern(eb) {_zpthread_unlock(&((eb)->extern_plocker));}

#define _function_pthread_is_self(eb) (_zvar_current_base == (eb))

#define _ready_go_and_return(aio, ret_value) { \
    aio->ret = ret_value; \
    aio->callback(aio); \
    return; \
}

#define _want_read_or_write(obj, r, w) { \
    (obj)->is_want_read = (r); \
    (obj)->is_want_write = (w); \
}

#define _clear_read_write_flag(obj) { \
    (obj)->is_want_read = 0; \
    (obj)->is_want_write = 0; \
    (obj)->is_ssl_error_or_closed = 0; \
}

#define _zaio_active(aio) { \
    _active_vector[aio->action_type](aio); \
}
/* }}} */

/* {{{ struct */

typedef enum active_stage_type_t active_stage_type_t;
enum active_stage_type_t {
    active_stage_none = 0,
    active_stage_incoming,
    active_stage_timeout,
    active_stage_epoll,
    active_stage_unknown,
};

typedef enum action_type_t action_type_t;
enum action_type_t {
    action_none = 0,
    action_readable,
    action_read,
    action_readn,
    action_read_delimiter,
    action_read_cint,
    action_writeable,
    action_writen,
    action_tls_connect,
    action_tls_accept,
    action_sleep,
    action_unknown,
};

typedef enum monitor_type_t monitor_type_t;
enum {
    monitor_none = 0,
    monitor_clear,
    monitor_active,
    monitor_max,
};

typedef void (*zaio_cb_t)(zaio_t *aio);
typedef struct zaio_rwbuf_t zaio_rwbuf_t;
typedef struct zaio_rwbuf_list_t zaio_rwbuf_list_t;
typedef struct zaio_rwbuf_longbuf_t zaio_rwbuf_longbuf_t;

struct zaio_rwbuf_t {
    zaio_rwbuf_t *next;
    unsigned int long_flag:1;
    int p1:15;
    int p2:15;
    char data[aio_rwbuf_size];
};
struct zaio_rwbuf_longbuf_t {
    int p1;
    int p2;
    char *data;
};
struct zaio_rwbuf_list_t {
    int len;
    zaio_rwbuf_t *head;
    zaio_rwbuf_t *tail;
};
struct zaio_t {
    unsigned char action_type:5;
    unsigned char active_stage:3;
    unsigned char in_base_context:1;
    unsigned char in_timeout_tree:1;
    unsigned char in_incoming_queue:1;
    unsigned char in_extern_incoming_queue:1;
    unsigned char in_tmp_queue:1;
    unsigned char is_want_read:1;
    unsigned char is_want_write:1;
    unsigned char is_io_ok_once:1;
    unsigned char is_delimiter_checked:1;
    unsigned char is_ssl_error_or_closed:1;
    unsigned char is_cint_want_data:1;
    unsigned char is_once_timer:1;
    unsigned char old_input_events:3;
    char delimiter;
    int ret;
    int fd;
    int read_wait_timeout;
    int write_wait_timeout;
    int sleep_timeout;
    int want_read_len; /* max_len, strict_len */
    zaio_rwbuf_list_t read_cache;
    zaio_rwbuf_list_t write_cache;
    SSL *ssl;
    void *context;
    void (*callback) (zaio_t *aio);
    long cutoff_time;
    zrbtree_node_t rbnode_time;
    zaio_base_t *aiobase;
};

struct zaio_base_t {
    unsigned char stop_flag:1;
    int epoll_fd;
    int epoll_event_size;
    struct epoll_event *epoll_event_vec;
    zrbtree_t timeout_tree;
    zaio_t *eventfd_aio;
    void (*loop_fn)(zaio_base_t *eb);
    void *context;
    zrbtree_node_t *incoming_queue_head;
    zrbtree_node_t *incoming_queue_tail;
    zrbtree_node_t *extern_incoming_queue_head;
    zrbtree_node_t *extern_incoming_queue_tail;
    zrbtree_node_t *tmp_queue_head;
    zrbtree_node_t *tmp_queue_tail;
    pthread_mutex_t extern_plocker;
};
/* }}} */

#ifdef ___ZC_AIO_USE_SSL_INNER___

/* {{{ create free with ssl */
static int _flag_zaio_ssl_engine_init = 0;
static void _zaio_ssl_engine_init();

zaio_t *zaio_create_by_ssl(SSL *ssl, zaio_base_t *aiobase)
{
    if (_flag_zaio_ssl_engine_init == 0) {
        _zaio_ssl_engine_init();
    }

    zaio_t *aio = zaio_create_by_fd(zopenssl_SSL_get_fd(ssl), aiobase);
    aio->ssl = ssl;
    return aio;
}

extern void (*_zaio_free_release)(zaio_t *aio);

static void _zaio_free_release_with_ssl(zaio_t *aio)
{
    if (aio->ssl) {
        zopenssl_SSL_free(aio->ssl);
        aio->ssl = 0;
    }
    if (aio->fd > -1) {
        zclose(aio->fd);
    }
}

SSL *zaio_get_ssl(zaio_t *aio)
{
    return aio->ssl;
}
/* }}} */

/* {{{ io with ssl */
/* -1: 错, 0: 重试, >0: 正确,长度 */
extern int (*_zaio_io_read)(zaio_t *aio, void *buf, int len);
static int _zaio_io_read_with_ssl(zaio_t *aio, void *buf, int len)
{
    int ret;

    _clear_read_write_flag(aio);

    if (aio->ssl) {
        if ((ret = SSL_read(aio->ssl, buf, len)) > 0) {
            aio->is_io_ok_once = 1;
            return ret;
        }
        int status = SSL_get_error(aio->ssl, ret);
        if (status == SSL_ERROR_WANT_WRITE) {
            aio->is_want_write = 1;
        } else if (status == SSL_ERROR_WANT_READ) {
            aio->is_want_read = 1;
        } else {
            aio->is_ssl_error_or_closed = 1;
        }
        if (aio->is_ssl_error_or_closed) {
            return -1;
        }
        return 0;
    } else  {
        while (1) {
            if ((ret = read(aio->fd, buf, len)) > 0) {
                aio->is_io_ok_once = 1;
                return ret;
            } else if (ret == 0) {
                return -1;
            }
            int ec = errno;
            if (ec == EINTR) {
                continue;
            }
            aio->is_want_read = 1;
            if (ec == EAGAIN) {
                return 0;
            }
            return -1;
        }
    }
}

extern int (*_zaio_io_write)(zaio_t *aio, const void *buf, int len);
static int _zaio_io_write_with_ssl(zaio_t *aio, const void *buf, int len)
{
    int ret;

    _clear_read_write_flag(aio);

    if (aio->ssl) {
        aio->is_ssl_error_or_closed = 0;

        if ((ret = SSL_write(aio->ssl, buf, len)) > 0) {
            aio->is_io_ok_once = 1;
            return ret;
        }
        int status = SSL_get_error(aio->ssl, ret);
        if (status == SSL_ERROR_WANT_WRITE) {
            aio->is_want_write = 1;
        } else if (status == SSL_ERROR_WANT_READ) {
            aio->is_want_read = 1;
        } else {
            aio->is_ssl_error_or_closed = 1;
        }
        if (aio->is_ssl_error_or_closed) {
            return -1;
        }
        return 0;
    } else  {
        while (1) {
            if ((ret = write(aio->fd, buf, len)) >= 0) {
                if (ret > 0) {
                    aio->is_io_ok_once = 1;
                }
                return ret;
            }
            int ec = errno;
            if (ec == EINTR) {
                continue;
            }
            if (ec == EPIPE) {
                return -1;
            }
            aio->is_want_write = 1;
            if (ec == EAGAIN) {
                return 0;
            }
            return -1;
        }
    }
}
/* }}} */

/* {{{ tls */
void _zaio_event_monitor(zaio_t *aio, int monitor_cmd);
void _zaio_enter_incoming(zaio_t *aio);

extern void (*_zaio_active_tls_connect)(zaio_t *aio);
static void _active_tls_connect(zaio_t *aio)
{
    if (aio->active_stage == active_stage_timeout) {
        _ready_go_and_return(aio, -1);
    }

    _clear_read_write_flag(aio);

    int ret = SSL_connect(aio->ssl);
    if (ret < 1) {
        int status = SSL_get_error(aio->ssl, ret);
        if (status == SSL_ERROR_WANT_WRITE) {
            aio->is_want_write = 1;
        } else if (status == SSL_ERROR_WANT_READ) {
            aio->is_want_read = 1;
        } else {
            aio->is_ssl_error_or_closed = 1;
        }
    }

    if (ret > 0) {
        _zaio_event_monitor(aio, monitor_none);
        _ready_go_and_return(aio, 1);
    } else if (aio->is_ssl_error_or_closed) {
        _zaio_event_monitor(aio, monitor_clear);
        _ready_go_and_return(aio, -1);
    } else if (aio->is_want_read || aio->is_want_write ) {
        aio->ret = 0;
        _zaio_event_monitor(aio, monitor_active);
        return;
    } else {
    }
}

extern void (*_zaio_active_tls_accept)(zaio_t *aio);
static void _active_tls_accept(zaio_t *aio)
{
    if (aio->active_stage == active_stage_timeout) {
        _ready_go_and_return(aio, -1);
    }

    _clear_read_write_flag(aio);

    int ret = SSL_accept(aio->ssl);
    if (ret < 1) {
        int status = SSL_get_error(aio->ssl, ret);
        if (status == SSL_ERROR_WANT_WRITE) {
            aio->is_want_write = 1;
        } else if (status == SSL_ERROR_WANT_READ) {
            aio->is_want_read = 1;
        } else {
            aio->is_ssl_error_or_closed = 1;
        }
    }

    if (ret > 0) {
        _zaio_event_monitor(aio, monitor_none);
        _ready_go_and_return(aio, 1);
    } else if (aio->is_ssl_error_or_closed) {
        _zaio_event_monitor(aio, monitor_clear);
        _ready_go_and_return(aio, -1);
    } else if (aio->is_want_read || aio->is_want_write ) {
        aio->ret = 0;
        _zaio_event_monitor(aio, monitor_active);
        return;
    } else {
    }
}

void zaio_tls_connect(zaio_t *aio, SSL_CTX *ctx, void (*callback)(zaio_t *aio))
{
    if (_flag_zaio_ssl_engine_init == 0) {
        _zaio_ssl_engine_init();
    }

    aio->ssl = zopenssl_SSL_create(ctx, aio->fd);
    aio->action_type = action_tls_connect;
    aio->callback = callback;

    _zaio_enter_incoming(aio);
}

void zaio_tls_accept(zaio_t *aio, SSL_CTX *ctx, void (*callback)(zaio_t *aio))
{
    if (_flag_zaio_ssl_engine_init == 0) {
        _zaio_ssl_engine_init();
    }

    aio->ssl = zopenssl_SSL_create(ctx, aio->fd);
    aio->action_type = action_tls_accept;
    aio->callback = callback;

    _zaio_enter_incoming(aio);
}
/* }}} */

/* {{{ engine ssl */
static void _zaio_ssl_engine_init()
{
    _zaio_free_release = _zaio_free_release_with_ssl;

    _zaio_io_read = _zaio_io_read_with_ssl;
    _zaio_io_write = _zaio_io_write_with_ssl;

    _zaio_active_tls_connect = _active_tls_connect;
    _zaio_active_tls_accept = _active_tls_accept;

    _flag_zaio_ssl_engine_init = 1;
}
/* }}} */

#else /* ___ZC_AIO_USE_SSL_INNER___ */

/* {{{ var */
zaio_base_t *zvar_default_aio_base;
static __thread zaio_base_t *_zvar_current_base = 0;
static zaio_cb_t _active_vector[action_unknown+1];
/* }}} */

/* {{{ rwbuf */
static void ___zaio_cache_shift(zaio_t * aio, zaio_rwbuf_list_t * ioc, const void *data, int len)
{
    int rlen, i, olen = len;
    char *buf = (char *)(void *)data;
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

static void ___zaio_cache_append(zaio_t *aio, zaio_rwbuf_list_t *ioc, const void *data, int len)
{
    if (len < 1) {
        return;
    }
    char *buf = (char *)(void *)data;
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
        if (!rwb || (p2 == aio_rwbuf_size - 1)) {
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

/* {{{ monitor */ 
void _zaio_event_monitor(zaio_t *aio, int monitor_cmd)
{

    zaio_base_t *eb = aio->aiobase;
    unsigned char action_type = aio->action_type;

    if (monitor_cmd == monitor_none) {
        return;
    }

    /* new event */
    unsigned char new_input_events = 0;
    if (monitor_cmd == monitor_clear) {
        new_input_events = 0;
    } else if (monitor_cmd == monitor_active) {
        if (aio->is_want_read) {
            new_input_events |= event_read;
        }
        if (aio->is_want_write) {
            new_input_events |= event_write;
        }
    }

    /* timer */
    if (monitor_cmd == monitor_active) {
        if (action_type == action_sleep) {
            aio->cutoff_time = ztimeout_set_millisecond(1000L * (aio->sleep_timeout));
            zrbtree_attach(&(eb->timeout_tree), &(aio->rbnode_time));
            aio->in_timeout_tree = 1;
        } else {
            int max_timeout = -1, max_read_timeout = -2, max_write_timeout = -2;
            if (new_input_events & event_read) {
                max_read_timeout = ((aio->read_wait_timeout>-1)?aio->read_wait_timeout:-1);
            }
            if (new_input_events & event_write) {
                max_write_timeout = ((aio->write_wait_timeout>-1)?aio->write_wait_timeout:-1);
            }
            if ((max_read_timeout == -1)||(max_write_timeout == -1)) {
                max_timeout = -1;
            } else if ((max_read_timeout == -2)) {
                max_timeout = max_write_timeout;
            } else if (max_write_timeout == -2) {
                max_timeout = max_read_timeout;
            } else {
                max_timeout = (max_read_timeout<max_write_timeout?max_write_timeout:max_read_timeout);
            }
            if (max_timeout > -1) {
                aio->cutoff_time = ztimeout_set_millisecond(1000L * max_timeout);
                zrbtree_attach(&(eb->timeout_tree), &(aio->rbnode_time));
                aio->in_timeout_tree = 1;
            }
        }
    }

    /* event */
    unsigned char old_input_events = aio->old_input_events;
    struct epoll_event epev;
    epev.events =  EPOLLHUP | EPOLLERR;
    if (new_input_events == 0) {
        if (old_input_events) {
            if (epoll_ctl(eb->epoll_fd, EPOLL_CTL_DEL, aio->fd, NULL) == -1) {
                zfatal("FATAL epoll_ctl del fd=%d(%m)", aio->fd);
            }
        }
    } else if (new_input_events != old_input_events) {
        if (new_input_events & event_read) {
            epev.events |= EPOLLIN;
        }
        if (new_input_events & event_write) {
            epev.events |= EPOLLOUT;
        }
        epev.data.ptr = aio;
        if (epoll_ctl(eb->epoll_fd, (old_input_events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD), aio->fd, &epev) == -1) {
            zfatal("FATAL epoll_ctl %s fd=%d(%m)", (old_input_events ? "mod" : "add"), aio->fd);
        }
    }
    aio->old_input_events = new_input_events;
}
/* }}} */

/* {{{ incoming */
void _zaio_enter_incoming(zaio_t *aio)
{
    zaio_base_t *eb = aio->aiobase;
    zrbtree_node_t *rn = &(aio->rbnode_time);
    int is_self = _function_pthread_is_self(eb);

    if ((!is_self) && (aio->in_base_context)) {
        zfatal("FATAL cannot operate the same zaio_t instance in another thread at the same time");
    }

    if (aio->callback == 0) {
        zfatal("FATAL callback is null");
    }

    aio->in_base_context = 1;
    _clear_read_write_flag(aio);

    if (is_self) {
        aio->in_incoming_queue = 1;
        ZMLINK_APPEND(eb->incoming_queue_head, eb->incoming_queue_tail, rn, zrbtree_left, zrbtree_right);
    } else {
        _base_lock_extern(eb);
        aio->in_extern_incoming_queue = 1;
        ZMLINK_APPEND(eb->extern_incoming_queue_head, eb->extern_incoming_queue_tail, rn, zrbtree_left, zrbtree_right);
        _base_unlock_extern(eb);
        zaio_base_touch(eb);
    }
}
/* }}} */

/* {{{ io */
/* -1: 错, 0: 重试, >0: 正确,长度 */
static int _zaio_io_read_no_ssl(zaio_t *aio, void *buf, int len)
{
    int ret;

    _clear_read_write_flag(aio);

    while (1) {
        if ((ret = read(aio->fd, buf, len)) > 0) {
            aio->is_io_ok_once = 1;
            return ret;
        } else if (ret == 0) {
            return -1;
        }
        int ec = errno;
        if (ec == EINTR) {
            continue;
        }
        aio->is_want_read = 1;
        if (ec == EAGAIN) {
            return 0;
        }
        return -1;
    }
}
int (*_zaio_io_read)(zaio_t *aio, void *buf, int len) = _zaio_io_read_no_ssl;

static int _zaio_io_write_no_ssl(zaio_t *aio, const void *buf, int len)
{
    int ret;

    _clear_read_write_flag(aio);

    while (1) {
        if ((ret = write(aio->fd, buf, len)) >= 0) {
            if (ret > 0) {
                aio->is_io_ok_once = 1;
            }
            return ret;
        }
        int ec = errno;
        if (ec == EINTR) {
            continue;
        }
        if (ec == EPIPE) {
            return -1;
        }
        aio->is_want_write = 1;
        if (ec == EAGAIN) {
            return 0;
        }
        return -1;
    }
}
int (*_zaio_io_write)(zaio_t *aio, const void *buf, int len) = _zaio_io_write_no_ssl;
/* }}} */

/* {{{ tls nothing */
void (*_zaio_active_tls_connect)(zaio_t *aio) = 0;
static zinline void _active_tls_connect(zaio_t *aio)
{
    return _zaio_active_tls_connect(aio);
}

void (*_zaio_active_tls_accept)(zaio_t *aio) = 0;
static zinline void _active_tls_accept(zaio_t *aio)
{
    return _zaio_active_tls_accept(aio);
}
/* }}} */

/* {{{ none */
static void _active_none(zaio_t *aio)
{
}
/* }}} */

/* {{{ readable */
static void _active_readable(zaio_t *aio)
{
    if (aio->read_cache.len) {
        _zaio_event_monitor(aio, monitor_none);
        _ready_go_and_return(aio, 1);
    }

    if (aio->active_stage == active_stage_timeout) {
        _ready_go_and_return(aio, -2);
    } else if (aio->active_stage == active_stage_epoll) {
        _zaio_event_monitor(aio, monitor_none);
        _ready_go_and_return(aio, 1);
    }

    _want_read_or_write(aio, 1, 0);
    _zaio_event_monitor(aio, monitor_active);
}

void zaio_readable(zaio_t *aio, void (*callback)(zaio_t *aio))
{
    aio->action_type = action_readable;
    aio->callback = callback;

    _zaio_enter_incoming(aio);
}
/* }}} */

/* {{{ read */
static void _active_read(zaio_t *aio)
{
    if (aio->active_stage == active_stage_timeout) {
        if (aio->is_io_ok_once) {
            _ready_go_and_return(aio, -1);
        } else {
            _ready_go_and_return(aio, -2);
        }
    }

    int ret;
    char buf[aio_rwbuf_size + 1];
    if (aio->want_read_len < 1) {
        _zaio_event_monitor(aio, monitor_none);
        _ready_go_and_return(aio, 0);
    }

    if (aio->read_cache.len == 0) {
        if ((ret = _zaio_io_read(aio, buf, aio_rwbuf_size)) > 0) {
            ___zaio_cache_append(aio, &(aio->read_cache), buf, ret);
        } else if (ret == 0) {
            aio->ret = 0;
            _zaio_event_monitor(aio, monitor_active);
            return;
        } else {
            _zaio_event_monitor(aio, monitor_clear);
            _ready_go_and_return(aio, -1);
        }
    }

    if (aio->read_cache.len > aio->want_read_len) {
        ret = aio->want_read_len;
    } else {
        ret = aio->read_cache.len;
    }
    _zaio_event_monitor(aio, monitor_none);
    _ready_go_and_return(aio, ret);
}

void zaio_read(zaio_t *aio, int max_len, void (*callback)(zaio_t *aio))
{
    aio->action_type = action_read;
    aio->want_read_len = max_len;
    aio->callback = callback;

    _zaio_enter_incoming(aio);
}

/* }}} */

/* {{{ readn */
static void _active_readn(zaio_t *aio)
{
    if (aio->active_stage == active_stage_timeout) {
        if (aio->is_io_ok_once) {
            _ready_go_and_return(aio, -1);
        } else {
            _ready_go_and_return(aio, -2);
        }
    }

    int strict_len = aio->want_read_len;
    char buf[aio_rwbuf_size + 1];

    if (strict_len < 1) {
        _zaio_event_monitor(aio, monitor_none);
        _ready_go_and_return(aio, 0);
    }
    while (1) {
        if (strict_len <= aio->read_cache.len) {
            break;
        }
        int ret = _zaio_io_read(aio, buf, aio_rwbuf_size);
        if (ret == 0) {
            aio->ret = 0;
            _zaio_event_monitor(aio, monitor_active);
            return;
        } else if (ret < 0) {
            _zaio_event_monitor(aio, monitor_clear);
            _ready_go_and_return(aio, -1);
        }
        ___zaio_cache_append(aio, &(aio->read_cache), buf, ret);
    }
    _zaio_event_monitor(aio, monitor_none);
    _ready_go_and_return(aio, strict_len);
}

void zaio_readn(zaio_t *aio, int strict_len, void (*callback)(zaio_t *aio))
{
    aio->action_type = action_readn;
    aio->want_read_len = strict_len;
    aio->callback = callback;

    _zaio_enter_incoming(aio);
}

/* }}} */

/* {{{ gets */
static void _active_read_delimiter(zaio_t *aio)
{
    if (aio->active_stage == active_stage_timeout) {
        if (aio->is_io_ok_once) {
            _ready_go_and_return(aio, -1);
        } else {
            _ready_go_and_return(aio, -2);
        }
    }

    int max_len = aio->want_read_len, rlen = -1, ret;
    char delimiter = aio->delimiter;
    char buf[aio_rwbuf_size + 1], *data;

    if (max_len < 1) {
        _zaio_event_monitor(aio, monitor_none);
        _ready_go_and_return(aio, 0);
    }

    if (aio->is_delimiter_checked == 0) {
        aio->is_delimiter_checked = 1;
        int found = 0;
        rlen = 0;
        for (zaio_rwbuf_t *rwb = aio->read_cache.head; rwb; rwb = rwb->next) {
            data = rwb->data;
            int end = rwb->p2 + 1;
            for (int i = rwb->p1; i != end; i++) {
                rlen++;
                if ((data[i] == delimiter) || (rlen == max_len)) {
                    found = 1;
                    break;
                }
            }
            if (found) {
                break;
            }
        }
        if (found ) {
            _zaio_event_monitor(aio, monitor_none);
            _ready_go_and_return(aio, rlen);
        }
    }

    while (1) {
        if ((ret = _zaio_io_read(aio, buf, aio_rwbuf_size)) < 0) {
            _zaio_event_monitor(aio, monitor_clear);
            _ready_go_and_return(aio, -1);
        } else if (ret == 0) {
            aio->ret = 0;
            _zaio_event_monitor(aio, monitor_active);
            return;
        }
        ___zaio_cache_append(aio, &(aio->read_cache), buf, ret);
        rlen = -1;
        if ((data = (char *)memchr(buf, delimiter, ret))) {
            rlen = aio->read_cache.len - ret + (data - buf + 1);
            if (rlen > max_len) {
                rlen = max_len;
            }
        } else {
            if (max_len <= aio->read_cache.len) {
                rlen = max_len;
            }
        }
        if (rlen != -1) {
            _zaio_event_monitor(aio, monitor_none);
            _ready_go_and_return(aio, rlen);
        }
    }
}

void zaio_read_delimiter(zaio_t *aio, int delimiter, int max_len, void (*callback)(zaio_t *aio))
{
    aio->action_type = action_read_delimiter;
    aio->delimiter = delimiter;
    aio->is_delimiter_checked = 0;
    aio->want_read_len = max_len;
    aio->callback = callback;

    _zaio_enter_incoming(aio);
}
/* }}} */

/* {{{ cint */
static void _active_read_cint(zaio_t *aio)
{
    if (aio->is_cint_want_data && (aio->want_read_len > -1)) {
        _active_readn(aio);
        return;
    }

    if (aio->active_stage == active_stage_timeout) {
        if (aio->is_io_ok_once) {
            _ready_go_and_return(aio, -1);
        } else {
            _ready_go_and_return(aio, -2);
        }
    }

    char buf[aio_rwbuf_size + 10];

    while (1) {
        int byte_count = 0, shift = 0, size = 0;
        for (zaio_rwbuf_t *rwb = aio->read_cache.head; rwb; rwb = rwb->next) {
            unsigned char *buf = (unsigned char *)(rwb->data);
            int end = rwb->p2 + 1;
            for (int i = rwb->p1; i != end; i++) {
                byte_count++;
                if (byte_count > 4) {
                    _zaio_event_monitor(aio, monitor_none);
                    _ready_go_and_return(aio, -1);
                }
                int ch = buf[i];
                size |= ((ch & 0177) << shift);
                if (ch & 0200) {
                    ___zaio_cache_shift(aio, &(aio->read_cache), 0, byte_count);
                    _zaio_event_monitor(aio, monitor_none);
                    if ((aio->is_cint_want_data==0) || (size == 0)) {
                        _ready_go_and_return(aio, size);
                    } else {
                        aio->want_read_len = size;
                        _zaio_enter_incoming(aio);
                        return;
                    }
                }
                shift += 7;
            }
        }
        int ret = _zaio_io_read(aio, buf, aio_rwbuf_size);
        if (ret < 0) {
            _zaio_event_monitor(aio, monitor_clear);
            _ready_go_and_return(aio, -1);
        } else if (ret == 0) {
            aio->ret = 0;
            _zaio_event_monitor(aio, monitor_active);
            return;
        }
        ___zaio_cache_append(aio, &(aio->read_cache), buf, ret);
    }
}

void zaio_get_cint(zaio_t *aio, void (*callback)(zaio_t *aio))
{
    aio->is_cint_want_data = 0;
    aio->want_read_len = -2020;
    aio->action_type = action_read_cint;
    aio->callback = callback;

    _zaio_enter_incoming(aio);
}

void zaio_get_cint_and_data(zaio_t *aio, void (*callback)(zaio_t *aio))
{
    aio->is_cint_want_data = 1;
    aio->want_read_len = -2020;
    aio->action_type = action_read_cint;
    aio->callback = callback;

    _zaio_enter_incoming(aio);
}

/* }}} */

/* {{{ write */
static void _active_writeable(zaio_t *aio)
{
    if (aio->active_stage == active_stage_timeout) {
        if (aio->is_io_ok_once) {
            _ready_go_and_return(aio, -1);
        } else {
            _ready_go_and_return(aio, -2);
        }
    } else if (aio->active_stage == active_stage_epoll) {
        _zaio_event_monitor(aio, monitor_none);
        _ready_go_and_return(aio, 1);
    }
    _want_read_or_write(aio, 0, 1);
    _zaio_event_monitor(aio, monitor_active);
}

void zaio_writeable(zaio_t *aio, void (*callback)(zaio_t *aio))
{
    aio->action_type = action_writeable;
    aio->callback = callback;

    _zaio_enter_incoming(aio);
}

static void _active_writen(zaio_t *aio)
{
    if (aio->active_stage == active_stage_timeout) {
        if (aio->is_io_ok_once) {
            _ready_go_and_return(aio, -1);
        } else {
            _ready_go_and_return(aio, -2);
        }
        return;
    }

    int ret;
    char *data;
    while (1) {
        ___zaio_cache_first_line(aio, &(aio->write_cache), &data, &ret);
        if (ret == 0) {
            _zaio_event_monitor(aio, monitor_none);
            _ready_go_and_return(aio, 1);
        }
        if ((ret = _zaio_io_write(aio, data, ret)) < 0) {
            _zaio_event_monitor(aio, monitor_clear);
            _ready_go_and_return(aio, -1);
        } else if (ret == 0) {
            aio->ret = 0;
            _zaio_event_monitor(aio, monitor_active);
            return;
        }
        ___zaio_cache_shift(aio, &(aio->write_cache), 0, ret);
    }
}

static zinline void _zaio_cache_write_inner(zaio_t *aio, const void *buf, int len)
{
    ___zaio_cache_append(aio, &(aio->write_cache), buf, len);
}

void zaio_cache_printf_1024(zaio_t *aio, const char *fmt, ...)
{
    va_list ap;
    char buf[1024+1];
    int len;

    va_start(ap, fmt);
    len = vsnprintf(buf, 1024, fmt, ap);
    va_end(ap);

    _zaio_cache_write_inner(aio, buf, len);
}

void zaio_cache_puts(zaio_t *aio, const char *s)
{
    _zaio_cache_write_inner(aio, s, strlen(s));
}

void zaio_cache_write(zaio_t *aio, const void *buf, int len)
{
    ___zaio_cache_append(aio, &(aio->write_cache), buf, len);
}

void zaio_cache_write_cint(zaio_t *aio, int len)
{
    char sbuf[32];
    int ret = zcint_put(len, sbuf);
    _zaio_cache_write_inner(aio, sbuf, ret);
}

void zaio_cache_write_cint_and_data(zaio_t *aio, const void *data, int len)
{
    if(len <0) {
        len = strlen((char *)(void *)data);
    }
    char sbuf[32];
    int ret = zcint_put(len, sbuf);
    _zaio_cache_write_inner(aio, sbuf, ret);
    _zaio_cache_write_inner(aio, data, len);
}

void zaio_cache_write_direct(zaio_t *aio, const void *buf, int len)
{
    if (len < 1) {
        return;
    }
 
    zaio_rwbuf_t *rwb = (zaio_rwbuf_t *)calloc(1, sizeof(zaio_rwbuf_t));
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

void zaio_cache_flush(zaio_t *aio, void (*callback)(zaio_t *aio))
{
    aio->action_type = action_writen;
    aio->callback = callback;

    _zaio_enter_incoming(aio);
}
/* }}} */

/* {{{ sleep */
static void _active_sleep(zaio_t *aio)
{
    if (aio->active_stage == active_stage_timeout) {
        _ready_go_and_return(aio, 1);
    } else if (aio->active_stage == active_stage_incoming) {
        if (aio->sleep_timeout < 1) {
            _ready_go_and_return(aio, 1);
        }
    }
    _zaio_event_monitor(aio, monitor_active);
}

void zaio_sleep(zaio_t *aio, void (*callback)(zaio_t *aio), int timeout)
{
    aio->action_type = action_sleep;
    aio->sleep_timeout = timeout;
    aio->callback = callback;

    _zaio_enter_incoming(aio);
}
/* }}} */

/* {{{ timer */
typedef struct _zaio_timer_t _zaio_timer_t;
struct _zaio_timer_t{
    void (*callback)(void *);
    void *ctx;
};
static void _zaio_timer_callback(zaio_t *aio)
{
    _zaio_timer_t *at = (_zaio_timer_t *)zaio_get_context(aio);
    zaio_free(aio, 1);
    at->callback(at->ctx);
    zfree(at);
}

void zaio_base_timer(zaio_base_t *eb, void (*callback)(void *ctx), void *ctx, int timeout)
{
    if (!callback) {
        return;
    }
    zaio_t *aio = zaio_create(-1, eb);
    _zaio_timer_t *at = (_zaio_timer_t *)zcalloc(1, sizeof(_zaio_timer_t));
    at->callback = callback;
    at->ctx = ctx;
    zaio_set_context(aio, at);
    zaio_sleep(aio, _zaio_timer_callback, timeout);
}
/* }}} */

/* {{{ disable */
void zaio_disable(zaio_t *aio)
{
    int is_self = _function_pthread_is_self(aio->aiobase);
    if (!is_self) {
        zfatal("FATAL zaio_disable can only be executed in the pthread it belongs to"); 
    }

    zaio_base_t *eb = (aio)->aiobase;
    zrbtree_node_t *rn = &(aio->rbnode_time);
    if (aio->in_timeout_tree) {
        zrbtree_detach(&(eb->timeout_tree), rn);
        aio->in_timeout_tree = 0;
    }
    if (aio->in_tmp_queue) {
        ZMLINK_DETACH(eb->tmp_queue_head, eb->tmp_queue_tail, rn, zrbtree_left, zrbtree_right);
        aio->in_tmp_queue = 0;
    }
    if (aio->in_incoming_queue) {
        ZMLINK_DETACH(eb->incoming_queue_head, eb->incoming_queue_tail, rn, zrbtree_left, zrbtree_right);
        aio->in_incoming_queue = 0;
    }
    if (aio->in_extern_incoming_queue) {
        _base_lock_extern(eb);
        ZMLINK_DETACH(eb->extern_incoming_queue_head, eb->extern_incoming_queue_tail, rn, zrbtree_left, zrbtree_right);
        _base_unlock_extern(eb);
        aio->in_extern_incoming_queue = 0;
    } 

    aio->action_type = action_none;
    aio->in_base_context = 0;

    _clear_read_write_flag(aio);
    _zaio_event_monitor(aio, monitor_clear);
}
/* }}} */

/* {{{ create free */
zaio_t *zaio_create_by_fd(int fd, zaio_base_t *aiobase)
{
    zaio_t *aio = (zaio_t *)calloc(1, sizeof(zaio_t));
    aio->fd = fd;
    aio->aiobase = (aiobase?aiobase:zvar_default_aio_base);
    aio->ret = 1;
    aio->read_wait_timeout = -1;
    aio->write_wait_timeout = -1;
    return aio;
}


static void _zaio_free_release_no_ssl(zaio_t *aio)
{
    if (aio->fd > -1) {
        zclose(aio->fd);
    }
}
void (*_zaio_free_release)(zaio_t *aio) = _zaio_free_release_no_ssl;

void zaio_free(zaio_t *aio, int close_fd_and_release_ssl)
{
    zaio_base_t *eb = aio->aiobase;
    int is_self = _function_pthread_is_self(eb);

    if ((!is_self) && (aio->in_base_context)) {
        zfatal("FATAL cannot operate the same zaio_t instance in another thread at the same time");
    }

    if (is_self) {
        zaio_disable(aio);
    }

    if (aio->read_cache.len > 0) {
        ___zaio_cache_shift(aio, &(aio->read_cache), 0, aio->read_cache.len);
    }
    if (aio->write_cache.len > 0) {
        ___zaio_cache_shift(aio, &(aio->write_cache), 0, aio->write_cache.len);
    }
    if (close_fd_and_release_ssl) {
        _zaio_free_release(aio);
    }
    zfree(aio);
}

void zaio_rebind_aio_base(zaio_t *aio, zaio_base_t *aiobase)
{
    zaio_base_t *eb = aio->aiobase;
    int is_self = _function_pthread_is_self(eb);

    if ((!is_self) && (aio->in_base_context)) {
        zfatal("FATAL cannot operate the same zaio_t instance in another thread at the same time");
    }
    aio->aiobase = (aiobase?aiobase:zvar_default_aio_base);
}
/* }}} */

/* {{{ wait_timeout result fd ssl context cache_size aiobase */
void zaio_set_read_wait_timeout(zaio_t *aio, int read_wait_timeout)
{
    aio->read_wait_timeout = read_wait_timeout;
}

void zaio_set_write_wait_timeout(zaio_t *aio, int write_wait_timeout)
{
    aio->write_wait_timeout = write_wait_timeout;
}

int zaio_get_result(zaio_t *aio)
{
    return aio->ret;
}

int zaio_get_fd(zaio_t *aio)
{
    return aio->fd;
}

void zaio_set_context(zaio_t *aio, const void *ctx)
{
    aio->context = (void *)ctx;
}

void *zaio_get_context(zaio_t *aio)
{
    return aio->context;
}

int zaio_get_write_cache_size(zaio_t *aio)
{
    return aio->write_cache.len;
}

int zaio_get_read_cache_size(zaio_t *aio)
{
    return aio->read_cache.len;
}

zaio_base_t *zaio_get_aio_base(zaio_t *aio)
{
    return aio->aiobase;
}

/* }}} */

/* {{{ get cache */
void zaio_get_read_cache(zaio_t *aio, zbuf_t *bf, int strict_len)
{
    zbuf_need_space(bf, strict_len+1);
    char *buf = zbuf_data(bf);
    ___zaio_cache_shift(aio, &(aio->read_cache), buf + bf->len, strict_len);
    bf->len += strict_len;
    zbuf_terminate(bf);
}

void zaio_get_write_cache(zaio_t *aio, zbuf_t *bf, int strict_len)
{
    zbuf_need_space(bf, strict_len+1);
    char *buf = zbuf_data(bf);
    ___zaio_cache_shift(aio, &(aio->write_cache), buf + bf->len, strict_len);
    bf->len += strict_len;
    zbuf_terminate(bf);
}

/* }}} */

/* {{{ timeout_tree */

static int ___timeout_cmp(zrbtree_node_t *n1, zrbtree_node_t *n2)
{
    zaio_t *t1 = ZCONTAINER_OF(n1, zaio_t, rbnode_time);
    zaio_t *t2 = ZCONTAINER_OF(n2, zaio_t, rbnode_time);

    long r = t1->cutoff_time - t2->cutoff_time;
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

static int _check_timeout(zaio_base_t *eb)
{
    zrbtree_t *timer_tree = &(eb->timeout_tree);
    if (!zrbtree_have_data(timer_tree)) {
        return 1000;
    }
    while (1) {
        if (eb->stop_flag) {
            return 1000;
        }
        zrbtree_node_t *rn = zrbtree_first(timer_tree);
        if (!rn) {
            return 1000;
        }
        zaio_t *aio = ZCONTAINER_OF(rn, zaio_t, rbnode_time);
        long delay = ztimeout_left_millisecond(aio->cutoff_time);
        if (delay > 0) {
            return (delay>1000?1000:delay);
        }
        zrbtree_detach(timer_tree, rn);
        aio->in_timeout_tree = 0;
        aio->active_stage = active_stage_timeout;
        _zaio_active(aio);
    }
    return 1000;
}


/* }}} */

/* {{{ list for others */
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

/* {{{ aio_base */

zaio_base_t *zaio_base_get_current_pthread_aio_base()
{
    return _zvar_current_base;
}

static void _zaio_base_event_notify_reader(zaio_t *aio)
{
    uint64_t u;
    read(aio->fd, &u, sizeof(uint64_t));
}

zaio_base_t *zaio_base_create()
{
#define ___active_f___(name)    _active_vector[action_##name] = _active_##name;
    ___active_f___(none);
    ___active_f___(readable);
    ___active_f___(read);
    ___active_f___(readn);
    ___active_f___(read_delimiter);
    ___active_f___(read_cint);
    ___active_f___(writeable);
    ___active_f___(writen);
    ___active_f___(tls_connect);
    ___active_f___(tls_accept);
    ___active_f___(sleep);
#undef ___active_f___

    zaio_base_t *eb = (zaio_base_t *)zcalloc(1, sizeof(zaio_base_t));
    zrbtree_init(&(eb->timeout_tree), ___timeout_cmp);

    eb->epoll_fd = epoll_create(1024);
    zclose_on_exec(eb->epoll_fd, 1);

    eb->epoll_event_size = 256;
    eb->epoll_event_vec = (struct epoll_event *)zcalloc(eb->epoll_event_size, sizeof(struct epoll_event));

    pthread_mutex_init(&(eb->extern_plocker), 0);

    int eventfd_fd = eventfd(0, 0);
    zclose_on_exec(eventfd_fd, 1);
    znonblocking(eventfd_fd, 1);

    eb->eventfd_aio = zaio_create(eventfd_fd, eb);
    zaio_readable(eb->eventfd_aio, _zaio_base_event_notify_reader);

    return eb;
}

void zaio_base_free(zaio_base_t *eb)
{
    zaio_free(eb->eventfd_aio, 1);
    zclose(eb->epoll_fd);
    zfree(eb->epoll_event_vec);
    pthread_mutex_destroy(&(eb->extern_plocker));
    zfree(eb);
}

void zaio_base_stop_notify(zaio_base_t *eb)
{
    eb->stop_flag = 1;
    zaio_base_touch(eb);
}

void zaio_base_touch(zaio_base_t *eb)
{
    uint64_t u = 1;
    write(eb->eventfd_aio->fd, &u, sizeof(uint64_t));
}

static void _check_incoming(zaio_base_t *eb)
{
    eb->tmp_queue_head = eb->incoming_queue_head;
    eb->tmp_queue_tail = eb->incoming_queue_tail;
    eb->incoming_queue_head = 0;
    eb->incoming_queue_tail = 0;

    _base_lock_extern(eb);
    ZMLINK_CONCAT(eb->tmp_queue_head, eb->tmp_queue_tail, eb->extern_incoming_queue_head, eb->extern_incoming_queue_tail, zrbtree_left, zrbtree_right);
    eb->extern_incoming_queue_head = 0;
    eb->extern_incoming_queue_tail = 0;
    _base_unlock_extern(eb);

    for (zrbtree_node_t *n = eb->tmp_queue_head; n; n = n->zrbtree_right) {
        zaio_t *aio = (ZCONTAINER_OF(n, zaio_t, rbnode_time));
        aio->in_incoming_queue = 0;
        aio->in_extern_incoming_queue = 0;
        aio->in_tmp_queue = 1;
    }

    while(eb->tmp_queue_head) {
        zrbtree_node_t *n = eb->tmp_queue_head;
        ZMLINK_DETACH(eb->tmp_queue_head, eb->tmp_queue_tail, n, zrbtree_left, zrbtree_right);
        zaio_t *aio = (ZCONTAINER_OF(n, zaio_t, rbnode_time));
        aio->in_tmp_queue = 0;
        aio->active_stage = active_stage_incoming;
        _zaio_active(aio);
    }
}

#define ___zaio_base_run_while_begin while(1) {
#define ___zaio_base_run_while_end }
#define ___zaio_base_check_stop if (eb->stop_flag) { break; }

void zaio_base_run(zaio_base_t *eb, void (*loop_fn)())
{
    _zvar_current_base = eb;
    ___zaio_base_run_while_begin;
    ___zaio_base_check_stop;

    if (loop_fn) {
        loop_fn();
    }

    if (eb->incoming_queue_head || eb->extern_incoming_queue_head) {
        _check_incoming(eb);
    }

    int delay = 1 * 1000;
    if (zrbtree_have_data(&(eb->timeout_tree))) {
        int delay_tmp = _check_timeout(eb);
        if (delay_tmp < delay) {
            delay = delay_tmp;
        }
    } 
    ___zaio_base_check_stop;
    if ((delay < 0) || eb->incoming_queue_head || eb->extern_incoming_queue_head) {
        delay = 0;
    }

    int nfds = epoll_wait(eb->epoll_fd, eb->epoll_event_vec, eb->epoll_event_size, delay);
    if (nfds < 0) {
        if (errno != EINTR) {
            zfatal("FATAL epoll_wait(%m)");
        }
        continue;
    }
    for (int i = 0; i < nfds; i++) {
        ___zaio_base_check_stop;
        zaio_t *aio = (zaio_t *)(eb->epoll_event_vec[i].data.ptr);
        aio->active_stage = active_stage_epoll;
        if (aio->in_timeout_tree) {
            zrbtree_detach(&(eb->timeout_tree), &(aio->rbnode_time));
            aio->in_timeout_tree = 0;
        }
        _zaio_active(aio);
    }
    if ((nfds == eb->epoll_event_size) && (eb->epoll_event_size < aio_rwbuf_size)) {
        eb->epoll_event_size *= 2;
        eb->epoll_event_vec = (struct epoll_event *)zrealloc(eb->epoll_event_vec, (eb->epoll_event_size*sizeof(struct epoll_event)));
    }
    ___zaio_base_run_while_end;

#undef ___zaio_base_run_while_begin
#undef ___zaio_base_run_while_end
#undef ___zaio_base_check_stop
    return;
}

/* }}} */

#endif /* ___ZC_AIO_USE_SSL_INNER___ */

#pragma pack(pop)

/* Local variables:
* End:
* vim600: fdm=marker
*/

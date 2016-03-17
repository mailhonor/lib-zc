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
#include <pthread.h>
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

#define ZEV_INNER_CB_FN_CONNECT             0X01

#define ZEVBASE_LOCK(eb)	pthread_mutex_lock((pthread_mutex_t *)((eb)->locker))
#define ZEVBASE_UNLOCK(eb)	pthread_mutex_unlock((pthread_mutex_t *)((eb)->locker));

/* ################################################################## */
/* connect */

static int inline zev_connect_cb(zev_t * zev)
{
    int sock;
    int error;
    socklen_t error_len;

    sock = zev_get_fd(zev);

    if (zev_get_events(zev) & ZEV_WRITE)
    {
        error = 0;
        error_len = sizeof(error);
        if (getsockopt(sock, SOL_SOCKET, SO_ERROR, (char *)&error, &error_len) < 0)
        {
            zev->recv_events = ZEV_ERROR;
        }
        if (error)
        {
            errno = error;
            zev->recv_events = ZEV_ERROR;
        }
    }
    zev->callback(zev);

    return 0;
}

static inline int ___zev_connect(zev_t * zev, struct sockaddr *sa, socklen_t len, zev_cb_t callback)
{
    int sock;

    sock = zev_get_fd(zev);
    if (connect(sock, sa, len))
    {
        zev->recv_events = ZEV_WRITE;
        zev_attach(zev, callback);
        return 0;
    }

    if (errno != EINPROGRESS)
    {
        zev->recv_events = ZEV_ERROR;
        zev_attach(zev, callback);
        return 0;
    }

    zev->inner_callback = ZEV_INNER_CB_FN_CONNECT;
    zev_set(zev, ZEV_WRITE, callback);

    return 0;

}

int zev_inet_connect(zev_t * zev, char *dip, int port, zev_cb_t callback)
{
    int sock;
    struct sockaddr_in addr;
    int on = 1;

    sock = zev_get_fd(zev);
    znonblocking(sock, 1);
    (void)setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char *)&on, sizeof(on));

    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(dip);
    ___zev_connect(zev, (struct sockaddr *)&addr, sizeof(struct sockaddr_in), callback);

    return 0;
}

int zev_unix_connect(zev_t * zev, char *path, zev_cb_t callback)
{
    struct sockaddr_un addr;
    int len = strlen(path);

    memset((char *)&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    if (len < (int)sizeof(addr.sun_path))
    {
        memcpy(addr.sun_path, path, len + 1);
    }
    else
    {
        zev->recv_events = ZEV_ERROR;
        zev_attach(zev, callback);
        return 0;
    }
    ___zev_connect(zev, (struct sockaddr *)&addr, sizeof(struct sockaddr_un), callback);

    return 0;
}

/* ################################################################## */
/* ev/event/trigger */

int zev_attach(zev_t * ev, zev_cb_t callback)
{
    zevbase_t *eb = zev_get_base(ev);

    ev->callback = callback;
    zevbase_queue_enter(eb, (zevbase_cb_t) (-1), ev);

    return 0;
}

int zev_stop(zev_t * ev)
{
    zev_set(ev, 0, 0);

    return 0;
}

int zev_set(zev_t * ev, int events, zev_cb_t callback)
{
    int fd, old_events, e_events;
    struct epoll_event epv;
    zevbase_t *aio;

    aio = ev->evbase;
    fd = zev_get_fd(ev);
    e_events = EPOLLHUP | EPOLLRDHUP | EPOLLERR;

    ev->callback = callback;
    old_events = ev->events;
    ev->events = events;
    if (callback == 0)
    {
        events = 0;
    }

    if (events == 0)
    {
        if (old_events)
        {
            if (epoll_ctl(aio->epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1)
            {
                zfatal("zev_set: fd %d: DEL  error: %m", fd);
            }
        }
    }
    else if (old_events != events)
    {
        if (events & ZEV_READ)
        {
            e_events |= EPOLLIN;
        }
        if (events & ZEV_WRITE)
        {
            e_events |= EPOLLOUT;
        }
        if (events & ZEV_PERSIST)
        {
            e_events |= EPOLLET;
        }
        epv.events = e_events;
        epv.data.ptr = ev;
        if (epoll_ctl(aio->epoll_fd, (old_events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD), fd, &epv) == -1)
        {
            zfatal("zev_set: fd %d: %s error: %m", fd, (old_events ? "MOD" : "ADD"));
        }
    }

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
    if (ev->inner_callback == 0)
    {
        if (ev->callback)
        {
            ev->callback(ev);
        }
        else
        {
            zev_set(ev, 0, 0);
        }
    }
    else if (ev->inner_callback == ZEV_INNER_CB_FN_CONNECT)
    {
        ev->inner_callback = 0;
        zev_connect_cb(ev);
    }
    else
    {
        ev->inner_callback = 0;
        zev_set(ev, 0, 0);
    }

    return 0;
}

/* ################################################################## */
/* aio */

static int ___zaio_read_n_TRUE(zaio_t * aio, int strict_len, zaio_cb_t callback);
static int zaio_event_set(zaio_t * aio, int ev_type);
static int ZAIO_P2_MAX = ZAIO_RWBUF_SIZE - 1;
static void zaio_ready_do(zaio_t * aio);

static inline void ___zaio_cache_shift(zaio_t * aio, zaio_rwbuf_list_t * ioc, void *data, int len)
{
    int rlen, i, olen = len;
    char *buf = (char *)data;
    char *cdata;
    zaio_rwbuf_t *rwb;
    zevbase_t *eb;

    eb = zaio_get_base(aio);

    while (len > 0)
    {
        rwb = ioc->head;
        rlen = rwb->p2 - rwb->p1 + 1;
        if (data)
        {
            if (len >= rlen)
            {
                i = rlen;
            }
            else
            {
                i = len;
            }
            cdata = rwb->data + rwb->p1;
            while (i--)
            {
                *buf++ = *cdata++;
            }
        }
        if (len >= rlen)
        {
            rwb = rwb->next;
            zmcot_free_one((eb)->aio_rwbuf_mpool, ioc->head);
            ioc->head = rwb;
            len -= rlen;
        }
        else
        {
            rwb->p1 += len;
            len = 0;
        }
    }
    if (ioc->head == 0)
    {
        ioc->tail = 0;
    }
    ioc->len -= olen;
}

static void ___zaio_cache_first_line(zaio_t * aio, zaio_rwbuf_list_t * ioc, char **data, int *len)
{
    zaio_rwbuf_t *rwb;

    rwb = ioc->head;
    if (!rwb)
    {
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
    zevbase_t *eb;

    eb = zaio_get_base(aio);

    rwb = ioc->tail;
    p2 = 0;
    cdata = 0;
    if (rwb)
    {
        p2 = rwb->p2;
        cdata = rwb->data;
    }
    for (i = 0; i < len; i++)
    {
        if (!rwb || (p2 == ZAIO_P2_MAX))
        {
            rwb = (zaio_rwbuf_t *) zmcot_alloc_one(eb->aio_rwbuf_mpool);
            rwb->next = 0;
            rwb->p1 = 0;
            rwb->p2 = 0;
            if (ioc->tail)
            {
                ioc->tail->next = rwb;
                ioc->tail->p2 = p2;
            }
            else
            {
                ioc->head = rwb;
            }
            ioc->tail = rwb;
            cdata = rwb->data;
            p2 = rwb->p2;
        }
        else
        {
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
    if (rlen < 1)
    {
        status = SSL_get_error(aio->ssl->ssl, rlen);
        if (status == SSL_ERROR_WANT_WRITE)
        {
            aio->ssl->read_want_write = 1;
        }
        else if (status == SSL_ERROR_WANT_READ)
        {
            aio->ssl->read_want_read = 1;
        }
        else
        {
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
    if (rlen < 1)
    {
        status = SSL_get_error(aio->ssl->ssl, rlen);
        if (status == SSL_ERROR_WANT_WRITE)
        {
            aio->ssl->write_want_write = 1;
        }
        else if (status == SSL_ERROR_WANT_READ)
        {
            aio->ssl->write_want_read = 1;
        }
        else
        {
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
    if (rlen < 1)
    {
        status = SSL_get_error(aio->ssl->ssl, rlen);
        if (status == SSL_ERROR_WANT_WRITE)
        {
            aio->ssl->write_want_write = 1;
        }
        else if (status == SSL_ERROR_WANT_READ)
        {
            aio->ssl->write_want_read = 1;
        }
        else
        {
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
    if (rlen < 1)
    {
        status = SSL_get_error(aio->ssl->ssl, rlen);
        if (status == SSL_ERROR_WANT_WRITE)
        {
            aio->ssl->read_want_write = 1;
        }
        else if (status == SSL_ERROR_WANT_READ)
        {
            aio->ssl->read_want_read = 1;
        }
        else
        {
            aio->ssl->error = 1;
        }
        return -1;
    }

    return 1;
}

int zaio_ssl_init(zaio_t * aio, zsslctx_t * ctx, zaio_cb_t callback)
{
    zaio_ssl_t *zssl;
    SSL *ssl;
    int rlen;

    ssl = SSL_new(ctx->ssl_ctx);
    SSL_set_fd(ssl, aio->fd);

    zssl = (zaio_ssl_t *) zcalloc(1, sizeof(zaio_ssl_t));
    zssl->ssl = ssl;
    zssl->server_or_client = ctx->server_or_client;

    aio->ssl = zssl;

    aio->rw_type = ZAIO_CB_TYPE_SSL_INIT;
    aio->callback = callback;

    if (zssl->server_or_client)
    {
        rlen = zaio_try_ssl_accept(aio);
    }
    else
    {
        rlen = zaio_try_ssl_connect(aio);
    }

    if (rlen > 0)
    {
        zaio_ready_do(aio);
        return 0;
    }

    zaio_event_set(aio, 1);

    return 0;
}

void zaio_ssl_fini(zaio_t * aio)
{
    if (!(aio->ssl))
    {
        return;
    }
    if (aio->ssl->ssl)
    {
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

    if (!assl->ssl)
    {
        return 0;
    }

    ssl = assl->ssl;
    assl->ssl = 0;

    return ssl;
}

int zaio_move(zaio_t * aio, zevbase_t * neb, zaio_cb_t attach_callback)
{
    int fd;
    zaio_ssl_t *assl;

    fd = zaio_get_fd(aio);
    zaio_event_set(aio, 0);
    assl = zaio_ssl_detach(aio);

    /* FIXME readbuf,writebuf may be not blank */
    zaio_fini(aio);

    zaio_init(aio, neb, fd);
    zaio_ssl_attach(aio, assl);

    zaio_attach(aio, attach_callback);

    return 0;
}

static void zaio_ready_do_once(zaio_t * aio)
{
    zaio_cb_t callback;
    char worker_buf[1100000];
    unsigned int rw_type;
    int ret = 0;

    callback = aio->callback;
    rw_type = aio->rw_type;
    ret = aio->ret;

    /* SSL connect/accept */
    if (rw_type == ZAIO_CB_TYPE_SSL_INIT)
    {
        if (callback)
        {
            (callback) (aio, 0);
        }
        return;
    }

    /* SLEEP */
    if (rw_type == ZAIO_CB_TYPE_SLEEP)
    {
        if (callback)
        {
            aio->ret = 1;
            (callback) (aio, 0);
        }
        return;
    }
    /* WRITE */
    if ((rw_type & ZAIO_CB_WRITE))
    {
        if (callback)
        {
            (callback) (aio, 0);
        }
        return;
    }
    /* READ */
    if (ret < 1)
    {
        if (callback)
        {
            (callback) (aio, worker_buf);
        }
        return;
    }

    ___zaio_cache_shift(aio, &(aio->read_cache), worker_buf, ret);
    if (rw_type == ZAIO_CB_TYPE_READ_N && aio->delimiter == 'N')
    {
        int LLL = 0, i;
        unsigned char *p = (unsigned char *)worker_buf;;
        for (i = 0; i < 4; i++)
        {
            LLL = LLL * 256 + p[i];
        }
        aio->delimiter = '\0';
        if (LLL > 1024 * 1024 || LLL < 0)
        {
            aio->ret = -1;
            if (callback)
            {
                (callback) (aio, worker_buf);
            }
            return;
        }
        if (LLL == 0)
        {
            if (callback)
            {
                aio->ret = 0;
                (callback) (aio, worker_buf);
            }
            return;
        }
        ___zaio_read_n_TRUE(aio, LLL, callback);
    }
    else if (callback)
    {
        worker_buf[ret] = 0;
        (callback) (aio, worker_buf);
    }
}

static void zaio_ready_do(zaio_t * aio)
{
    zevbase_t *eb = aio->evbase;

    eb->magic_aio = 0;
    if (aio->in_loop)
    {
        eb->magic_aio = aio;
        return;
    }
    aio->in_loop = 1;
    eb->magic_aio = aio;
    while (eb->magic_aio)
    {
        eb->magic_aio = 0;
        zaio_ready_do_once(aio);
    }
    aio->in_loop = 0;
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
    zaio_event_set(aio, 0);

    if (aio->read_cache.len > 0)
    {
        ___zaio_cache_shift(aio, &(aio->read_cache), 0, aio->read_cache.len);
    }
    if (aio->write_cache.len > 0)
    {
        ___zaio_cache_shift(aio, &(aio->write_cache), 0, aio->write_cache.len);
    }
    if (aio->ssl)
    {
        zaio_ssl_fini(aio);
    }
}

void zaio_reset_base(zaio_t * aio, zevbase_t * eb_new)
{
    zaio_event_set(aio, 0);

    if (aio->read_cache.len > 0)
    {
        ___zaio_cache_shift(aio, &(aio->read_cache), 0, aio->read_cache.len);
    }
    if (aio->write_cache.len > 0)
    {
        ___zaio_cache_shift(aio, &(aio->write_cache), 0, aio->write_cache.len);
    }
    aio->evbase = eb_new;
}

static int zaio_event_set(zaio_t * aio, int ev_type)
{
    zevbase_t *eb;
    int fd, events, old_events, e_events;
    struct epoll_event evt;
    int rw_type;

    eb = aio->evbase;
    fd = aio->fd;
    rw_type = aio->rw_type;
    events = 0;

    if (ev_type == 0)
    {
        /* clear event; */
        goto evdo;
    }
    /* compute the events */
    if (!(aio->ssl))
    {
        if ((rw_type & ZAIO_CB_MAGIC) == ZAIO_CB_READ)
        {
            events |= ZEV_READ;
        }
        if ((rw_type & ZAIO_CB_MAGIC) == ZAIO_CB_WRITE)
        {
            events |= ZEV_WRITE;
        }
        if (aio->write_cache.len > 0)
        {
            events |= ZEV_WRITE;
        }
    }
    else
    {
        if (aio->ssl->read_want_read || aio->ssl->write_want_read)
        {
            events |= ZEV_READ;
        }
        if (aio->ssl->read_want_write || aio->ssl->write_want_write)
        {
            events |= ZEV_WRITE;
        }
        if (aio->write_cache.len > 0)
        {
            if ((aio->ssl->write_want_read == 0) && (aio->ssl->write_want_write == 0))
            {
                aio->ssl->write_want_write = 1;
                events |= ZEV_WRITE;
            }
        }
    }
    /* compute over */

evdo:
    e_events = EPOLLHUP | EPOLLRDHUP | EPOLLERR;
    old_events = aio->events;
    aio->events = events;

    if (events == 0)
    {
        if (old_events)
        {
            if (epoll_ctl(eb->epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1)
            {
                zfatal("zaio_event_set: fd %d: DEL  error: %m", fd);
            }
        }
    }
    else if (old_events != events)
    {
        if (events & ZEV_READ)
        {
            e_events |= EPOLLIN;
        }
        if (events & ZEV_WRITE)
        {
            e_events |= EPOLLOUT;
        }
        if (events & ZEV_PERSIST)
        {
            e_events |= EPOLLET;
        }
        evt.events = e_events;
        evt.data.ptr = aio;
        if (epoll_ctl(eb->epoll_fd, (old_events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD), fd, &evt) == -1)
        {
            zfatal("zaio_event_set: fd %d: %s error: %m", fd, (old_events ? "MOD" : "ADD"));
        }
    }

    return 0;
}

int zaio_attach(zaio_t * aio, zaio_cb_t callback)
{
    zevbase_t *eb;

    aio->callback = callback;

    eb = zaio_get_base(aio);
    zevbase_queue_enter(eb, (zevbase_cb_t) (-2), aio);

    return 0;
}

int zaio_stop(zaio_t * aio)
{
    zaio_event_set(aio, 0);

    return 0;
}

int zaio_read(zaio_t * aio, int max_len, zaio_cb_t callback)
{
    int magic, rlen;
    char buf[10240];

    aio->rw_type = ZAIO_CB_TYPE_READ;
    aio->callback = callback;
    aio->read_magic_len = max_len;

    if (max_len < 1)
    {
        aio->ret = 0;
        zaio_ready_do(aio);
        return 0;
    }

    while (1)
    {
        if (aio->read_cache.len == 0)
        {
            magic = -1;
        }
        else if (aio->read_cache.len > max_len)
        {
            magic = max_len;
        }
        else
        {
            magic = aio->read_cache.len;
        }

        if (magic > 0)
        {
            aio->ret = magic;
            zaio_ready_do(aio);
            return 0;
        }
        if (!(aio->ssl))
        {
            rlen = read(aio->fd, buf, 10240);
        }
        else
        {
            rlen = zaio_try_ssl_read(aio, buf, 10240);
        }
        if (rlen < 1)
        {
            break;
        }
        ___zaio_cache_append(aio, &(aio->read_cache), buf, rlen);
    }

    zaio_event_set(aio, 1);

    return 0;
}

static int ___zaio_read_n_TRUE(zaio_t * aio, int strict_len, zaio_cb_t callback)
{
    int magic, rlen;
    char buf[10240];

    aio->rw_type = ZAIO_CB_TYPE_READ_N;
    aio->callback = callback;
    aio->read_magic_len = strict_len;

    if (strict_len < 1)
    {
        aio->ret = 0;
        zaio_ready_do(aio);
        return 0;
    }

    while (1)
    {
        if (aio->read_cache.len < strict_len)
        {
            magic = -1;
        }
        else
        {
            magic = strict_len;
        }
        if (magic > 0)
        {
            aio->ret = magic;
            zaio_ready_do(aio);
            return 0;
        }
        if (!(aio->ssl))
        {
            rlen = read(aio->fd, buf, 10240);
        }
        else
        {
            rlen = zaio_try_ssl_read(aio, buf, 10240);
        }
        if (rlen < 1)
        {
            break;
        }
        ___zaio_cache_append(aio, &(aio->read_cache), buf, rlen);
    }

    zaio_event_set(aio, 1);

    return 0;
}

int zaio_read_n(zaio_t * aio, int strict_len, zaio_cb_t callback)
{
    aio->delimiter = '\0';
    return ___zaio_read_n_TRUE(aio, strict_len, callback);
}

static inline int ___zaio_read_delimiter_check(zaio_t * aio, unsigned char delimiter, int max_len)
{
    int magic, i, end;
    char *buf;
    zaio_rwbuf_t *rwb;

    if (aio->read_cache.len == 0)
    {
        return -1;
    }

    magic = 0;
    for (rwb = aio->read_cache.head; rwb; rwb = rwb->next)
    {
        buf = rwb->data;
        end = rwb->p2 + 1;
        for (i = rwb->p1; i != end; i++)
        {
            magic++;
            if (buf[i] == delimiter)
            {
                return magic;
            }
            if (magic == max_len)
            {
                return magic;
            }
        }
    }

    return -1;

}

int zaio_read_delimiter(zaio_t * aio, char delimiter, int max_len, zaio_cb_t callback)
{
    int magic, rlen;
    char buf[10240];
    char *data;

    aio->rw_type = ZAIO_CB_TYPE_READ_DELIMETER;
    aio->callback = callback;
    aio->read_magic_len = max_len;
    aio->delimiter = delimiter;

    if (max_len < 1)
    {
        aio->ret = 0;
        zaio_ready_do(aio);
        return 0;
    }
    magic = ___zaio_read_delimiter_check(aio, (unsigned char)delimiter, max_len);
    while (1)
    {
        if (magic > 0)
        {
            aio->ret = magic;
            zaio_ready_do(aio);
            return 0;
        }
        if (!(aio->ssl))
        {
            rlen = read(aio->fd, buf, 10240);
        }
        else
        {
            rlen = zaio_try_ssl_read(aio, buf, 10240);
        }
        if (rlen < 1)
        {
            break;
        }
        ___zaio_cache_append(aio, &(aio->read_cache), buf, rlen);
        data = memchr(buf, aio->delimiter, rlen);
        if (data)
        {
            magic = aio->read_cache.len - rlen + (data - buf + 1);
            if (magic > aio->read_magic_len)
            {
                magic = aio->read_magic_len;
            }
        }
        else
        {
            if (aio->read_magic_len <= aio->read_cache.len)
            {
                magic = aio->read_magic_len;
            }
            else
            {
                magic = -1;
            }
        }
    }

    zaio_event_set(aio, 1);

    return 0;
}

int zaio_read_sizedata(zaio_t * aio, zaio_cb_t callback)
{
    aio->delimiter = 'N';
    return ___zaio_read_n_TRUE(aio, 4, callback);
}

int zaio_write_cache_append(zaio_t * aio, void *buf, int len)
{
    if (len < 1)
    {
        return 0;
    }
    ___zaio_cache_append(aio, &(aio->write_cache), buf, len);

    return len;
}

int zaio_write_cache_append_sizedata(zaio_t * aio, int len, void *buf)
{
    char p[8];

    p[0] = (len >> 24) & 0xFF;
    p[1] = (len >> 16) & 0xFF;
    p[2] = (len >> 8) & 0xFF;
    p[3] = (len) & 0xFF;
    zaio_write_cache_append(aio, p, 4);
    zaio_write_cache_append(aio, buf, len);

    return 0;
}

int zaio_write_cache_flush(zaio_t * aio, zaio_cb_t callback)
{
    aio->ret = 1;
    aio->rw_type = ZAIO_CB_TYPE_WRITE;
    aio->callback = callback;

    zaio_event_set(aio, 1);

    return 0;
}

int zaio_write_cache_get_len(zaio_t * aio)
{
    return aio->write_cache.len;
}

int zaio_sleep(zaio_t * aio, zaio_cb_t callback)
{
    aio->rw_type = ZAIO_CB_TYPE_SLEEP;
    aio->callback = callback;

    zaio_event_set(aio, 0);

    return 0;
}

static inline int zaio_action_read_once(zaio_t * aio, int rw_type, char *buf)
{
    int rlen, rlen_t;
    char *data;

    rlen_t = aio->read_magic_len - aio->read_cache.len;
    if (rlen_t < 10240)
    {
        rlen_t = 10240;
    }
    if (!(aio->ssl))
    {
        rlen = read(aio->fd, buf, rlen_t);
    }
    else
    {
        rlen = zaio_try_ssl_read(aio, buf, rlen_t);
    }
    if (rlen < 1)
    {
        return -1;
    }
    buf[rlen] = 0;
    ___zaio_cache_append(aio, &(aio->read_cache), buf, rlen);

    if (rw_type == ZAIO_CB_TYPE_READ)
    {
        if (aio->read_cache.len >= aio->read_magic_len)
        {
            aio->ret = aio->read_magic_len;
        }
        else
        {
            aio->ret = aio->read_cache.len;
        }
        return 0;
    }
    if (rw_type == ZAIO_CB_TYPE_READ_N)
    {
        if (aio->read_cache.len >= aio->read_magic_len)
        {
            aio->ret = aio->read_magic_len;
            return 0;
        }
        else
        {
            return 1;
        }
    }
    if (rw_type == ZAIO_CB_TYPE_READ_DELIMETER)
    {
        data = memchr(buf, aio->delimiter, rlen);
        if (data)
        {
            aio->ret = aio->read_cache.len - rlen + (data - buf + 1);
            if (aio->ret > aio->read_magic_len)
            {
                aio->ret = aio->read_magic_len;
            }
            return 0;
        }
        else
        {
            if (aio->read_magic_len <= aio->read_cache.len)
            {
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

    if ((aio->ssl) && (!aio->ssl->session_init))
    {
        ret = zaio_try_ssl_accept(aio);
        if (ret > 0)
        {
            aio->ssl->session_init = 1;
            aio->ret = 1;
            return 0;
        }

        return 1;
    }

    while (1)
    {
        ret = zaio_action_read_once(aio, rw_type, buf);
        if (ret == -1)
        {
            return 1;
        }
        if (ret == 0)
        {
            return 0;
        }
    }

    return 0;
}

static inline int zaio_action_write(zaio_t * aio, int rw_type)
{
    int wlen, rlen, len;
    char *data;

    if ((aio->ssl) && (!aio->ssl->session_init))
    {
        rlen = zaio_try_ssl_connect(aio);
        if (rlen > 0)
        {
            aio->ssl->session_init = 1;
            aio->ret = 1;
            return 0;
        }

        return 1;
    }
    while (1)
    {
        ___zaio_cache_first_line(aio, &(aio->write_cache), &data, &len);
        if (len == 0)
        {
            aio->ret = 1;
            return 0;
        }
        wlen = len;
        if (!(aio->ssl))
        {
            rlen = write(aio->fd, data, wlen);
        }
        else
        {
            rlen = zaio_try_ssl_write(aio, data, wlen);
        }
        if (rlen < 1)
        {
            return 1;
        }
        ___zaio_cache_shift(aio, &(aio->write_cache), 0, rlen);
    }

    return 1;
}

static inline int zaio_action(zaio_t * aio)
{
    unsigned int rw_type;
    int events, transfer;
    zaio_ssl_t *ssl = aio->ssl;

    transfer = 1;
    rw_type = aio->rw_type;
    events = aio->recv_events;

    if (events & ZEV_EXCEPTION)
    {
        aio->ret = -1;
        transfer = 0;
        goto transfer;
    }
#define _ssl_w (((ssl->write_want_write) &&(events & ZEV_WRITE))||((ssl->write_want_read) &&(events & ZEV_READ)))
#define _ssl_r (((ssl->read_want_write) &&(events & ZEV_WRITE))||((ssl->read_want_read) &&(events & ZEV_READ)))
    if (((aio->ssl) && (_ssl_w)) || ((!(aio->ssl)) && (events & ZEV_WRITE)))
    {
        transfer = zaio_action_write(aio, rw_type);
        if (rw_type & ZAIO_CB_WRITE)
        {
            goto transfer;
        }
        transfer = 1;
        goto transfer;
    }

    if (((aio->ssl) && (_ssl_r)) || ((!(aio->ssl)) && (events & ZEV_READ)))
    {
        transfer = zaio_action_read(aio, rw_type);
        goto transfer;
    }

    aio->ret = -1;
    transfer = 0;

transfer:
    if (aio->ssl && aio->ssl->error)
    {
        aio->ret = -1;
        transfer = 0;
    }

    if (transfer)
    {
        zaio_event_set(aio, 1);
        return 1;
    }

    zaio_ready_do(aio);

    return 0;
}

/* ################################################################## */
/* timer */

static int ztimer_cmp(zrbtree_node_t * n1, zrbtree_node_t * n2)
{
    ztimer_t *t1, *t2;
    long r;

    t1 = ZCONTAINER_OF(n1, ztimer_t, rbnode_time);
    t2 = ZCONTAINER_OF(n2, ztimer_t, rbnode_time);

    r = t1->timeout - t2->timeout;

    if (!r)
    {
        r = (char *)(n1) - (char *)(n2);
    }
    if (r > 0)
    {
        return 1;
    }
    else if (r < 0)
    {
        return -1;
    }

    return 0;
}

static void ztimer_base_init(zrbtree_t * timer_tree)
{
    zrbtree_init(timer_tree, ztimer_cmp);
}

static int inline ztimer_check(zrbtree_t * timer_tree)
{
    ztimer_t *timer;
    zrbtree_node_t *rn;
    long delay = 10 * 1000;
    ztimer_cb_t callback;

    if (!zrbtree_have_data(timer_tree))
    {
        return delay;
    }
    while (1)
    {
        delay = 0;
        rn = zrbtree_first(timer_tree);
        if (!rn)
        {
            break;
        }
        timer = ZCONTAINER_OF(rn, ztimer_t, rbnode_time);
        delay = ztimeout_left(timer->timeout);
        if (delay > 0)
        {
            break;
        }
        callback = timer->callback;
        zrbtree_detach(timer_tree, &(timer->rbnode_time));
        timer->in_time = 0;
        if (callback)
        {
            callback(timer);
        }
    }

    return delay;
}

ztimer_t *ztimer_create(void)
{
    return (ztimer_t *) zcalloc(1, sizeof(ztimer_t));
}

void ztimer_free(ztimer_t * timer)
{
    zfree(timer);
}

void ztimer_init(ztimer_t * timer, zevbase_t * eb)
{
    memset(timer, 0, sizeof(ztimer_t));
    timer->evbase = eb;
}

void ztimer_fini(ztimer_t * timer)
{
    ztimer_start(timer, 0, 0);
}

int ztimer_start(ztimer_t * timer, ztimer_cb_t callback, int timeout)
{
    zrbtree_t *timer_tree;

    timer_tree = &(timer->evbase->timer_tree);

    if ((timer->in_time) && (timeout != -1))
    {
        zrbtree_detach(timer_tree, &(timer->rbnode_time));
        timer->in_time = 0;
    }
    if (timeout > 0)
    {
        timer->callback = callback;
        timer->enable_time = 1;
        timer->timeout = ztimeout_set(timeout);
        zrbtree_attach(timer_tree, &(timer->rbnode_time));
        timer->in_time = 1;
    }
    else if (timeout == 0)
    {
        timer->enable_time = 0;
    }
    else if (timeout == -1)
    {
        if ((timer->enable_time) && (!timer->in_time))
        {
            zrbtree_attach(timer_tree, &(timer->rbnode_time));
        }
    }

    return 0;
}

int ztimer_pause(ztimer_t * timer)
{
    return ztimer_start(timer, timer->callback, 0);
}

int ztimer_continue(ztimer_t * timer)
{
    return ztimer_start(timer, timer->callback, -1);
}

int ztimer_stop(ztimer_t * timer)
{
    return ztimer_start(timer, 0, 0);
}

int ztimer_attach(ztimer_t * timer, ztimer_cb_t callback)
{
    zevbase_t *eb;

    timer->callback = callback;

    eb = zaio_get_base(timer);
    zevbase_queue_enter(eb, (zevbase_cb_t) (-3), timer);

    return 0;
}

int ztimer_detach(ztimer_t * timer)
{
    ztimer_start(timer, 0, 0);
    return 0;
}

/* ################################################################## */
/* evbase */

zevbase_t *zvar_evbase = 0;
void zvar_evbase_init(void)
{
    if (! zvar_evbase)
    {
        zvar_evbase = zevbase_create();
    }
}

static int zevbase_notify_reader(zev_t * ev)
{
    uint64_t u;
    int ret;

    ret = read(zev_get_fd(ev), &u, sizeof(uint64_t));

    return ret;
}

int zevbase_notify(zevbase_t * eb)
{
    uint64_t u;
    int ret;

    u = 1;
    ret = write(eb->eventfd_event.fd, &u, sizeof(uint64_t));

    return ret;
}

zevbase_t *zevbase_create(void)
{
    zevbase_t *eb;
    int eventfd_fd;

    eb = (zevbase_t *) zcalloc(1, sizeof(zevbase_t) + sizeof(pthread_mutex_t));

    eb->locker = (char *)eb + sizeof(zevbase_t);
    pthread_mutex_init(eb->locker, 0);

    ztimer_base_init(&(eb->timer_tree));

    eb->aio_rwbuf_mpool = zmcot_create(sizeof(zaio_rwbuf_t));
    eb->queue_mpool = zmcot_create(sizeof(zevbase_queue_t));

    eb->epoll_fd = epoll_create(1024);
    zclose_on_exec(eb->epoll_fd, 1);

    eb->epoll_event_count = 32;
    eb->epoll_event_list = (struct epoll_event *)zmalloc(sizeof(struct epoll_event) * eb->epoll_event_count);

    eventfd_fd = eventfd(0, 0);
    zclose_on_exec(eventfd_fd, 1);
    znonblocking(eventfd_fd, 1);

    zev_init(&(eb->eventfd_event), eb, eventfd_fd);
    zev_set(&(eb->eventfd_event), ZEV_READ, zevbase_notify_reader);

    return eb;
}

static int inline zevbase_queue_checker(zevbase_t * eb)
{
    zaio_t *aio;
    zev_t *ev;
    ztimer_t *timer;

    zevbase_queue_t *node;
    zevbase_cb_t callback;
    void *context;

    while (eb->queue_head)
    {
        ZEVBASE_LOCK(eb);
        node = eb->queue_head;
        if (!node)
        {
            ZEVBASE_UNLOCK(eb);
            break;
        }
        ZMLINK_DETACH(eb->queue_head, eb->queue_tail, node, prev, next);
        callback = node->callback;
        context = node->context;
        zmcot_free_one(eb->queue_mpool, node);
        ZEVBASE_UNLOCK(eb);

        if (callback)
        {
            if (callback == (zevbase_cb_t) (-1))
            {
                ev = (zev_t *) context;
                ev->inner_callback = 0;
                if (ev->callback)
                {
                    ev->callback(ev);
                }
            }
            else if (callback == (zevbase_cb_t) (-2))
            {
                aio = (zaio_t *) context;
                if (aio->callback)
                {
                    aio->ret = 0;
                    aio->callback(aio, 0);
                }
            }
            else if (callback == (zevbase_cb_t) (-3))
            {
                timer = (ztimer_t *) context;
                if (timer->callback)
                {
                    timer->callback(timer);
                }
            }
            else
            {
                callback(eb, context);
            }
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

    if ((delay < 1) || (delay > 10 * 1000))
    {
        delay = 10 * 1000;
    }

    if (zrbtree_have_data(&(eb->timer_tree)))
    {
        delay_tmp = ztimer_check(&(eb->timer_tree));
        if (delay_tmp < delay)
        {
            delay = delay_tmp;
        }
    }

    if (eb->queue_head)
    {
        zevbase_queue_checker(eb);
    }

    nfds = epoll_wait(eb->epoll_fd, eb->epoll_event_list, eb->epoll_event_count, delay);
    if (nfds == -1)
    {
        if (errno != EINTR)
        {
            zfatal("zbase_dispatch: epoll_wait: %m");
        }
        return -1;
    }

    for (i = 0; i < nfds; i++)
    {
        epv = eb->epoll_event_list + i;
        events = epv->events;
        recv_events = 0;
        if (events & EPOLLHUP)
        {
            recv_events |= ZEV_HUP;
        }
        if (events & EPOLLRDHUP)
        {
            recv_events |= ZEV_RDHUP;
        }
        if (events & EPOLLERR)
        {
            recv_events |= ZEV_ERROR;
        }
        if (events & EPOLLIN)
        {
            recv_events |= ZEV_READ;
        }
        if (events & EPOLLOUT)
        {
            recv_events |= ZEV_WRITE;
        }
        aio = (zaio_t *) (epv->data.ptr);
        if (aio->aio_type == ZEV_TYPE_EVENT)
        {
            ev = (zev_t *) aio;
            ev->recv_events = recv_events;
            zev_action(ev);
        }
        else if (1 || (aio->aio_type == ZEV_TYPE_AIO))
        {
            aio->recv_events = recv_events;
            zaio_action(aio);
        }
    }
    if (nfds == eb->epoll_event_count && nfds < 4096)
    {
        eb->epoll_event_count *= 2;
        eb->epoll_event_list = (struct epoll_event *)zrealloc(eb->epoll_event_list, sizeof(struct epoll_event) * (eb->epoll_event_count));
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
    zmcot_free(eb->queue_mpool);

    zfree(eb);
}

int zevbase_queue_enter(zevbase_t * eb, zevbase_cb_t callback, void *context)
{
    zevbase_queue_t *qnode;

    ZEVBASE_LOCK(eb);
    qnode = (zevbase_queue_t *) zmcot_alloc_one(eb->queue_mpool);
    qnode->callback = callback;
    qnode->context = context;

    ZMLINK_APPEND(eb->queue_head, eb->queue_tail, qnode, prev, next);
    ZEVBASE_UNLOCK(eb);

    zevbase_notify(eb);

    return 0;
}

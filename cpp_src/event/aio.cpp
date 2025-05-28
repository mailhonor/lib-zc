/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2022-01-16
 * ================================
 */

#ifdef __linux__

#pragma GCC diagnostic ignored "-Wpacked-not-aligned"

#include "zcc/zcc_aio.h"
#include "zcc/zcc_openssl.h"
#include "zcc/zcc_rbtree.h"
#include "zcc/zcc_link.h"
#include "zcc/zcc_intpack.h"
#include <arpa/inet.h>
#include <errno.h>
#include <openssl/ssl.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/inotify.h>
#include <sys/time.h>
#include <sys/un.h>
#include <unistd.h>

zcc_namespace_begin;

#define event_read 0X01
#define event_write 0X02

#define aio_rwbuf_size 4096

#define _zpthread_lock(l)                               \
    {                                                   \
        if (pthread_mutex_lock((pthread_mutex_t *)(l))) \
        {                                               \
            zcc_fatal("mutex:%m");                      \
        }                                               \
    }
#define _zpthread_unlock(l)                               \
    {                                                     \
        if (pthread_mutex_unlock((pthread_mutex_t *)(l))) \
        {                                                 \
            zcc_fatal("mutex:%m");                        \
        }                                                 \
    }

#define _base_lock_extern(eb)                    \
    {                                            \
        _zpthread_lock(&((eb)->extern_plocker)); \
    }
#define _base_unlock_extern(eb)                    \
    {                                              \
        _zpthread_unlock(&((eb)->extern_plocker)); \
    }

#define _function_pthread_is_self(eb) (_current_base == (eb))

#define _ready_go_and_return(engine_, ret_value)                                                     \
    {                                                                                                \
        if ((engine_->action_type != action_readable) && (engine_->action_type != action_writeable)) \
        {                                                                                            \
            _aio_event_monitor(engine_, monitor_clear);                                              \
        }                                                                                            \
        engine_->ret = ret_value;                                                                    \
        engine_->callback();                                                                         \
        return;                                                                                      \
    }

#define _want_read_or_write(obj, r, w) \
    {                                  \
        (obj)->is_want_read = (r);     \
        (obj)->is_want_write = (w);    \
    }

#define _clear_read_write_flag(obj)        \
    {                                      \
        (obj)->is_want_read = 0;           \
        (obj)->is_want_write = 0;          \
        (obj)->is_ssl_error_or_closed = 0; \
    }

#define _aio_active(engine_)                           \
    {                                                  \
        _active_vector[engine_->action_type](engine_); \
    }

#pragma pack(push, 4)

enum active_stage_type_t
{
    active_stage_none = 0,
    active_stage_incoming,
    active_stage_timeout,
    active_stage_epoll,
    active_stage_unknown,
};

enum action_type_t
{
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

enum monitor_type_t
{
    monitor_none = 0,
    monitor_clear,
    monitor_active,
    monitor_max,
};

typedef void (*aio_cb_t)(aio_engine *engine_);

struct aio_rwbuf_t
{
    aio_rwbuf_t *next;
    unsigned int long_flag : 1;
    int p1 : 15;
    int p2 : 15;
    char data[aio_rwbuf_size];
};

struct aio_rwbuf_longbuf_t
{
    int p1;
    int p2;
    char *data;
};
struct aio_rwbuf_list_t
{
    int len;
    aio_rwbuf_t *head;
    aio_rwbuf_t *tail;
};

struct aio_engine
{
    const char *magic{"magic"};
    unsigned char action_type : 5;
    unsigned char active_stage : 3;
    unsigned char in_base_context : 1;
    unsigned char in_timeout_tree : 1;
    unsigned char in_incoming_queue : 1;
    unsigned char in_extern_incoming_queue : 1;
    unsigned char in_tmp_queue : 1;
    unsigned char is_want_read : 1;
    unsigned char is_want_write : 1;
    unsigned char is_io_ok_once : 1;
    unsigned char is_delimiter_checked : 1;
    unsigned char is_ssl_error_or_closed : 1;
    unsigned char is_cint_want_data : 1;
    unsigned char is_once_timer : 1;
    unsigned char old_input_events : 3;
    char delimiter;
    int ret{0};
    int fd{0};
    int wait_timeout{0};
    int want_read_len{0}; /* max_len, strict_len, sleep_timeout */
    aio_rwbuf_list_t read_cache;
    aio_rwbuf_list_t write_cache;
    SSL *ssl{0};
    int64_t cutoff_time{0};
    rbtree_node_t rbnode_time;
    aio *aio_{nullptr};
    std::function<void()> callback{nullptr};
    aio_base_engine *aiobase_engine{nullptr};
};

struct aio_base_engine
{
    bool stop_flag{false};
    bool running_flag{false};
    int epoll_fd;
    int epoll_event_size;
    struct epoll_event *epoll_event_vec{nullptr};
    rbtree_t timeout_tree;
    aio_engine *eventfd_aio{nullptr};
    std::function<void()> loop_fn{nullptr};
    rbtree_node_t *incoming_queue_head{nullptr};
    rbtree_node_t *incoming_queue_tail{nullptr};
    rbtree_node_t *extern_incoming_queue_head{nullptr};
    rbtree_node_t *extern_incoming_queue_tail{nullptr};
    rbtree_node_t *tmp_queue_head{nullptr};
    rbtree_node_t *tmp_queue_tail{nullptr};
    pthread_mutex_t extern_plocker;
    aio_base *aio_base_;
};

class aio_friend
{
public:
    static inline aio_engine *get_aio_engine(aio *a)
    {
        return a->engine_;
    }
    static inline aio_base_engine *get_aio_base_engine(aio_base *a)
    {
        return a->engine_;
    }
};

#pragma pack(pop)

aio_base *var_main_aio_base = nullptr;

static __thread aio_base_engine *_current_base = 0;
static aio_cb_t _active_vector[action_unknown + 1];

static void ___aio_cache_shift(aio_engine *engine_, aio_rwbuf_list_t *ioc, const void *data, int len)
{
    int rlen, i, olen = len;
    char *buf = (char *)(void *)data;
    char *cdata;
    aio_rwbuf_t *rwb;

    rwb = ioc->head;
    if (rwb && rwb->long_flag)
    {
        /* only write */
        aio_rwbuf_longbuf_t *lb = (aio_rwbuf_longbuf_t *)(rwb->data);
        lb->p1 += len;
        if (lb->p1 == lb->p2)
        {
            ioc->head = rwb->next;
            free(rwb);
            if (ioc->head == 0)
            {
                ioc->tail = 0;
            }
        }
        ioc->len -= olen;
        return;
    }

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
            free(ioc->head);
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

static void ___aio_cache_first_line(aio_engine *engine_, aio_rwbuf_list_t *ioc, char **data, int *len)
{
    aio_rwbuf_t *rwb;

    rwb = ioc->head;
    if (!rwb)
    {
        *data = 0;
        *len = 0;
        return;
    }
    if (rwb->long_flag)
    {
        aio_rwbuf_longbuf_t *lb = (aio_rwbuf_longbuf_t *)(rwb->data);
        *data = lb->data + lb->p1;
        *len = lb->p2 - lb->p1 + 1;
    }
    else
    {
        *data = (rwb->data + rwb->p1);
        *len = (rwb->p2 - rwb->p1 + 1);
    }
}

static void ___aio_cache_append(aio_engine *engine_, aio_rwbuf_list_t *ioc, const void *data, int len)
{
    if (len < 1)
    {
        return;
    }
    char *buf = (char *)(void *)data;
    char *cdata;
    int i, p2;
    aio_rwbuf_t *rwb = ioc->tail;
    p2 = 0;
    cdata = 0;
    if (rwb)
    {
        p2 = rwb->p2;
        cdata = rwb->data;
    }
    for (i = 0; i < len; i++)
    {
        if (!rwb || (p2 == aio_rwbuf_size - 1))
        {
            rwb = (aio_rwbuf_t *)malloc(sizeof(aio_rwbuf_t));
            rwb->long_flag = 0;
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

static void _aio_event_monitor(aio_engine *engine_, int monitor_cmd)
{
    aio_base_engine *eb = engine_->aiobase_engine;
    unsigned char action_type = engine_->action_type;

    if (monitor_cmd == monitor_none)
    {
        return;
    }

    /* new event */
    unsigned char new_input_events = 0;
    if (monitor_cmd == monitor_clear)
    {
        new_input_events = 0;
    }
    else if (monitor_cmd == monitor_active)
    {
        if (engine_->is_want_read)
        {
            new_input_events |= event_read;
        }
        if (engine_->is_want_write)
        {
            new_input_events |= event_write;
        }
    }

    /* timer */
    if (monitor_cmd == monitor_active)
    {
        if (action_type == action_sleep)
        {
            engine_->cutoff_time = millisecond() + 1000L * (engine_->want_read_len);
            rbtree_attach(&(eb->timeout_tree), &(engine_->rbnode_time));
            engine_->in_timeout_tree = 1;
        }
        else
        {
            int max_timeout = -1, max_read_timeout = -2, max_write_timeout = -2;
            if (new_input_events & event_read)
            {
                max_read_timeout = ((engine_->wait_timeout > -1) ? engine_->wait_timeout : -1);
            }
            if (new_input_events & event_write)
            {
                max_write_timeout = ((engine_->wait_timeout > -1) ? engine_->wait_timeout : -1);
            }
            if ((max_read_timeout == -1) || (max_write_timeout == -1))
            {
                max_timeout = -1;
            }
            else if ((max_read_timeout == -2))
            {
                max_timeout = max_write_timeout;
            }
            else if (max_write_timeout == -2)
            {
                max_timeout = max_read_timeout;
            }
            else
            {
                max_timeout = (max_read_timeout < max_write_timeout ? max_write_timeout : max_read_timeout);
            }
            if (max_timeout > -1)
            {
                engine_->cutoff_time = millisecond() + 1000L * max_timeout;
                rbtree_attach(&(eb->timeout_tree), &(engine_->rbnode_time));
                engine_->in_timeout_tree = 1;
            }
        }
    }

    /* event */
    unsigned char old_input_events = engine_->old_input_events;
    struct epoll_event epev;
    epev.events = EPOLLHUP | EPOLLERR;
    if (new_input_events == 0)
    {
        if (old_input_events)
        {
            if (epoll_ctl(eb->epoll_fd, EPOLL_CTL_DEL, engine_->fd, NULL) == -1)
            {
                zcc_fatal("epoll_ctl del fd=%d(%m)", engine_->fd);
            }
        }
    }
    else if (new_input_events != old_input_events)
    {
        if (new_input_events & event_read)
        {
            epev.events |= EPOLLIN;
        }
        if (new_input_events & event_write)
        {
            epev.events |= EPOLLOUT;
        }
        epev.data.ptr = engine_;
        if (epoll_ctl(eb->epoll_fd, (old_input_events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD), engine_->fd, &epev) == -1)
        {
            zcc_fatal("epoll_ctl %s fd=%d(%m)", (old_input_events ? "mod" : "add"), engine_->fd);
        }
    }
    engine_->old_input_events = new_input_events;
}

static inline aio_engine *get_aio_engine_by_rbnode_time(rbtree_node_t *n)
{
    return aio_friend::get_aio_engine(*(aio **)(((char *)(n)) + sizeof(rbtree_node_t)));
}

static void _aio_enter_incoming(aio_engine *engine_)
{
    aio_base_engine *eb = engine_->aiobase_engine;
    rbtree_node_t *rn = &(engine_->rbnode_time);
    int is_self = _function_pthread_is_self(eb);
    if (!eb->running_flag)
    {
        is_self = 1;
    }

    if ((!is_self) && (engine_->in_base_context))
    {
        zcc_fatal("cannot operate the same aio_engine instance in another thread at the same time");
    }

    if (!engine_->callback)
    {
        zcc_fatal("callback is null");
    }

    engine_->in_base_context = 1;
    _clear_read_write_flag(engine_);

    if (is_self)
    {
        if (engine_->in_incoming_queue == 0)
        {
            engine_->in_incoming_queue = 1;
            ZCC_MLINK_APPEND(eb->incoming_queue_head, eb->incoming_queue_tail, rn, rbtree_left, rbtree_right);
        }
        else
        {
        }
    }
    else
    {
        if (engine_->in_extern_incoming_queue == 0)
        {
            _base_lock_extern(eb);
            engine_->in_extern_incoming_queue = 1;
            ZCC_MLINK_APPEND(eb->extern_incoming_queue_head, eb->extern_incoming_queue_tail, rn, rbtree_left, rbtree_right);
            _base_unlock_extern(eb);
            eb->aio_base_->touch();
        }
    }
}

/* -1: 错, 0: 重试, >0: 正确,长度 */
static int _aio_io_read(aio_engine *engine_, void *buf, int len)
{
    int ret;

    _clear_read_write_flag(engine_);

    if (engine_->ssl)
    {
        if ((ret = SSL_read(engine_->ssl, buf, len)) > 0)
        {
            engine_->is_io_ok_once = 1;
            return ret;
        }
        int status = SSL_get_error(engine_->ssl, ret);
        if (status == SSL_ERROR_WANT_WRITE)
        {
            engine_->is_want_write = 1;
        }
        else if (status == SSL_ERROR_WANT_READ)
        {
            engine_->is_want_read = 1;
        }
        else
        {
            engine_->is_ssl_error_or_closed = 1;
        }
        if (engine_->is_ssl_error_or_closed)
        {
            return -1;
        }
        return 0;
    }
    else
    {
        while (1)
        {
            if ((ret = read(engine_->fd, buf, len)) > 0)
            {
                engine_->is_io_ok_once = 1;
                return ret;
            }
            else if (ret == 0)
            {
                return -1;
            }
            int ec = errno;
            if (ec == EINTR)
            {
                continue;
            }
            engine_->is_want_read = 1;
            if (ec == EAGAIN)
            {
                return 0;
            }
            return -1;
        }
    }
}

int _aio_io_write(aio_engine *engine_, const void *buf, int len)
{
    int ret;

    _clear_read_write_flag(engine_);

    if (engine_->ssl)
    {
        engine_->is_ssl_error_or_closed = 0;

        if ((ret = SSL_write(engine_->ssl, buf, len)) > 0)
        {
            engine_->is_io_ok_once = 1;
            return ret;
        }
        int status = SSL_get_error(engine_->ssl, ret);
        if (status == SSL_ERROR_WANT_WRITE)
        {
            engine_->is_want_write = 1;
        }
        else if (status == SSL_ERROR_WANT_READ)
        {
            engine_->is_want_read = 1;
        }
        else
        {
            engine_->is_ssl_error_or_closed = 1;
        }
        if (engine_->is_ssl_error_or_closed)
        {
            return -1;
        }
        return 0;
    }
    else
    {
        while (1)
        {
            if ((ret = write(engine_->fd, buf, len)) >= 0)
            {
                if (ret > 0)
                {
                    engine_->is_io_ok_once = 1;
                }
                return ret;
            }
            int ec = errno;
            if (ec == EINTR)
            {
                continue;
            }
            if (ec == EPIPE)
            {
                return -1;
            }
            engine_->is_want_write = 1;
            if (ec == EAGAIN)
            {
                return 0;
            }
            return -1;
        }
    }
}

static void _active_tls_connect(aio_engine *engine_)
{
    if (engine_->active_stage == active_stage_timeout)
    {
        _ready_go_and_return(engine_, -1);
    }

    _clear_read_write_flag(engine_);

    int ret = SSL_connect(engine_->ssl);
    if (ret < 1)
    {
        int status = SSL_get_error(engine_->ssl, ret);
        if (status == SSL_ERROR_WANT_WRITE)
        {
            engine_->is_want_write = 1;
        }
        else if (status == SSL_ERROR_WANT_READ)
        {
            engine_->is_want_read = 1;
        }
        else
        {
            engine_->is_ssl_error_or_closed = 1;
        }
    }

    if (ret > 0)
    {
        _ready_go_and_return(engine_, 1);
    }
    else if (engine_->is_ssl_error_or_closed)
    {
        _ready_go_and_return(engine_, -1);
    }
    else if (engine_->is_want_read || engine_->is_want_write)
    {
        engine_->ret = 0;
        _aio_event_monitor(engine_, monitor_active);
        return;
    }
    else
    {
    }
}

static void _active_tls_accept(aio_engine *engine_)
{
    if (engine_->active_stage == active_stage_timeout)
    {
        _ready_go_and_return(engine_, -1);
    }

    if (engine_->read_cache.len > 0)
    {
        // http://www.postfix.org/CVE-2011-0411.html
        _ready_go_and_return(engine_, -1);
    }

    _clear_read_write_flag(engine_);

    int ret = SSL_accept(engine_->ssl);
    if (ret < 1)
    {
        int status = SSL_get_error(engine_->ssl, ret);
        if (status == SSL_ERROR_WANT_WRITE)
        {
            engine_->is_want_write = 1;
        }
        else if (status == SSL_ERROR_WANT_READ)
        {
            engine_->is_want_read = 1;
        }
        else
        {
            engine_->is_ssl_error_or_closed = 1;
        }
    }

    if (ret > 0)
    {
        _ready_go_and_return(engine_, 1);
    }
    else if (engine_->is_ssl_error_or_closed)
    {
        _ready_go_and_return(engine_, -1);
    }
    else if (engine_->is_want_read || engine_->is_want_write)
    {
        engine_->ret = 0;
        _aio_event_monitor(engine_, monitor_active);
        return;
    }
    else
    {
    }
}

static void _active_none(aio_engine *engine_)
{
}

static void _active_readable(aio_engine *engine_)
{
    if (engine_->read_cache.len)
    {
        _ready_go_and_return(engine_, 1);
    }

    if (engine_->active_stage == active_stage_timeout)
    {
        _ready_go_and_return(engine_, -2);
    }
    else if (engine_->active_stage == active_stage_epoll)
    {
        _ready_go_and_return(engine_, 1);
    }

    _want_read_or_write(engine_, 1, 0);
    _aio_event_monitor(engine_, monitor_active);
}

static void _active_read(aio_engine *engine_)
{
    if (engine_->active_stage == active_stage_timeout)
    {
        if (engine_->is_io_ok_once)
        {
            _ready_go_and_return(engine_, -1);
        }
        else
        {
            _ready_go_and_return(engine_, -2);
        }
    }

    int ret;
    char buf[aio_rwbuf_size + 1];
    if (engine_->want_read_len < 1)
    {
        _ready_go_and_return(engine_, 0);
    }

    if (engine_->read_cache.len == 0)
    {
        if ((ret = _aio_io_read(engine_, buf, aio_rwbuf_size)) > 0)
        {
            ___aio_cache_append(engine_, &(engine_->read_cache), buf, ret);
        }
        else if (ret == 0)
        {
            engine_->ret = 0;
            _aio_event_monitor(engine_, monitor_active);
            return;
        }
        else
        {
            _ready_go_and_return(engine_, -1);
        }
    }

    if (engine_->read_cache.len > engine_->want_read_len)
    {
        ret = engine_->want_read_len;
    }
    else
    {
        ret = engine_->read_cache.len;
    }
    _ready_go_and_return(engine_, ret);
}

static void _active_readn(aio_engine *engine_)
{
    if (engine_->active_stage == active_stage_timeout)
    {
        if (engine_->is_io_ok_once)
        {
            _ready_go_and_return(engine_, -1);
        }
        else
        {
            _ready_go_and_return(engine_, -2);
        }
    }

    int strict_len = engine_->want_read_len;
    char buf[aio_rwbuf_size + 1];

    if (strict_len < 1)
    {
        _ready_go_and_return(engine_, 0);
    }
    while (1)
    {
        if (strict_len <= engine_->read_cache.len)
        {
            break;
        }
        int ret = _aio_io_read(engine_, buf, aio_rwbuf_size);
        if (ret == 0)
        {
            engine_->ret = 0;
            _aio_event_monitor(engine_, monitor_active);
            return;
        }
        else if (ret < 0)
        {
            _ready_go_and_return(engine_, -1);
        }
        ___aio_cache_append(engine_, &(engine_->read_cache), buf, ret);
    }
    _ready_go_and_return(engine_, strict_len);
}

static void _active_read_delimiter(aio_engine *engine_)
{
    if (engine_->active_stage == active_stage_timeout)
    {
        if (engine_->is_io_ok_once)
        {
            _ready_go_and_return(engine_, -1);
        }
        else
        {
            _ready_go_and_return(engine_, -2);
        }
    }

    int max_len = engine_->want_read_len, rlen = -1, ret;
    char delimiter = engine_->delimiter;
    char buf[aio_rwbuf_size + 1], *data;

    if (max_len < 1)
    {
        _ready_go_and_return(engine_, 0);
    }

    if (engine_->is_delimiter_checked == 0)
    {
        engine_->is_delimiter_checked = 1;
        int found = 0;
        rlen = 0;
        for (aio_rwbuf_t *rwb = engine_->read_cache.head; rwb; rwb = rwb->next)
        {
            data = rwb->data;
            int end = rwb->p2 + 1;
            for (int i = rwb->p1; i != end; i++)
            {
                rlen++;
                if ((data[i] == delimiter) || (rlen == max_len))
                {
                    found = 1;
                    break;
                }
            }
            if (found)
            {
                break;
            }
        }
        if (found)
        {
            _ready_go_and_return(engine_, rlen);
        }
    }

    while (1)
    {
        if ((ret = _aio_io_read(engine_, buf, aio_rwbuf_size)) < 0)
        {
            _ready_go_and_return(engine_, -1);
        }
        else if (ret == 0)
        {
            engine_->ret = 0;
            _aio_event_monitor(engine_, monitor_active);
            return;
        }
        ___aio_cache_append(engine_, &(engine_->read_cache), buf, ret);
        rlen = -1;
        if ((data = (char *)memchr(buf, delimiter, ret)))
        {
            rlen = engine_->read_cache.len - ret + (data - buf + 1);
            if (rlen > max_len)
            {
                rlen = max_len;
            }
        }
        else
        {
            if (max_len <= engine_->read_cache.len)
            {
                rlen = max_len;
            }
        }
        if (rlen != -1)
        {
            _ready_go_and_return(engine_, rlen);
        }
    }
}

static void _active_read_cint(aio_engine *engine_)
{
    if (engine_->is_cint_want_data && (engine_->want_read_len > -1))
    {
        _active_readn(engine_);
        return;
    }

    if (engine_->active_stage == active_stage_timeout)
    {
        if (engine_->is_io_ok_once)
        {
            _ready_go_and_return(engine_, -1);
        }
        else
        {
            _ready_go_and_return(engine_, -2);
        }
    }

    char buf[aio_rwbuf_size + 10];

    while (1)
    {
        int byte_count = 0, shift = 0, size = 0;
        for (aio_rwbuf_t *rwb = engine_->read_cache.head; rwb; rwb = rwb->next)
        {
            unsigned char *buf = (unsigned char *)(rwb->data);
            int end = rwb->p2 + 1;
            for (int i = rwb->p1; i != end; i++)
            {
                byte_count++;
                if (byte_count > 4)
                {
                    _ready_go_and_return(engine_, -1);
                }
                int ch = buf[i];
                size |= ((ch & 0177) << shift);
                if (ch & 0200)
                {
                    ___aio_cache_shift(engine_, &(engine_->read_cache), 0, byte_count);
                    if ((engine_->is_cint_want_data == 0) || (size == 0))
                    {
                        _ready_go_and_return(engine_, size);
                    }
                    else
                    {
                        engine_->want_read_len = size;
                        _aio_enter_incoming(engine_);
                        return;
                    }
                }
                shift += 7;
            }
        }
        int ret = _aio_io_read(engine_, buf, aio_rwbuf_size);
        if (ret < 0)
        {
            _ready_go_and_return(engine_, -1);
        }
        else if (ret == 0)
        {
            engine_->ret = 0;
            _aio_event_monitor(engine_, monitor_active);
            return;
        }
        ___aio_cache_append(engine_, &(engine_->read_cache), buf, ret);
    }
}

static void _active_writeable(aio_engine *engine_)
{
    if (engine_->active_stage == active_stage_timeout)
    {
        if (engine_->is_io_ok_once)
        {
            _ready_go_and_return(engine_, -1);
        }
        else
        {
            _ready_go_and_return(engine_, -2);
        }
    }
    else if (engine_->active_stage == active_stage_epoll)
    {
        _ready_go_and_return(engine_, 1);
    }
    _want_read_or_write(engine_, 0, 1);
    _aio_event_monitor(engine_, monitor_active);
}

static void _active_writen(aio_engine *engine_)
{
    if (engine_->active_stage == active_stage_timeout)
    {
        if (engine_->is_io_ok_once)
        {
            _ready_go_and_return(engine_, -1);
        }
        else
        {
            _ready_go_and_return(engine_, -2);
        }
        return;
    }

    int ret;
    char *data;
    while (1)
    {
        ___aio_cache_first_line(engine_, &(engine_->write_cache), &data, &ret);
        if (ret == 0)
        {
            _ready_go_and_return(engine_, 1);
        }
        if ((ret = _aio_io_write(engine_, data, ret)) < 0)
        {
            _ready_go_and_return(engine_, -1);
        }
        else if (ret == 0)
        {
            engine_->ret = 0;
            _aio_event_monitor(engine_, monitor_active);
            return;
        }
        ___aio_cache_shift(engine_, &(engine_->write_cache), 0, ret);
    }
}

static void _active_sleep(aio_engine *engine_)
{
    if (engine_->active_stage == active_stage_timeout)
    {
        _ready_go_and_return(engine_, 1);
    }
    else if (engine_->active_stage == active_stage_incoming)
    {
        if (engine_->want_read_len < 1)
        {
            _ready_go_and_return(engine_, 1);
        }
    }
    _aio_event_monitor(engine_, monitor_active);
}

static aio_engine *aio_engine_create(int fd, SSL *ssl, aio_base *aiobase, aio *a)
{
    if (!aiobase)
    {
        aiobase = var_main_aio_base;
    }
    aio_engine *engine_ = new aio_engine();
    engine_->magic = "magic";
    engine_->aio_ = a;
    engine_->aiobase_engine = aio_friend::get_aio_base_engine(aiobase);

    engine_->action_type = 0;
    engine_->active_stage = 0;
    engine_->in_base_context = 0;
    engine_->in_timeout_tree = 0;
    engine_->in_incoming_queue = 0;
    engine_->in_extern_incoming_queue = 0;
    engine_->in_tmp_queue = 0;
    engine_->is_want_read = 0;
    engine_->is_want_write = 0;
    engine_->is_io_ok_once = 0;
    engine_->is_delimiter_checked = 0;
    engine_->is_ssl_error_or_closed = 0;
    engine_->is_cint_want_data = 0;
    engine_->is_once_timer = 0;
    engine_->old_input_events = 0;

    engine_->ssl = ssl;
    if (ssl)
    {
        fd = openssl::SSL_get_fd(ssl);
    }
    engine_->fd = fd;
    engine_->ret = 1;
    engine_->wait_timeout = -1;
    engine_->wait_timeout = -1;

    std::memset(&(engine_->read_cache), 0, sizeof(aio_rwbuf_list_t));
    std::memset(&(engine_->write_cache), 0, sizeof(aio_rwbuf_list_t));
    return engine_;
}

aio::aio()
{
    engine_ = aio_engine_create(-1, 0, 0, this);
}

aio::aio(int fd, aio_base *aiobase)
{
    engine_ = aio_engine_create(fd, 0, aiobase, this);
}

aio::aio(SSL *ssl, aio_base *aiobase)
{
    engine_ = aio_engine_create(0, ssl, aiobase, this);
}

aio::~aio()
{
    close(true);
}

void aio::rebind_fd(int fd, aio_base *aiobase)
{
    close(true);
    engine_ = aio_engine_create(fd, 0, aiobase, this);
}

void aio::rebind_SLL(SSL *ssl, aio_base *aiobase)
{
    close(true);
    engine_ = aio_engine_create(0, ssl, aiobase, this);
}

void aio::close(bool close_fd_and_release_ssl)
{
    if (!engine_)
    {
        return;
    }
    aio_base_engine *eb = engine_->aiobase_engine;
    int is_self = _function_pthread_is_self(eb);

    if ((!is_self) && (engine_->in_base_context))
    {
        zcc_fatal("cannot operate the same aio_engine instance in another thread at the same time");
    }

    if (is_self)
    {
        disable();
    }

    if (engine_->read_cache.len > 0)
    {
        ___aio_cache_shift(engine_, &(engine_->read_cache), 0, engine_->read_cache.len);
    }
    if (engine_->write_cache.len > 0)
    {
        ___aio_cache_shift(engine_, &(engine_->write_cache), 0, engine_->write_cache.len);
    }
    if (close_fd_and_release_ssl)
    {
        if (engine_->ssl)
        {
            openssl::SSL_free(engine_->ssl);
            engine_->ssl = 0;
        }
        if (engine_->fd > -1)
        {
            close_socket(engine_->fd);
        }
    }
    delete engine_;
    engine_ = nullptr;
}

void aio::rebind_aio_base(aio_base *aiobase)
{
    aio_base_engine *eb = engine_->aiobase_engine;
    int is_self = _function_pthread_is_self(eb);

    if ((!is_self) && (engine_->in_base_context))
    {
        zcc_fatal("cannot operate the same aio_engine instance in another thread at the same time");
    }
    engine_->aiobase_engine = eb;
}

void aio::set_timeout(int wait_timeout)
{
    engine_->wait_timeout = wait_timeout;
}

void aio::disable()
{
    int is_self = _function_pthread_is_self(engine_->aiobase_engine);
    if (!is_self)
    {
        zcc_fatal("aio::disable can only be executed in the pthread it belongs to");
    }

    aio_base_engine *eb = engine_->aiobase_engine;
    rbtree_node_t *rn = &(engine_->rbnode_time);
    if (engine_->in_timeout_tree)
    {
        rbtree_detach(&(eb->timeout_tree), rn);
        engine_->in_timeout_tree = 0;
    }
    if (engine_->in_tmp_queue)
    {
        ZCC_MLINK_DETACH(eb->tmp_queue_head, eb->tmp_queue_tail, rn, rbtree_left, rbtree_right);
        engine_->in_tmp_queue = 0;
    }
    if (engine_->in_incoming_queue)
    {
        ZCC_MLINK_DETACH(eb->incoming_queue_head, eb->incoming_queue_tail, rn, rbtree_left, rbtree_right);
        engine_->in_incoming_queue = 0;
    }
    if (engine_->in_extern_incoming_queue)
    {
        _base_lock_extern(eb);
        ZCC_MLINK_DETACH(eb->extern_incoming_queue_head, eb->extern_incoming_queue_tail, rn, rbtree_left, rbtree_right);
        _base_unlock_extern(eb);
        engine_->in_extern_incoming_queue = 0;
    }

    engine_->action_type = action_none;
    engine_->in_base_context = 0;

    _clear_read_write_flag(engine_);
    _aio_event_monitor(engine_, monitor_clear);
}

int aio::get_result()
{
    return engine_->ret;
}

int aio::get_fd()
{
    return engine_->fd;
}

int aio::get_read_cache_size()
{
    return engine_->read_cache.len;
}

static inline void aio_get_read_cache_to_buf(aio_engine *engine_, char *buf, int strict_len)
{
    ___aio_cache_shift(engine_, &(engine_->read_cache), buf, strict_len);
}

void aio::get_read_cache(char *buf, int strict_len)
{
    aio_get_read_cache_to_buf(engine_, buf, strict_len);
}

void aio::get_read_cache(std::string &bf, int strict_len)
{

    char buf[1024 + 1];
    while (strict_len > 0)
    {
        int len = (strict_len > 1024 ? 1024 : strict_len);
        aio_get_read_cache_to_buf(engine_, buf, len);
        bf.append(buf, len);
        strict_len -= len;
    }
}

int aio::get_write_cache_size()
{
    return engine_->write_cache.len;
}

static inline void aio_get_write_cache_to_buf(aio_engine *engine_, char *buf, int strict_len)
{
    ___aio_cache_shift(engine_, &(engine_->write_cache), buf, strict_len);
}

void aio::get_write_cache(char *buf, int strict_len)
{
    aio_get_write_cache_to_buf(engine_, buf, strict_len);
}

void aio::get_write_cache(std::string &bf, int strict_len)
{
    char buf[1024 + 1];
    while (strict_len > 0)
    {
        int len = (strict_len > 1024 ? 1024 : strict_len);
        aio_get_write_cache_to_buf(engine_, buf, len);
        bf.append(buf, len);
        strict_len -= len;
    }
}

void aio::readable(std::function<void()> callback)
{
    engine_->action_type = action_readable;
    engine_->callback = callback;
    _aio_enter_incoming(engine_);
}

void aio::writeable(std::function<void()> callback)
{
    engine_->action_type = action_writeable;
    engine_->callback = callback;
    _aio_enter_incoming(engine_);
}

void aio::read(int max_len, std::function<void()> callback)
{
    engine_->action_type = action_read;
    engine_->want_read_len = max_len;
    engine_->callback = callback;
    _aio_enter_incoming(engine_);
}

void aio::readn(int strict_len, std::function<void()> callback)
{
    engine_->action_type = action_readn;
    engine_->want_read_len = strict_len;
    engine_->callback = callback;
    _aio_enter_incoming(engine_);
}

void aio::read_delimiter(int delimiter, int max_len, std::function<void()> callback)
{
    engine_->action_type = action_read_delimiter;
    engine_->delimiter = delimiter;
    engine_->is_delimiter_checked = 0;
    engine_->want_read_len = max_len;
    engine_->callback = callback;
    _aio_enter_incoming(engine_);
}

void aio::get_cint(std::function<void()> callback)
{
    engine_->is_cint_want_data = 0;
    engine_->want_read_len = -2020;
    engine_->action_type = action_read_cint;
    engine_->callback = callback;
    _aio_enter_incoming(engine_);
}

void aio::get_cint_and_data(std::function<void()> callback)
{
    engine_->is_cint_want_data = 1;
    engine_->want_read_len = -2020;
    engine_->action_type = action_read_cint;
    engine_->callback = callback;
    _aio_enter_incoming(engine_);
}

int aio::cache_write(const void *buf, int len)
{
    if (len < 0)
    {
        len = std::strlen((const char *)buf);
    }
    ___aio_cache_append(engine_, &(engine_->write_cache), buf, len);
    return len;
}

void aio::cache_printf_1024(const char *fmt, ...)
{
    va_list ap;
    char buf[1024 + 1];
    int len;

    va_start(ap, fmt);
    len = std::vsnprintf(buf, 1024, fmt, ap);
    len = ((len < 1024) ? len : (1024 - 1));
    va_end(ap);
    cache_write(buf, len);
}

void aio::cache_write_direct(const void *buf, int len)
{
    if (len < 0)
    {
        len = std::strlen((const char *)buf);
    }

    aio_rwbuf_t *rwb = (aio_rwbuf_t *)calloc(1, sizeof(aio_rwbuf_t));
    rwb->next = 0;
    rwb->long_flag = 1;
    rwb->p1 = 0;
    rwb->p2 = 0;

    aio_rwbuf_longbuf_t *lb = (aio_rwbuf_longbuf_t *)(rwb->data);
    lb->data = (char *)buf;
    lb->p1 = 0;
    lb->p2 = len - 1;

    if (engine_->write_cache.head)
    {
        engine_->write_cache.head->next = rwb;
        engine_->write_cache.tail = rwb;
    }
    else
    {
        engine_->write_cache.head = rwb;
        engine_->write_cache.tail = rwb;
    }
}

void aio::cache_write_cint(int len)
{
    char sbuf[32];
    int ret = cint_put(len, sbuf);
    ___aio_cache_append(engine_, &(engine_->write_cache), sbuf, ret);
}

void aio::cache_write_cint_and_data(const void *data, int len)
{
    if (len < 0)
    {
        len = strlen((char *)(void *)data);
    }
    char sbuf[32];
    int ret = cint_put(len, sbuf);
    ___aio_cache_append(engine_, &(engine_->write_cache), sbuf, ret);
    ___aio_cache_append(engine_, &(engine_->write_cache), data, len);
}

void aio::cache_flush(std::function<void()> callback)
{
    engine_->action_type = action_writen;
    engine_->callback = callback;
    _aio_enter_incoming(engine_);
}

void aio::sleep(std::function<void()> callback, int timeout)
{
    engine_->action_type = action_sleep;
    engine_->want_read_len = timeout;
    engine_->callback = callback;
    _aio_enter_incoming(engine_);
}

void aio::tls_connect(SSL_CTX *ctx, std::function<void()> callback)
{
    engine_->ssl = openssl::SSL_create(ctx, engine_->fd);
    engine_->action_type = action_tls_connect;
    engine_->callback = callback;
    _aio_enter_incoming(engine_);
}

void aio::tls_accept(SSL_CTX *ctx, std::function<void()> callback)
{
    engine_->ssl = openssl::SSL_create(ctx, engine_->fd);
    engine_->action_type = action_tls_accept;
    engine_->callback = callback;
    _aio_enter_incoming(engine_);
}

SSL *aio::get_ssl()
{
    return engine_->ssl;
}

aio_base *aio::get_aio_base()
{
    return engine_->aiobase_engine->aio_base_;
}

// TIMER
aio_timer::aio_timer(aio_base *aiobase) : aio(-1, aiobase)
{
}

aio_timer::~aio_timer()
{
}

void aio_timer::rebind_aio_base(aio_base *aiobase)
{
    aio::rebind_fd(-1, aiobase);
}

void aio_timer::after(std::function<void()> callback, int timeout)
{
    sleep(callback, timeout);
}

// BASE

aio_base *aio_base::get_current_thread_aio_base()
{

    return _current_base ? _current_base->aio_base_ : nullptr;
}

static void _base_event_notify_reader(aio *aio)
{
    aio_engine *engine_ = aio_friend::get_aio_engine(aio);
    uint64_t u;
    if (::read(engine_->fd, &u, sizeof(uint64_t)))
        ;
}

static int _base_timeout_cmp(rbtree_node_t *n1, rbtree_node_t *n2)
{
    aio_engine *t1 = get_aio_engine_by_rbnode_time(n1);
    aio_engine *t2 = get_aio_engine_by_rbnode_time(n2);

    int64_t r = t1->cutoff_time - t2->cutoff_time;
    if (!r)
    {
        r = (char *)t1 - (char *)t2;
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

static void _base_check_incoming(aio_base_engine *eb)
{
    eb->tmp_queue_head = eb->incoming_queue_head;
    eb->tmp_queue_tail = eb->incoming_queue_tail;
    eb->incoming_queue_head = 0;
    eb->incoming_queue_tail = 0;

    if (eb->extern_incoming_queue_head)
    {
        _base_lock_extern(eb);
#if 1
        ZCC_MLINK_CONCAT(eb->tmp_queue_head, eb->tmp_queue_tail, eb->extern_incoming_queue_head, eb->extern_incoming_queue_tail, rbtree_left, rbtree_right);
#else
        while (eb->extern_incoming_queue_head)
        {
            rbtree_node_t *n = eb->extern_incoming_queue_head;
            ZCC_MLINK_DETACH(eb->extern_incoming_queue_head, eb->extern_incoming_queue_tail, n, rbtree_left, rbtree_right);
            ZCC_MLINK_APPEND(eb->tmp_queue_head, eb->tmp_queue_tail, n, rbtree_left, rbtree_right);
        }
#endif
        eb->extern_incoming_queue_head = 0;
        eb->extern_incoming_queue_tail = 0;
        _base_unlock_extern(eb);
    }

    for (rbtree_node_t *n = eb->tmp_queue_head; n; n = n->rbtree_right)
    {
        aio_engine *engine_ = get_aio_engine_by_rbnode_time(n);
        engine_->in_incoming_queue = 0;
        engine_->in_extern_incoming_queue = 0;
        engine_->in_tmp_queue = 1;
    }

    while (eb->tmp_queue_head)
    {
        rbtree_node_t *n = eb->tmp_queue_head;
        ZCC_MLINK_DETACH(eb->tmp_queue_head, eb->tmp_queue_tail, n, rbtree_left, rbtree_right);
        aio_engine *engine_ = get_aio_engine_by_rbnode_time(n);
        engine_->in_tmp_queue = 0;
        engine_->active_stage = active_stage_incoming;
        _aio_active(engine_);
    }
}

static int _base_check_timeout(aio_base_engine *eb)
{
    rbtree_t *timer_tree = &(eb->timeout_tree);
    if (!rbtree_have_data(timer_tree))
    {
        return 1000;
    }
    while (1)
    {
        if (eb->stop_flag)
        {
            return 1000;
        }
        rbtree_node_t *rn = rbtree_first(timer_tree);
        if (!rn)
        {
            return 1000;
        }
        aio_engine *engine_ = get_aio_engine_by_rbnode_time(rn);
        int64_t delay = millisecond_to(engine_->cutoff_time);
        if (delay > 0)
        {
            return (delay > 1000 ? 1000 : delay);
        }
        rbtree_detach(timer_tree, rn);
        engine_->in_timeout_tree = 0;
        engine_->active_stage = active_stage_timeout;
        _aio_active(engine_);
    }
    return 1000;
}

static void _base_run_before(aio_base *eb)
{
    auto engine_ = aio_friend::get_aio_base_engine(eb);
    _current_base = engine_;
    engine_->running_flag = true;
    int eventfd_fd = eventfd(0, 0);
    close_on_exec(eventfd_fd);
    nonblocking(eventfd_fd);
    engine_->eventfd_aio = aio_friend::get_aio_engine(new aio(eventfd_fd, eb));
    engine_->eventfd_aio->aio_->readable(std::bind(_base_event_notify_reader, engine_->eventfd_aio->aio_));
}

aio_base::aio_base()
{
#define ___active_f___(name) _active_vector[action_##name] = _active_##name;
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

    engine_ = new aio_base_engine();
    engine_->aio_base_ = this;
    engine_->stop_flag = false;
    engine_->running_flag = false;
    rbtree_init(&(engine_->timeout_tree), _base_timeout_cmp);

    engine_->epoll_fd = epoll_create(1024);
    close_on_exec(engine_->epoll_fd);

    engine_->epoll_event_size = 256;
    engine_->epoll_event_vec = (struct epoll_event *)calloc(engine_->epoll_event_size, sizeof(struct epoll_event));

    pthread_mutex_init(&(engine_->extern_plocker), 0);
}

aio_base::~aio_base()
{
    if (engine_->eventfd_aio)
    {
        delete engine_->eventfd_aio->aio_;
    }
    close_socket(engine_->epoll_fd);
    free(engine_->epoll_event_vec);
    pthread_mutex_destroy(&(engine_->extern_plocker));
    delete engine_;
}

void aio_base::set_loop_fn(std::function<void()> callback)
{
    engine_->loop_fn = callback;
}

void aio_base::run()
{
#define ___aio_base_run_while_begin \
    while (1)                       \
    {
#define ___aio_base_run_while_end }
#define ___aio_base_check_stop \
    if (engine_->stop_flag)    \
    {                          \
        break;                 \
    }

    _base_run_before(this);

    ___aio_base_run_while_begin;
    ___aio_base_check_stop;

    if (engine_->loop_fn)
    {
        engine_->loop_fn();
    }

    if (engine_->incoming_queue_head || engine_->extern_incoming_queue_head)
    {
        _base_check_incoming(engine_);
    }

    int delay = 1 * 1000;
    if (rbtree_have_data(&(engine_->timeout_tree)))
    {
        int delay_tmp = _base_check_timeout(engine_);
        if (delay_tmp < delay)
        {
            delay = delay_tmp;
        }
    }
    ___aio_base_check_stop;
    if ((delay < 0) || engine_->incoming_queue_head || engine_->extern_incoming_queue_head)
    {
        delay = 0;
    }

    int nfds = epoll_wait(engine_->epoll_fd, engine_->epoll_event_vec, engine_->epoll_event_size, delay);
    if (nfds < 0)
    {
        if (errno != EINTR)
        {
            zcc_fatal("epoll_wait(%m)");
        }
        continue;
    }
    for (int i = 0; i < nfds; i++)
    {
        ___aio_base_check_stop;
        aio_engine *a = (aio_engine *)(engine_->epoll_event_vec[i].data.ptr);
        a->active_stage = active_stage_epoll;
        if (a->in_timeout_tree)
        {
            rbtree_detach(&(engine_->timeout_tree), &(a->rbnode_time));
            a->in_timeout_tree = 0;
        }
        _aio_active(a);
    }
    ___aio_base_check_stop;
    if ((nfds == engine_->epoll_event_size) && (engine_->epoll_event_size < aio_rwbuf_size))
    {
        ___aio_base_check_stop;
        auto tmp = (struct epoll_event *)malloc(engine_->epoll_event_size * 2 * sizeof(struct epoll_event));
        if (engine_->epoll_event_vec)
        {
            std::memcpy(tmp, engine_->epoll_event_vec, engine_->epoll_event_size);
        }
        free(engine_->epoll_event_vec);
        engine_->epoll_event_vec = tmp;
        engine_->epoll_event_size *= 2;
    }
    ___aio_base_check_stop;
    ___aio_base_run_while_end;

#undef ___aio_base_run_while_begin
#undef ___aio_base_run_while_end
#undef ___aio_base_check_stop
    return;
}

void aio_base::stop_notify()
{
    engine_->stop_flag = true;
    touch();
}

void aio_base::touch()
{
    uint64_t u = 1;
    if (write(engine_->eventfd_aio->fd, &u, sizeof(uint64_t)))
        ;
}

static void aio_base_timer_do(aio_base *ab, aio *a, std::function<void()> callback)
{
    callback();
    delete a;
}

void aio_base::enter_timer(std::function<void()> callback, int timeout)
{
    if (!callback)
    {
        return;
    }
    aio *a = new aio(-1, this);
    a->sleep(std::bind(aio_base_timer_do, this, a, callback), timeout);
}

zcc_namespace_end;

#endif // __linux__

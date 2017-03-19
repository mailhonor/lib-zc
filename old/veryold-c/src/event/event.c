#include "zc.h"

#define ZAIO_CB_MAGIC                   0XF0U
#define ZAIO_CB_FN_NONE                 0X00U
#define ZAIO_CB_READ                    0X10U
#define ZAIO_CB_FN_READ                 0X11U
#define ZAIO_CB_FN_READ_N               0X12U
#define ZAIO_CB_FN_READ_DELIMETER       0X13U
#define ZAIO_CB_WRITE                   0X20U
#define ZAIO_CB_FN_WRITE                0X21U
#define ZAIO_CB_FN_SLEEP                0X31U
#define ZAIO_CB_FN_SSL_INIT             0X41U

#define ZEVENT_BASE_LOCK(eb)	(pthread_mutex_lock(&((eb)->locker)))
#define ZEVENT_BASE_UNLOCK(eb)	(pthread_mutex_unlock(&((eb)->locker)))

#define ___AIO_RWBUF_MALLOC(eb, rwb) { \
	rwb = (ZAIO_RWBUF *) zmpool_alloc_one(eb->aio_rwbuf_mpool); \
}
#define ___AIO_RWBUF_FREE(eb, rwb) { \
	zmpool_free_one((eb)->aio_rwbuf_mpool, (rwb)); \
}

static int ___zaio_read_n_TRUE(ZAIO * aio, int strict_len, ZAIO_CB_FN callback, void *context, int timeout);

static int zaio_timer_cmp(ZRBTREE_NODE * n1, ZRBTREE_NODE * n2);
static int zaio_timer_check(ZEVENT_BASE * eb);
static int zaio_action(ZAIO * aio);
static int zaio_event_set(ZAIO * aio, int events, int timeout);
static int ZAIO_P2_MAX = ZAIO_RWBUF_SIZE - 1;
static void zaio_ready_do(ZAIO * aio);

static int zevent_timer_cmp(ZRBTREE_NODE * n1, ZRBTREE_NODE * n2);
static int zevent_timer_check(ZEVENT_BASE * aio);

static int zevent_base_notify_reader(ZEVENT * zev, void *context)
{
	uint64_t u;
	int ret;

	ret = read(zevent_get_fd(zev), &u, sizeof(uint64_t));

	return ret;
}

int zevent_base_notify(ZEVENT_BASE * eb)
{
	uint64_t u;
	int ret;

	u = 1;
	ret = write(eb->eventfd_event.fd, &u, sizeof(uint64_t));

	return ret;
}

ZEVENT_BASE *zevent_base_create(void)
{
	ZEVENT_BASE *eb;
	int efd;

	eb = (ZEVENT_BASE *) zcalloc(1, sizeof(ZEVENT_BASE));

	zrbtree_init(&(eb->event_timer_tree), zevent_timer_cmp);
	zrbtree_init(&(eb->aio_timer_tree), zaio_timer_cmp);
	ztimer_base_init(&(eb->timer_tree));

	eb->aio_rwbuf_mpool = zmpool_create(sizeof(ZAIO_RWBUF), 100, 1000);

	pthread_mutex_init(&(eb->locker), 0);

	eb->epoll_fd = epoll_create(1024);
	zio_close_on_exec(eb->epoll_fd, 1);

	eb->epoll_event_count = 32;
	eb->epoll_event_list = (struct epoll_event *)zmalloc(sizeof(struct epoll_event) * eb->epoll_event_count);

	efd = eventfd(0, 0);
	zio_nonblocking(efd, 1);

	zevent_init(&(eb->eventfd_event), eb, efd);
	zevent_set(&(eb->eventfd_event), ZEVENT_READ, zevent_base_notify_reader, 0, 0);

	return eb;
}

void zevent_base_dispatch(ZEVENT_BASE * eb, int delay)
{
	int i, nfds, events, zaios;
	struct epoll_event *ev;
	ZAIO *aio;
	ZEVENT *zev;
	void *context;
	ZRBTREE_NODE *rn, *head, *tail;

	aio = 0;
	ztimer_check(&(eb->timer_tree));
	zevent_timer_check(eb);
	zaio_timer_check(eb);

	while (eb->event_base_queue_head) {
		ZEVENT_BASE_CB_FN callback;
		ZEVENT_BASE_QUEUE *rn, *head, *tail;

		ZEVENT_BASE_LOCK(eb);
		rn = eb->event_base_queue_head;
		if (!rn) {
			ZEVENT_BASE_UNLOCK(eb);
			break;
		}
		head = eb->event_base_queue_head;
		tail = eb->event_base_queue_tail;
		ZMLINK_DETACH(head, tail, rn, prev, next);
		eb->event_base_queue_head = head;
		eb->event_base_queue_tail = tail;
		callback = rn->callback;
		context = rn->context;
		ZEVENT_BASE_UNLOCK(eb);
		zfree(rn);
		if (callback) {
			callback(eb, context);
		}
	}

	while (eb->event_set_list_head) {
		ZEVENT_CB_FN callback;

		ZEVENT_BASE_LOCK(eb);
		rn = eb->event_set_list_head;
		if (!rn) {
			ZEVENT_BASE_UNLOCK(eb);
			break;
		}
		head = eb->event_set_list_head;
		tail = eb->event_set_list_tail;
		ZMLINK_DETACH(head, tail, rn, zrbtree_left, zrbtree_right);
		eb->event_set_list_head = head;
		eb->event_set_list_tail = tail;
		zev = ZCONTAINER_OF(rn, ZEVENT, rbnode_time);
		callback = zev->callback;
		context = zev->context;
		ZEVENT_BASE_UNLOCK(eb);
		if (callback) {
			callback(zev, context);
		}
	}

	while (eb->aio_set_list_head) {
		ZAIO_CB_FN callback;

		ZEVENT_BASE_LOCK(eb);
		rn = eb->aio_set_list_head;
		if (!rn) {
			ZEVENT_BASE_UNLOCK(eb);
			break;
		}
		head = eb->aio_set_list_head;
		tail = eb->aio_set_list_tail;
		ZMLINK_DETACH(head, tail, rn, zrbtree_left, zrbtree_right);
		eb->aio_set_list_head = head;
		eb->aio_set_list_tail = tail;
		aio = ZCONTAINER_OF(rn, ZAIO, rbnode_time);
		callback = aio->callback;
		context = aio->context;
		ZEVENT_BASE_UNLOCK(eb);
		if (callback) {
			aio->ret = 0;
			callback(aio, context, 0);
		}
	}

	nfds = epoll_wait(eb->epoll_fd, eb->epoll_event_list, eb->epoll_event_count, 100);
	if (nfds == -1) {
		if (errno != EINTR) {
			zlog_fatal("zbase_dispatch: epoll_wait: %m");
		}
		return;
	}

	for (i = 0; i < nfds; i++) {
		ev = eb->epoll_event_list + i;
		events = ev->events;
		zaios = 0;
		if (events & EPOLLHUP) {
			zaios |= ZEVENT_HUP;
		}
		if (events & EPOLLRDHUP) {
			zaios |= ZEVENT_RDHUP;
		}
		if (events & EPOLLERR) {
			zaios |= ZEVENT_ERROR;
		}
		if (events & EPOLLIN) {
			zaios |= ZEVENT_READ;
		}
		if (events & EPOLLOUT) {
			zaios |= ZEVENT_WRITE;
		}
		aio = (ZAIO *) (ev->data.ptr);
		if (aio->aio_type == ZEVENT_TYPE_EVENT) {
			zev = (ZEVENT *) aio;
			zev->recv_events = zaios;
			if (zev->callback) {
				zev->callback(zev, zev->context);
			} else {
				zevent_set(zev, 0, 0, 0, 0);
			}
		} else if (1 || (aio->aio_type == ZEVENT_TYPE_AIO)) {
			aio->recv_events = zaios;
			zaio_action(aio);
		}
	}
	if (nfds == eb->epoll_event_count && nfds < 4096) {
		eb->epoll_event_count *= 2;
		eb->epoll_event_list = (struct epoll_event *)zrealloc(eb->epoll_event_list, sizeof(struct epoll_event) * (eb->epoll_event_count));
	}
}

void zevent_base_free(ZEVENT_BASE * eb)
{
	zmpool_free(eb->aio_rwbuf_mpool);
	close(eb->eventfd_event.fd);
	close(eb->epoll_fd);
	zfree(eb->epoll_event_list);

	zfree(eb);
}

static int zaio_timer_cmp(ZRBTREE_NODE * n1, ZRBTREE_NODE * n2)
{
	ZAIO *e1, *e2;
	int r;

	e1 = ZCONTAINER_OF(n1, ZAIO, rbnode_time);
	e2 = ZCONTAINER_OF(n2, ZAIO, rbnode_time);
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

static int zaio_timer_check(ZEVENT_BASE * eb)
{
	ZAIO *aio;
	ZAIO_CB_FN callback;
	void *context;
	ZRBTREE_NODE *rn;
	int delay;

	delay = 10000;
	if (!zrbtree_have_data(&(eb->aio_timer_tree))) {
		return delay;
	}

	while (1) {
		rn = zrbtree_first(&(eb->aio_timer_tree));
		if (!rn) {
			delay = 10000;
			break;
		}
		aio = ZCONTAINER_OF(rn, ZAIO, rbnode_time);
		delay = zmtime_left(aio->timeout);
		if (delay > 0) {
			break;
		}
		callback = aio->callback;
		context = aio->context;
		aio->recv_events = ZAIO_TIMEOUT;
		aio->ret = ZAIO_TIMEOUT;
		if (aio->rw_type == ZAIO_CB_FN_SLEEP) {
			aio->ret = 1;
		}
		if (callback) {
			(callback) (aio, context, 0);
		} else {
			zaio_event_set(aio, 0, 0);
		}
	}

	return delay;
}

static inline void ___zaio_cache_shift(ZAIO * aio, ZAIO_CACHE * ioc, void *data, int len)
{
	int rlen, i, olen = len;
	char *buf = (char *)data;
	char *cdata;
	ZAIO_RWBUF *rwb;
	ZEVENT_BASE *eb;

	eb = zaio_get_base(aio);

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
			___AIO_RWBUF_FREE(eb, ioc->head);
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

static inline void ___zaio_cache_first_line(ZAIO * aio, ZAIO_CACHE * ioc, char **data, int *len)
{
	ZAIO_RWBUF *rwb;

	rwb = ioc->head;
	if (!rwb) {
		*data = 0;
		*len = 0;
		return;
	}
	*data = (rwb->data + rwb->p1);
	*len = (rwb->p2 - rwb->p1 + 1);
}

static inline void ___zaio_cache_append(ZAIO * aio, ZAIO_CACHE * ioc, void *data, int len)
{
	char *buf = (char *)data;
	char *cdata;
	int i, p2;
	ZAIO_RWBUF *rwb;
	ZEVENT_BASE *eb;

	eb = zaio_get_base(aio);

	rwb = ioc->tail;
	p2 = 0;
	cdata = 0;
	if (rwb) {
		p2 = rwb->p2;
		cdata = rwb->data;
	}
	for (i = 0; i < len; i++) {
		if (!rwb || (p2 == ZAIO_P2_MAX)) {
			___AIO_RWBUF_MALLOC(eb, rwb);
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

static inline int zaio_try_ssl_read(ZAIO * aio, void *buf, int len)
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

static inline int zaio_try_ssl_write(ZAIO * aio, void *buf, int len)
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

static int zaio_try_ssl_connect(ZAIO * aio)
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

static int zaio_try_ssl_accept(ZAIO * aio)
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

int zaio_ssl_init(ZAIO * aio, ZSSL_CTX * ctx, ZAIO_CB_FN callback, void *context, int timeout)
{
	ZAIO_SSL *zssl;
	SSL *ssl;
	int rlen;

	ssl = SSL_new(ctx->ssl_ctx);
	SSL_set_fd(ssl, aio->fd);

	zssl = (ZAIO_SSL *) zcalloc(1, sizeof(ZAIO_SSL));
	zssl->ssl = ssl;
	zssl->server_or_client = ctx->server_or_client;

	aio->ssl = zssl;

	aio->rw_type = ZAIO_CB_FN_SSL_INIT;
	aio->context = context;
	aio->callback = callback;

	if (zssl->server_or_client) {
		rlen = zaio_try_ssl_accept(aio);
	} else {
		rlen = zaio_try_ssl_connect(aio);
	}

	if (rlen > 0) {
		zaio_ready_do(aio);
		return 0;
	}

	zaio_event_set(aio, 1, timeout);

	return 0;
}

void zaio_ssl_fini(ZAIO * aio)
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

ZAIO_SSL *zaio_ssl_detach(ZAIO * aio)
{
	ZAIO_SSL *zs;

	zs = aio->ssl;
	aio->ssl = 0;

	return zs;
}

int zaio_ssl_attach(ZAIO * aio, ZAIO_SSL * zssl)
{
	aio->ssl = zssl;

	return 0;
}

SSL *zaio_ssl_detach_ssl(ZAIO_SSL * assl)
{
	SSL *ssl;

	if (!assl->ssl) {
		return 0;
	}

	ssl = assl->ssl;
	assl->ssl = 0;

	return ssl;
}

int zaio_move(ZAIO * aio, ZEVENT_BASE * neb, ZAIO_CB_FN attach_callback, void *context)
{
	int fd;
	ZAIO_SSL *assl;

	fd = zaio_get_fd(aio);
	zaio_event_set(aio, 0, 0);
	assl = zaio_ssl_detach(aio);

	/* FIXME readbuf,writebuf may be not blank */
	zaio_fini(aio);

	zaio_init(aio, neb, fd);
	zaio_ssl_attach(aio, assl);

	zaio_attach(aio, attach_callback, context);

	return 0;
}

static void zaio_ready_do_once(ZAIO * aio)
{
	ZAIO_CB_FN callback;
	char worker_buf[1100000];
	unsigned int rw_type;
	int ret = 0;
	void *context;

	callback = aio->callback;
	context = aio->context;
	rw_type = aio->rw_type;
	ret = aio->ret;

	/* SSL connect/accept */
	if (rw_type == ZAIO_CB_FN_SSL_INIT) {
		if (callback) {
			(callback) (aio, context, 0);
		}
		return;
	}

	/* SLEEP */
	if (rw_type == ZAIO_CB_FN_SLEEP) {
		if (callback) {
			aio->ret = 1;
			(callback) (aio, context, 0);
		}
		return;
	}
	/* WRITE */
	if ((rw_type & ZAIO_CB_WRITE)) {
		if (callback) {
			(callback) (aio, context, 0);
		}
		return;
	}
	/* READ */
	if (ret < 1) {
		if (callback) {
			(callback) (aio, context, worker_buf);
		}
		return;
	}

	___zaio_cache_shift(aio, &(aio->read_cache), worker_buf, ret);
	if (rw_type == ZAIO_CB_FN_READ_N && aio->delimiter == 'N') {
		int LLL = 0, i;
		unsigned char *p = (unsigned char *)worker_buf;;
		for (i = 0; i < 4; i++) {
			LLL = LLL * 256 + p[i];
		}
		aio->delimiter = '\0';
		if (LLL > 1024 * 1024 || LLL < 0) {
			aio->ret = -1;
			if (callback) {
				(callback) (aio, context, worker_buf);
			}
			return;
		}
		if (LLL == 0) {
			if (callback) {
				aio->ret = 0;
				(callback) (aio, context, worker_buf);
			}
			return;
		}
		___zaio_read_n_TRUE(aio, LLL, callback, context, -1);
	} else if (callback) {
		worker_buf[ret] = 0;
		(callback) (aio, context, worker_buf);
	}
}

static void zaio_ready_do(ZAIO * aio)
{
	ZEVENT_BASE *eb = aio->event_base;

	eb->magic_aio = 0;
	if (aio->in_loop) {
		eb->magic_aio = aio;
		return;
	}
	aio->in_loop = 1;
	eb->magic_aio = aio;
	while (eb->magic_aio) {
		eb->magic_aio = 0;
		zaio_ready_do_once(aio);
	}
	aio->in_loop = 0;
}

void zaio_init(ZAIO * aio, ZEVENT_BASE * eb, int fd)
{
	memset(aio, 0, sizeof(ZAIO));
	aio->aio_type = ZEVENT_TYPE_AIO;
	aio->fd = fd;
	aio->event_base = eb;
}

void zaio_reset_base(ZAIO * aio, ZEVENT_BASE * eb_new)
{
	zaio_event_set(aio, 0, 0);

	if (aio->read_cache.len > 0) {
		___zaio_cache_shift(aio, &(aio->read_cache), 0, aio->read_cache.len);
	}
	if (aio->write_cache.len > 0) {
		___zaio_cache_shift(aio, &(aio->write_cache), 0, aio->write_cache.len);
	}
	aio->event_base = eb_new;
}

void zaio_fini(ZAIO * aio)
{
	zaio_event_set(aio, 0, 0);

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

static int zaio_event_set(ZAIO * aio, int ev_type, int timeout)
{
	ZEVENT_BASE *eb;
	int fd, events, old_events, e_events;
	struct epoll_event evt;
	ZRBTREE_NODE *rn;
	int rw_type;

	eb = aio->event_base;
	fd = aio->fd;
	rw_type = aio->rw_type;
	events = 0;

	if (ev_type == ZAIO_TIMEOUT) {
		goto timeout;
	}
	if (ev_type == 0) {
		/* clear event; */
		goto evdo;
	}
	/* compute the events */
	if (!(aio->ssl)) {
		if ((rw_type & ZAIO_CB_MAGIC) == ZAIO_CB_READ) {
			events |= ZEVENT_READ;
		}
		if ((rw_type & ZAIO_CB_MAGIC) == ZAIO_CB_WRITE) {
			events |= ZEVENT_WRITE;
		}
		if (aio->write_cache.len > 0) {
			events |= ZEVENT_WRITE;
		}
	} else {
		if (aio->ssl->read_want_read || aio->ssl->write_want_read) {
			events |= ZEVENT_READ;
		}
		if (aio->ssl->read_want_write || aio->ssl->write_want_write) {
			events |= ZEVENT_WRITE;
		}
		if (aio->write_cache.len > 0) {
			if ((aio->ssl->write_want_read == 0) && (aio->ssl->write_want_write == 0)) {
				aio->ssl->write_want_write = 1;
				events |= ZEVENT_WRITE;
			}
		}
	}
	/* compute over */

      evdo:
	e_events = EPOLLHUP | EPOLLRDHUP | EPOLLERR;
	old_events = aio->events;
	aio->events = events;

	if (events == 0) {
		if (old_events) {
			if (epoll_ctl(eb->epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1) {
				zlog_fatal("zaio_event_set: fd %d: DEL  error: %m", fd);
			}
		}
	} else if (old_events != events) {
		if (events & ZEVENT_READ) {
			e_events |= EPOLLIN;
		}
		if (events & ZEVENT_WRITE) {
			e_events |= EPOLLOUT;
		}
		if (events & ZEVENT_PERSIST) {
			e_events |= EPOLLET;
		}
		evt.events = e_events;
		evt.data.ptr = aio;
		if (epoll_ctl(eb->epoll_fd, (old_events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD), fd, &evt) == -1) {
			zlog_fatal("zaio_event_set: fd %d: %s error: %m", fd, (old_events ? "MOD" : "ADD"));
		}
	}

      timeout:

	rn = &(aio->rbnode_time);
	if ((aio->in_time) && (timeout != -1)) {
		zrbtree_detach(&(eb->aio_timer_tree), rn);
		aio->in_time = 0;
	}
	if (timeout > 0) {
		aio->in_time = 1;
		aio->enable_time = 1;
		aio->timeout = zmtime_set_timeout(timeout);
		zrbtree_attach(&(eb->aio_timer_tree), rn);
	} else if (timeout == 0) {
		aio->enable_time = 0;
	} else if (timeout == -1) {
		if ((aio->enable_time) && (!aio->in_time)) {
			zrbtree_attach(&(eb->aio_timer_tree), rn);
			aio->in_time = 1;
		}
	}

	return 0;
}

int zaio_attach(ZAIO * aio, ZAIO_CB_FN callback, void *context)
{
	ZEVENT_BASE *eb;
	ZRBTREE_NODE *rn, *head, *tail;

	eb = zaio_get_base(aio);

	aio->callback = callback;
	aio->context = context;

	ZEVENT_BASE_LOCK(eb);
	rn = &(aio->rbnode_time);
	head = eb->aio_set_list_head;
	tail = eb->aio_set_list_tail;
	ZMLINK_APPEND(head, tail, rn, zrbtree_left, zrbtree_right);
	eb->aio_set_list_head = head;
	eb->aio_set_list_tail = tail;
	ZEVENT_BASE_UNLOCK(eb);

	zevent_base_notify(eb);

	return 0;
}

int zaio_detach(ZAIO * aio)
{
	zaio_event_set(aio, 0, 0);

	return 0;
}

int zaio_read(ZAIO * aio, int max_len, ZAIO_CB_FN callback, void *context, int timeout)
{
	int magic, rlen;
	char buf[10240];

	aio->rw_type = ZAIO_CB_FN_READ;
	aio->context = context;
	aio->callback = callback;
	aio->read_magic_len = max_len;

	if (max_len < 1) {
		aio->ret = 0;
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

static int ___zaio_read_n_TRUE(ZAIO * aio, int strict_len, ZAIO_CB_FN callback, void *context, int timeout)
{
	int magic, rlen;
	char buf[10240];

	aio->rw_type = ZAIO_CB_FN_READ_N;
	aio->context = context;
	aio->callback = callback;
	aio->read_magic_len = strict_len;

	if (strict_len < 1) {
		aio->ret = 0;
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

int zaio_read_n(ZAIO * aio, int strict_len, ZAIO_CB_FN callback, void *context, int timeout)
{
	aio->delimiter = '\0';
	return ___zaio_read_n_TRUE(aio, strict_len, callback, context, timeout);
}

static inline int ___zaio_read_delimiter_check(ZAIO * aio, unsigned char delimiter, int max_len)
{
	int magic, i, end;
	char *buf;
	ZAIO_RWBUF *rwb;

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

int zaio_read_delimiter(ZAIO * aio, char delimiter, int max_len, ZAIO_CB_FN callback, void *context, int timeout)
{
	int magic, rlen;
	char buf[10240];
	char *data;

	aio->rw_type = ZAIO_CB_FN_READ_DELIMETER;
	aio->context = context;
	aio->callback = callback;
	aio->read_magic_len = max_len;
	aio->delimiter = delimiter;

	if (max_len < 1) {
		aio->ret = 0;
		zaio_ready_do(aio);
		return 0;
	}
	magic = ___zaio_read_delimiter_check(aio, (unsigned char)delimiter, max_len);
	while (1) {
		if (magic > 0) {
			aio->ret = magic;
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

int zaio_read_sizedata(ZAIO * aio, ZAIO_CB_FN callback, void *context, int timeout)
{
	aio->delimiter = 'N';
	return ___zaio_read_n_TRUE(aio, 4, callback, context, timeout);
}

int zaio_write_cache_append(ZAIO * aio, void *buf, int len)
{
	int rlen;
	int offset = 0;
	char *data = (char *)buf;

	if (len < 1) {
		return 0;
	}
	if (0) {
		if (aio->write_cache.len == 0) {
			while (1) {
				if (!(aio->ssl)) {
					rlen = write(aio->fd, data + offset, len - offset);
				} else {
					rlen = zaio_try_ssl_write(aio, data + offset, len - offset);
				}
				if (rlen < 1) {
					break;
				}
				offset += rlen;
			}
		}
		if (len > offset) {
			___zaio_cache_append(aio, &(aio->write_cache), data + offset, len - offset);
		}
	} else {
		___zaio_cache_append(aio, &(aio->write_cache), data, len);
	}

	return len;
}

int zaio_write_cache_append_sizedata(ZAIO * aio, int len, void *buf)
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

int zaio_write_cache_flush(ZAIO * aio, ZAIO_CB_FN callback, void *context, int timeout)
{
	aio->ret = 1;
	aio->rw_type = ZAIO_CB_FN_WRITE;
	aio->context = context;
	aio->callback = callback;

	zaio_event_set(aio, 1, timeout);

	return 0;
}

int zaio_write_cache_get_len(ZAIO *aio)
{
	return aio->write_cache.len;
}

int zaio_sleep(ZAIO * aio, ZAIO_CB_FN callback, char *context, int timeout)
{
	aio->rw_type = ZAIO_CB_FN_SLEEP;
	aio->context = context;
	aio->callback = callback;

	zaio_event_set(aio, 0, timeout);

	return 0;
}

static inline int zaio_action_read_once(ZAIO * aio, int rw_type, char *buf)
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
	buf[rlen] = 0;
	___zaio_cache_append(aio, &(aio->read_cache), buf, rlen);

	if (rw_type == ZAIO_CB_FN_READ) {
		if (aio->read_cache.len >= aio->read_magic_len) {
			aio->ret = aio->read_magic_len;
		} else {
			aio->ret = aio->read_cache.len;
		}
		return 0;
	}
	if (rw_type == ZAIO_CB_FN_READ_N) {
		if (aio->read_cache.len >= aio->read_magic_len) {
			aio->ret = aio->read_magic_len;
			return 0;
		} else {
			return 1;
		}
	}
	if (rw_type == ZAIO_CB_FN_READ_DELIMETER) {
		data = memchr(buf, aio->delimiter, rlen);
		if (data) {
			aio->ret = aio->read_cache.len - rlen + (data - buf + 1);
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

static inline int zaio_action_read(ZAIO * aio, int rw_type)
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

static inline int zaio_action_write(ZAIO * aio, int rw_type)
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

static int zaio_action(ZAIO * aio)
{
	unsigned int rw_type;
	int events, transfer;
	ZAIO_SSL *ssl = aio->ssl;

	transfer = 1;
	rw_type = aio->rw_type;
	events = aio->recv_events;

	if (events & ZEVENT_EXCEPTION) {
		aio->ret = ZAIO_ERROR;
		transfer = 0;
		goto transfer;
	}
#define _ssl_w (((ssl->write_want_write) &&(events & ZEVENT_WRITE))||((ssl->write_want_read) &&(events & ZEVENT_READ)))
#define _ssl_r (((ssl->read_want_write) &&(events & ZEVENT_WRITE))||((ssl->read_want_read) &&(events & ZEVENT_READ)))
	if (((aio->ssl) && (_ssl_w)) || ((!(aio->ssl)) && (events & ZEVENT_WRITE))) {
		transfer = zaio_action_write(aio, rw_type);
		if (rw_type & ZAIO_CB_WRITE) {
			goto transfer;
		}
		transfer = 1;
		goto transfer;
	}

	if (((aio->ssl) && (_ssl_r)) || ((!(aio->ssl)) && (events & ZEVENT_READ))) {
		transfer = zaio_action_read(aio, rw_type);
		goto transfer;
	}

	aio->ret = ZAIO_ERROR;
	transfer = 0;

      transfer:
	if (aio->ssl && aio->ssl->error) {
		aio->ret = ZAIO_ERROR;
		transfer = 0;
	}

	if (transfer) {
		zaio_event_set(aio, 1, -1);
		return 1;
	}

	zaio_ready_do(aio);

	return 0;
}

/* EVENT */

static int zevent_timer_cmp(ZRBTREE_NODE * n1, ZRBTREE_NODE * n2)
{
	ZEVENT *e1, *e2;
	int r;

	e1 = ZCONTAINER_OF(n1, ZEVENT, rbnode_time);
	e2 = ZCONTAINER_OF(n2, ZEVENT, rbnode_time);
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

static int zevent_timer_check(ZEVENT_BASE * aio)
{
	ZEVENT *zev;
	ZRBTREE_NODE *rn;
	ZEVENT_CB_FN callback;
	void *context;
	int delay;

	delay = 10000;
	if (!zrbtree_have_data(&(aio->event_timer_tree))) {
		return delay;
	}
	while (1) {
		rn = zrbtree_first(&(aio->event_timer_tree));
		if (!rn) {
			break;
		}
		zev = ZCONTAINER_OF(rn, ZEVENT, rbnode_time);
		delay = zmtime_left(zev->timeout);
		if (delay > 0) {
			break;
		}
		callback = zev->callback;
		context = zev->context;
		zev->recv_events = ZEVENT_TIMEOUT;
		if (callback) {
			(*callback) (zev, context);
		} else {
			zevent_set(zev, 0, 0, 0, 0);
		}
	}

	return delay;
}

int zevent_attach(ZEVENT * zev, ZEVENT_CB_FN callback, void *context)
{
	ZEVENT_BASE *eb = zevent_get_base(zev);
	ZRBTREE_NODE *rn, *head, *tail;

	zev->callback = callback;
	zev->context = context;

	ZEVENT_BASE_LOCK(eb);
	rn = &(zev->rbnode_time);
	head = eb->event_set_list_head;
	tail = eb->event_set_list_tail;
	ZMLINK_APPEND(head, tail, rn, zrbtree_left, zrbtree_right);
	eb->event_set_list_head = head;
	eb->event_set_list_tail = tail;
	ZEVENT_BASE_UNLOCK(eb);

	zevent_base_notify(eb);

	return 0;
}

int zevent_detach(ZEVENT * zev)
{
	return zevent_set(zev, 0, 0, 0, 0);
}

int zevent_set(ZEVENT * zev, int events, ZEVENT_CB_FN callback, void *context, int timeout)
{
	int fd, old_events, e_events;
	struct epoll_event evt;
	ZEVENT_BASE *aio;
	ZRBTREE_NODE *rn;

	aio = zev->event_base;
	fd = zevent_get_fd(zev);
	e_events = EPOLLHUP | EPOLLRDHUP | EPOLLERR;

	zev->callback = callback;
	zev->context = context;
	old_events = zev->events;
	zev->events = events;
	if (callback == 0) {
		events = 0;
		timeout = 0;
	}

	if (zev->fd < 0) {
		goto timeout;
	}
	if (events == 0) {
		if (old_events) {
			if (epoll_ctl(aio->epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1) {
				zlog_fatal("zevent_set: fd %d: DEL  error: %m", fd);
			}
		}
	} else if (old_events != events) {
		if (events & ZEVENT_READ) {
			e_events |= EPOLLIN;
		}
		if (events & ZEVENT_WRITE) {
			e_events |= EPOLLOUT;
		}
		if (events & ZEVENT_PERSIST) {
			e_events |= EPOLLET;
		}
		evt.events = e_events;
		evt.data.ptr = zev;
		if (epoll_ctl(aio->epoll_fd, (old_events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD), fd, &evt) == -1) {
			zlog_fatal("zevent_set: fd %d: %s error: %m", fd, (old_events ? "MOD" : "ADD"));
		}
	}

      timeout:
	rn = &(zev->rbnode_time);
	if ((zev->in_time) && (timeout > -1)) {
		zrbtree_detach(&(aio->event_timer_tree), rn);
		zev->in_time = 0;
	}
	if (timeout > 0) {
		zev->in_time = 1;
		zev->enable_time = 1;
		zev->timeout = zmtime_set_timeout(timeout);
		zrbtree_attach(&(aio->event_timer_tree), rn);
	} else if (timeout == 0) {
		zev->enable_time = 0;
	} else {
		if ((zev->enable_time) && (!zev->in_time)) {
			zrbtree_attach(&(aio->event_timer_tree), rn);
			zev->in_time = 1;
		}
	}

	return 0;
}

int zevent_unset(ZEVENT * zev)
{
	return zevent_set(zev, 0, 0, 0, 0);
}

void zevent_init(ZEVENT * zev, ZEVENT_BASE * eb, int fd)
{
	memset(zev, 0, sizeof(ZEVENT));
	zev->aio_type = ZEVENT_TYPE_EVENT;
	zev->event_base = eb;
	zev->fd = fd;
}

void zevent_fini(ZEVENT * zev)
{
	zevent_set(zev, 0, 0, 0, 0);
}

/* connect */
int zaio_read(ZAIO * aio, int max_len, ZAIO_CB_FN callback, void *context, int timeout);
typedef struct {
	ZEVENT_CB_FN cb;
	void *ctx;
} ___ZEVENT_CONNECT;

static int ___zevent_connect_cb(ZEVENT * zev, void *context)
{
	___ZEVENT_CONNECT *zct;
	ZEVENT_CB_FN callback;
	void *ctx;
	int sock;
	int error;
	socklen_t error_len;

	zct = (___ZEVENT_CONNECT *) context;
	callback = zct->cb;
	ctx = zct->ctx;
	zfree(zct);

	sock = zevent_get_fd(zev);

	if (zevent_get_events(zev) & ZEVENT_WRITE) {
		error = 0;
		error_len = sizeof(error);
		if (getsockopt(sock, SOL_SOCKET, SO_ERROR, (char *)&error, &error_len) < 0) {
			zev->recv_events = ZEVENT_ERROR;
		}
		if (error) {
			errno = error;
			zev->recv_events = ZEVENT_ERROR;
		}
	}
	zevent_attach(zev, callback, ctx);

	return 0;
}

static inline int ___zevent_connect(ZEVENT * zev, struct sockaddr *sa, socklen_t len, ZEVENT_CB_FN callback, void *context, int timeout)
{
	int sock;
	___ZEVENT_CONNECT *zct;

	sock = zevent_get_fd(zev);
	if (connect(sock, sa, len)) {
		zev->recv_events = ZEVENT_WRITE;
		zevent_attach(zev, callback, context);
		return 0;
	}

	if (errno != EINPROGRESS) {
		zev->recv_events = ZEVENT_ERROR;
		zevent_attach(zev, callback, context);
		return 0;
	}

	zct = (___ZEVENT_CONNECT *) zcalloc(1, sizeof(___ZEVENT_CONNECT));
	zct->cb = callback;
	zct->ctx = context;
	zevent_set(zev, ZEVENT_WRITE, ___zevent_connect_cb, zct, timeout);

	return 0;

}

int zevent_inet_connect(ZEVENT * zev, char *dip, int port, ZEVENT_CB_FN callback, void *context, int timeout)
{
	int sock;
	struct sockaddr_in addr;
	int on = 1;

	sock = zevent_get_fd(zev);
	zio_nonblocking(sock, 1);
	(void)setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char *)&on, sizeof(on));

	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(dip);
	___zevent_connect(zev, (struct sockaddr *)&addr, sizeof(struct sockaddr_in), callback, context, timeout);

	return 0;
}

int zevent_unix_connect(ZEVENT * zev, char *path, ZEVENT_CB_FN callback, void *context, int timeout)
{
	struct sockaddr_un addr;
	int len = strlen(path);

	memset((char *)&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	if (len < (int)sizeof(addr.sun_path)) {
		memcpy(addr.sun_path, path, len + 1);
	} else {
		zev->recv_events = ZEVENT_ERROR;
		zevent_attach(zev, callback, context);
		return 0;
	}
	___zevent_connect(zev, (struct sockaddr *)&addr, sizeof(struct sockaddr_un), callback, context, timeout);

	return 0;
}

/* base attach link */
int zevent_base_queue_enter(ZEVENT_BASE * eb, ZEVENT_BASE_CB_FN callback, void *context)
{
	ZEVENT_BASE_QUEUE *qnode, *head, *tail;

	qnode = (ZEVENT_BASE_QUEUE *) zcalloc(1, sizeof(ZEVENT_BASE_QUEUE));
	qnode->callback = callback;
	qnode->context = context;

	ZEVENT_BASE_LOCK(eb);
	head = eb->event_base_queue_head;
	tail = eb->event_base_queue_tail;
	ZMLINK_APPEND(head, tail, qnode, prev, next);
	eb->event_base_queue_head = head;
	eb->event_base_queue_tail = tail;
	ZEVENT_BASE_UNLOCK(eb);

	zevent_base_notify(eb);

	return 0;
}

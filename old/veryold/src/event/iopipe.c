#include "zc.h"

#define ZIOPIPE_RBUF_SIZE		4096
#define ZIOPIPE_BASE_LOCK(iopb)		(pthread_mutex_lock(&((iopb)->locker)))
#define ZIOPIPE_BASE_UNLOCK(iopb)	(pthread_mutex_unlock(&((iopb)->locker)))

#define ZIOPIPE_ALLOC(iopb)		(zmalloc(sizeof(ZIOPIPE)))

/* to remiopber: free some instances once*/
#define ZIOPIPE_FREE(iop)		(zfree(iop))
#define ZIOPIPE_RBUF_ALLOC(iopb)	zmalloc(ZIOPIPE_RBUF_SIZE)
#define ZIOPIPE_RBUF_FREE(iopb, p)	zfree(p)

typedef struct ZIOPIPE_LINKER ZIOPIPE_LINKER;
struct ZIOPIPE_LINKER {
	int cfd;
	int sfd;
	SSL *cssl;
	SSL *sssl;
	ZIOPIPE_LINKER *last;
	ZIOPIPE_LINKER *next;
};

static void ziopipe_set_event(ZIOPIPE_BASE * iopb, ZIOPIPE_PART * part, int events);
static inline int try_ssl_write(ZIOPIPE_PART * part, void *buf, int len);
static inline int try_ssl_read(ZIOPIPE_PART * part, void *buf, int len);

ZIOPIPE_BASE *ziopipe_base_create(void)
{
	ZIOPIPE_BASE *iopb;
	int efd;

	iopb = (ZIOPIPE_BASE *) zcalloc(1, sizeof(ZIOPIPE_BASE));

	pthread_mutex_init(&(iopb->locker), 0);

	iopb->epoll_fd = epoll_create(1024);
	zio_close_on_exec(iopb->epoll_fd, 1);

	iopb->epoll_event_count = 32;
	iopb->epoll_event_list = (struct epoll_event *)zmalloc(sizeof(struct epoll_event) * iopb->epoll_event_count);

	efd = eventfd(0, 0);
	zio_nonblocking(efd, 1);
	iopb->eventfd_iop.client.fd = efd;
	iopb->eventfd_iop.client.is_client_or_server = 0;
	ziopipe_set_event(iopb, &(iopb->eventfd_iop.client), ZEVENT_READ);

	return iopb;
}

int loop = 1;
void ziopipe_base_dispatch(ZIOPIPE_BASE * iopb, int delay)
{
	int i, nfds, zev, events, is_client_or_server, e_err;
	struct epoll_event *ev;
	ZIOPIPE *iop;
	ZIOPIPE_PART *part, *part_a, *part_client, *part_server;
	int efd;
	ZIOPIPE_LINKER *linker, *h, *t;

	efd = iopb->eventfd_iop.client.fd;

	loop++;

	while (iopb->set_list_head) {
		ZIOPIPE_BASE_LOCK(iopb);
		linker = (ZIOPIPE_LINKER *) (iopb->set_list_head);
		if (!linker) {
			ZIOPIPE_BASE_UNLOCK(iopb);
			break;
		}
		h = (ZIOPIPE_LINKER *) (iopb->set_list_head);
		t = (ZIOPIPE_LINKER *) (iopb->set_list_tail);
		ZMLINK_DETACH(h, t, linker, last, next);
		iopb->set_list_head = h;
		iopb->set_list_tail = t;
		ZIOPIPE_BASE_UNLOCK(iopb);

		int cfd, sfd;
		SSL *cssl, *sssl;
		cfd = linker->cfd;
		sfd = linker->sfd;
		cssl = linker->cssl;
		sssl = linker->sssl;
		iop = (ZIOPIPE *) linker;
		memset(iop, 0, sizeof(ZIOPIPE));
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
		ziopipe_set_event(iopb, &(iop->client), ZEVENT_READ);
		ziopipe_set_event(iopb, &(iop->server), ZEVENT_READ);
	}

	nfds = epoll_wait(iopb->epoll_fd, iopb->epoll_event_list, iopb->epoll_event_count, 10000);
	if (nfds == -1) {
		if (errno != EINTR) {
			zlog_fatal("zbase_dispatch: epoll_wait: %m");
		}
		return;
	}

	for (i = 0; i < nfds; i++) {
		ev = iopb->epoll_event_list + i;
		events = ev->events;
		part = (ZIOPIPE_PART *) (ev->data.ptr);
		is_client_or_server = part->is_client_or_server;
		if (!is_client_or_server) {
			iop = ZCONTAINER_OF(part, ZIOPIPE, client);
		} else {
			iop = ZCONTAINER_OF(part, ZIOPIPE, server);
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
				ZIOPIPE_RBUF_FREE(iopb, part_client->rbuf);
			}
			if (part_server->rbuf) {
				ZIOPIPE_RBUF_FREE(iopb, part_server->rbuf);
			}
			if (part->ssl) {
				SSL_free(part->ssl);
			}
			if (part_a->ssl) {
				SSL_free(part_a->ssl);
			}
			close(part_client->fd);
			close(part_server->fd);
			ZIOPIPE_FREE(iop);
			continue;
		}
		zev = 0;

		if (events & EPOLLOUT) {
			zev |= ZEVENT_WRITE;
		}
		if (events & EPOLLIN) {
			zev |= ZEVENT_READ;
		}
#define _debug_part(p)	printf("client_or_server: %ld, %d, %d, %d, %d\n", p->is_client_or_server, p->read_want_read, p->read_want_write, p->write_want_read, p->write_want_write)

#define _ssl_w (((part->write_want_write) &&(zev & ZEVENT_WRITE))||((part->write_want_read) &&(zev & ZEVENT_READ)))
		if (((part->ssl) && (_ssl_w)) || ((!(part->ssl)) && (zev & ZEVENT_WRITE))) {
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
						ZIOPIPE_RBUF_FREE(iopb, part_a->rbuf);
						part_a->rbuf = 0;

					}
				}
			}
		}
#define _ssl_r (((part->read_want_write) &&(zev & ZEVENT_WRITE))||((part->read_want_read) &&(zev & ZEVENT_READ)))
		if (((part->ssl) && (_ssl_r)) || ((!(part->ssl)) && (zev & ZEVENT_READ))) {
			int rlen;
			if (part->rbuf_p2 - part->rbuf_p1 == 0) {
				part->rbuf_p1 = part->rbuf_p2 = 0;
				if (!(part->rbuf)) {
					part->rbuf = (char *)ZIOPIPE_RBUF_ALLOC(iopb);
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
				eev |= ZEVENT_READ;
			}
			if (part->read_want_write || part->write_want_write) {
				eev |= ZEVENT_WRITE;
			}
			if (part_a->rbuf_p2 - part_a->rbuf_p1 > 0) {
				if ((part->write_want_read == 0) && (part->write_want_write == 0)) {
					part->write_want_write = 1;
					eev |= ZEVENT_WRITE;
				}
			}
		} else {
			if (part_a->rbuf_p2 - part_a->rbuf_p1 > 0) {
				eev |= ZEVENT_WRITE;
			}
		}
		if (part_a->ssl) {
			if (part_a->read_want_read || part_a->write_want_read) {
				eev_a |= ZEVENT_READ;
			}
			if (part_a->read_want_write || part_a->write_want_write) {
				eev_a |= ZEVENT_WRITE;
			}
			if (part->rbuf_p2 - part->rbuf_p1 > 0) {
				if ((part_a->write_want_read == 0) && (part_a->write_want_write == 0)) {
					part_a->write_want_write = 1;
					eev_a |= ZEVENT_WRITE;
				}
			}
		} else {
			if (part->rbuf_p2 - part->rbuf_p1 > 0) {
				eev_a |= ZEVENT_WRITE;
			}
		}
		if (!eev && !eev_a) {
			eev = ZEVENT_READ;
			if (part->ssl) {
				part->read_want_read = 0;
				part->read_want_write = 0;
				part->write_want_read = 0;
				part->write_want_write = 0;
				part->read_want_read = 1;
			}
			eev_a = ZEVENT_READ;
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
}

void ziopipe_base_free(ZIOPIPE_BASE * iopb)
{
	return;
}

void ziopipe_enter(ZIOPIPE_BASE * iopb, int client_fd, SSL * client_ssl, int server_fd, SSL * server_ssl)
{
	uint64_t u;
	ZIOPIPE_LINKER *linker, *h, *t;

	zio_nonblocking(client_fd, 1);
	zio_nonblocking(server_fd, 1);

	linker = (ZIOPIPE_LINKER *) ZIOPIPE_ALLOC(iopb);

	memset(linker, 0, sizeof(ZIOPIPE));
	linker->cfd = client_fd;
	linker->sfd = server_fd;
	linker->cssl = client_ssl;
	linker->sssl = server_ssl;

	ZIOPIPE_BASE_LOCK(iopb);
	h = (ZIOPIPE_LINKER *) (iopb->set_list_head);
	t = (ZIOPIPE_LINKER *) (iopb->set_list_tail);
	ZMLINK_APPEND(h, t, linker, last, next);
	iopb->set_list_head = h;
	iopb->set_list_tail = t;
	ZIOPIPE_BASE_UNLOCK(iopb);

	if (write(iopb->eventfd_iop.client.fd, &u, sizeof(uint64_t))) ;
}

static void ziopipe_set_event(ZIOPIPE_BASE * iopb, ZIOPIPE_PART * part, int events)
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
				zlog_fatal("ziopipe_set_event: fd %d: DEL  error: %m", fd);
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
		evt.data.ptr = part;
		if (epoll_ctl(iopb->epoll_fd, (old_events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD), fd, &evt) == -1) {
			zlog_fatal("ziopipe_set_event: fd %d: %s error: %m", fd, (old_events ? "MOD" : "ADD"));
		}
	}
}

static inline int try_ssl_write(ZIOPIPE_PART * part, void *buf, int len)
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

static inline int try_ssl_read(ZIOPIPE_PART * part, void *buf, int len)
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

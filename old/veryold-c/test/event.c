#include "zc.h"
#include "test_lib.h"

/* test event/trigger */

typedef struct {
	char buf[1024];
	int len;
} _BF;

int cb_write(ZEVENT * zev, void *context);

int _error(ZEVENT * zev, void *context)
{
	int events;
	int fd;

	events = zevent_get_events(zev);
	fd = zevent_get_fd(zev);
	if (events & ZEVENT_TIMEOUT) {
		zlog_info("%d: idle too long", fd);
	} else {
		zlog_info("%d: connection error", fd);
	}
	if (context) {
		zfree(context);
	}
	zevent_fini(zev);
	close(fd);

	return -1;
}

int cb_read(ZEVENT * zev, void *context)
{
	_BF *bf;
	int events;
	int fd;

	events = zevent_get_events(zev);
	fd = zevent_get_fd(zev);

	if (events & ZEVENT_EXCEPTION) {
		return _error(zev, context);
	}

	bf = (_BF *) context;
	if (bf == 0) {
		bf = (_BF *) zcalloc(1, sizeof(_BF));
	}

	bf->len = read(fd, bf->buf, 32);
	if (bf->len < 0) {
		bf->len = 0;
	}
	bf->buf[bf->len] = 0;

	zevent_set(zev, ZEVENT_WRITE, cb_write, bf, -1);

	return 0;
}

int cb_write(ZEVENT * zev, void *context)
{
	_BF *bf;
	int events;
	int fd;

	events = zevent_get_events(zev);
	fd = zevent_get_fd(zev);

	if (events & ZEVENT_TIMEOUT) {
		return _error(zev, context);
	}
	bf = (_BF *) context;
	if (bf == 0) {
		if (write(fd, "hello\n\n\n", 6)) ;
	} else {
		if (write(fd, bf->buf, bf->len)) ;
	}
	zevent_set(zev, ZEVENT_READ, cb_read, bf, 8000);

	return 0;
}

int cb_conn(ZEVENT * zev, void *context)
{
	int fd;
	int events;
	ZEVENT *zev2;

	events = zevent_get_events(zev);

	zevent_set(zev, ZEVENT_TIMEOUT, 0, 0, 2000);

	if (events & ZEVENT_TIMEOUT) {
		zlog_info("no connection in the past 2 secs: %lu", time(0));
		return -1;
	}
	fd = zsocket_inet_accept(zevent_get_fd(zev));
	if (fd < 0) {
		return -2;
	}

	char hostbuf[16];
	int host, port;

	zdns_getpeer_r(fd, &host, &port);
	zdns_inet_ntoa_r((struct in_addr *)&host, hostbuf);
	zlog_info("new connection: %d: (%s:%d)", fd, hostbuf, port);
	zev2 = (ZEVENT *) zmalloc(sizeof(ZEVENT));
	zevent_init(zev2, zevent_get_base(zev), fd);
	zevent_set(zev2, ZEVENT_WRITE, cb_write, 0, 8000);

	return 0;
}

int timer_cb(ZTIMER * zt, void *ctx)
{
	zlog_info("now exit!");
	exit(1);

	return 0;
}

int main(int argc, char **argv)
{
	int port;
	int sock;
	ZEVENT_BASE *eb;
	ZEVENT zev;
	ZTIMER tm;

	port = 99;

	eb = zevent_base_create();

	sock = zsocket_inet_listen(0, port, 5);
	zio_nonblocking(sock, 1);

	ztimer_init(&tm, eb);
	ztimer_set(&tm, timer_cb, 0, 10 * 1000);

	zevent_init(&zev, eb, sock);
	zevent_set(&zev, ZEVENT_READ, cb_conn, 0, 2000);

	unsigned long t1;
	t1 = time(0);
	int t1flag = 1;

	while (1) {
		zevent_base_dispatch(eb, 0);
		if (t1flag && (time(0) - t1) > 10) {
			t1flag = 0;
			close(sock);
			printf("close\n");
		}
	}

	return 0;
}

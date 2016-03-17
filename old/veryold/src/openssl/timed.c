#include "zc.h"

static inline int zssl_timed_do(ZSSL * zssl, int (*hsfunc) (SSL *), int (*rfunc) (SSL *, void *, int), int (*wfunc) (SSL *, const void *, int), void *buf, int num, int timeout)
{
	int ret;
	int status;
	int err;
	SSL *ssl = zssl->ssl;
	long start_time;
	int left_time;
	int fd = zssl->fd;

	start_time = zmtime_set_timeout(timeout);

	for (;;) {
		if (hsfunc) {
			status = hsfunc(ssl);
		} else if (rfunc) {
			status = rfunc(ssl, buf, num);
		} else if (wfunc) {
			status = wfunc(ssl, buf, num);
		} else {
			zlog_fatal("zssl_timed_do: nothing to do here");
		}
		err = SSL_get_error(ssl, status);

		switch (err) {
		case SSL_ERROR_WANT_WRITE:
			if ((left_time = zmtime_left(start_time)) < 0) {
				return ZIO_TIMEOUT;
			}
			if ((ret = zio_write_wait(fd, left_time)) < 0) {
				return (ret);
			}
			break;
		case SSL_ERROR_WANT_READ:
			if ((left_time = zmtime_left(start_time)) < 0) {
				return ZIO_TIMEOUT;
			}
			if ((ret = zio_read_wait(fd, left_time)) < 0) {
				return (ret);
			}
			break;

		default:
			ZDEBUG("zssl_timed_do: unexpected SSL_ERROR code %d", err);
		case SSL_ERROR_SSL:
			ZDEBUG("zssl_timed_do: SSL_ERROR_SSL");
			/* FIXME */
		case SSL_ERROR_ZERO_RETURN:
		case SSL_ERROR_NONE:
			errno = 0;
		case SSL_ERROR_SYSCALL:
			return (status);
		}
	}

	return 0;
}

int zssl_connect(ZSSL * ssl, int timeout)
{
	return zssl_timed_do(ssl, SSL_connect, 0, 0, 0, 0, timeout);
}

int zssl_accept(ZSSL * ssl, int timeout)
{
	return zssl_timed_do(ssl, SSL_accept, 0, 0, 0, 0, timeout);
}

int zssl_shutdown(ZSSL * ssl, int timeout)
{
	return zssl_timed_do(ssl, SSL_shutdown, 0, 0, 0, 0, timeout);
}

int zssl_read(ZSSL * ssl, void *buf, int len, int timeout)
{
	return zssl_timed_do(ssl, 0, SSL_read, 0, buf, len, timeout);
}

int zssl_write(ZSSL * ssl, void *buf, int len, int timeout)
{
	return zssl_timed_do(ssl, 0, 0, SSL_write, buf, len, timeout);
}

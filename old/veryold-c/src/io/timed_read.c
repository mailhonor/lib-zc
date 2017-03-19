#include "zc.h"

int zio_read_wait(int fd, int timeout)
{
	struct pollfd pollfd;

	pollfd.fd = fd;
	pollfd.events = POLLIN;
	for (;;) {
		switch (poll(&pollfd, 1, timeout)) {
		case -1:
			if (errno != EINTR) {
				return (ZIO_ERROR);
			}
			/* timeout should be recalculated */
			continue;
		case 0:
			errno = ETIMEDOUT;
			return (ZIO_TIMEOUT);
		default:
			if (pollfd.revents & POLLNVAL) {
				return ZIO_ERROR;
			}
			return 1;
		}
	}

	return 1;
}

ssize_t zio_timed_read(int fd, void *buf, size_t len, int timeout)
{
	ssize_t ret;

	for (;;) {
		if ((ret = zio_read_wait(fd, timeout)) < 0) {
			return (ret);
		}
		if ((ret = read(fd, buf, len)) < 0) {
			if (errno == EINTR) {
				continue;
			}
			return (ZIO_ERROR);
		} else if (ret == 0) {
			return ZIO_EOF;
		}
		return (ret);
	}

	return ZIO_EOF;
}

ssize_t zio_timed_strict_read(int fd, void *buf, size_t len, int timeout)
{
	ssize_t ret;
	int left;
	char *ptr;
	long start_time;
	int left_time;

	left = len;
	ptr = buf;

	start_time = zmtime_set_timeout(timeout);

	while (left > 0) {
		left_time = zmtime_left(start_time);
		if (left_time < 1) {
			return (ZIO_TIMEOUT);
		}
		if ((ret = zio_read_wait(fd, left_time)) < 0) {
			return ret;
		}
		ret = read(fd, ptr, left);
		if (ret < 0) {
			if (errno == EINTR) {
				continue;
			}
			return (ZIO_ERROR);
		} else if (ret == 0) {
			return (ZIO_ERROR);
		} else {
			left -= ret;
			ptr += ret;
		}
	}

	return len;
}

ssize_t zio_timed_read_delimiter(int fd, void *buf, size_t len, int delimiter, int timeout)
{
	ssize_t ret;
	int left;
	char *ptr, *p;
	long start_time;
	int left_time;

	left = len;
	ptr = buf;

	start_time = zmtime_set_timeout(timeout);

	while (left > 0) {
		left_time = zmtime_left(start_time);
		if (left_time < 1) {
			return (ZIO_TIMEOUT);
		}
		if ((ret = zio_read_wait(fd, left_time)) < 0) {
			return ret;
		}
		ret = read(fd, ptr, left);
		if (ret < 0) {
			if (errno == EINTR) {
				continue;
			}
			return (ZIO_ERROR);
		} else if (ret == 0) {
			return (ZIO_ERROR);
		} else {
			p = memchr(ptr, delimiter, ret);
			if (p) {
				return (len - left + (p - ptr + 1));
			}
			left -= ret;
			ptr += ret;
		}
	}

	return len;
}

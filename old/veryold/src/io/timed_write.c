#include "zc.h"

int zio_write_wait(int fd, int timeout)
{
	struct pollfd pollfd;

	pollfd.fd = fd;
	pollfd.events = POLLOUT;
	for (;;) {
		switch (poll(&pollfd, 1, timeout)) {
		case -1:
			if (errno != EINTR) {
				return (ZIO_ERROR);
			}
			continue;
		case 0:
			errno = ETIMEDOUT;
			return (ZIO_TIMEOUT);
		default:
			if (pollfd.revents & POLLNVAL) {
				return (ZIO_ERROR);
			}
			return 1;
		}
	}

	return 1;
}

ssize_t zio_timed_write(int fd, void *buf, size_t len, int timeout)
{
	ssize_t ret;

	for (;;) {
		if ((ret = zio_write_wait(fd, timeout)) < 0) {
			return (ret);
		}
		if ((ret = write(fd, buf, len)) < 0) {
			if (errno == EINTR) {
				continue;
			}
			return (ZIO_ERROR);
		} else if (ret == 0) {
			return ZIO_EOF;
		} else {
			return (ret);
		}
	}

	return 0;
}

ssize_t zio_timed_strict_write(int fd, void *buf, size_t len, int timeout)
{
	ssize_t ret;
	int left;
	char *ptr;
	long start_time;
	int left_time;

	start_time = zmtime_set_timeout(timeout);

	left = len;
	ptr = buf;

	while (left > 0) {
		left_time = zmtime_left(start_time);
		if (left_time < 1) {
			return (ZIO_TIMEOUT);
		}
		if ((ret = zio_write_wait(fd, left_time)) < 0) {
			return (ret);
		}
		ret = write(fd, ptr, left);
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

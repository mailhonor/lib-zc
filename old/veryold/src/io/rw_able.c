#include "zc.h"

int zio_readable_or_writeable(int fd, int events)
{
	struct pollfd pollfd;

	pollfd.fd = fd;
	pollfd.events = events;
	for (;;) {
		switch (poll(&pollfd, 1, 0)) {
		case -1:
			if (errno != EINTR) {
				return -1;
			}
			continue;
		case 0:
			return (0);
		default:
			if (pollfd.revents & POLLNVAL) {
				return -1;
			}
			return pollfd.revents;
			return (pollfd.revents & (POLLIN | POLLOUT));
			return (1);
		}
	}

	return 1;
}

int zio_rwable(int fd, int *r, int *w)
{
	int rw;

	rw = zio_readable_or_writeable(fd, POLLIN | POLLOUT);
	*r = *w = 0;
	if (rw & POLLIN) {
		*r = 1;
	}
	if (rw & POLLOUT) {
		*w = 1;
	}

	return (rw);
}

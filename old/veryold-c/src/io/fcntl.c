#include "zc.h"

int zio_nonblocking(int fd, int on)
{
	int flags;

	if ((flags = fcntl(fd, F_GETFL, 0)) < 0) {
		return -1;
	}

	if (fcntl(fd, F_SETFL, on ? flags | O_NONBLOCK : flags & ~O_NONBLOCK) < 0) {
		return -1;
	}

	return ((flags & O_NONBLOCK) != 0);
}

int zio_close_on_exec(int fd, int on)
{
	int flags;

	if ((flags = fcntl(fd, F_GETFD, 0)) < 0) {
		return -1;
	}

	if (fcntl(fd, F_SETFD, on ? flags | FD_CLOEXEC : flags & ~FD_CLOEXEC) < 0) {
		return -1;
	}

	return ((flags & FD_CLOEXEC) != 0);
}

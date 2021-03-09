#include "zyc.h"

int zio_read_wait(int fd, int timeout) {
	struct pollfd pollfd;

	pollfd.fd = fd;
	pollfd.events = POLLIN;
	for (;;) {
		switch (poll(&pollfd, 1, timeout * 1000)) {
			case -1:
				if (errno != EINTR)
					zmsg_fatal("poll: %m");
				continue;
			case 0:
				errno = ETIMEDOUT;
				return (-1);
			default:
				if (pollfd.revents & POLLNVAL)
					zmsg_fatal("poll: %m");
				return (0);
		}
	}
}

ssize_t zio_timed_read(int fd, void *buf, size_t len, int timeout) {
	ssize_t ret;

	for (;;) {
		if (timeout > 0 && zio_read_wait(fd, timeout) < 0)
			return (-2);
		if ((ret = read(fd, buf, len)) < 0 && timeout > 0 && errno == EAGAIN) {
			zmsg_warning("read() returns EAGAIN on a readable file descriptor!");
			sleep(1);
			continue;
		} else if (ret < 0 && errno == EINTR) {
			continue;
		} else {
			return (ret);
		}
	}
}

ssize_t zio_timed_strict_read(int fd, void *buf, size_t len, int timeout) {
	ssize_t ret;
	int left;
	char *ptr;

	/* we simply assume timeout > 0 */
	left = len;
	ptr=buf;

	while(left>0){
		if (zio_read_wait(fd, timeout) < 0)
			return (-2);
		ret = read(fd, ptr, left);
		if(ret < 0){
			if(errno == EAGAIN){
				zmsg_warning("read() returns EAGAIN on a readable file descriptor!");
				sleep(1);
				continue;
			}else if(errno == EINTR){
				continue;
			}else{
				return(ret);
			}
		}else if(ret==0){
			return (len-left);
		}else {
			left-=ret;
			ptr+=ret;
		}
	}
	return len;
}

int zio_write_wait(int fd, int timeout) {
	struct pollfd pollfd;

	pollfd.fd = fd;
	pollfd.events = POLLOUT;
	for (;;) {
		switch (poll(&pollfd, 1, timeout * 1000)) {
			case -1:
				if (errno != EINTR)
					zmsg_fatal("poll: %m");
				continue;
			case 0:
				errno = ETIMEDOUT;
				return (-1);
			default:
				if (pollfd.revents & POLLNVAL)
					zmsg_fatal("poll: %m");
				return (0);
		}
	}
}

ssize_t zio_timed_write(int fd, void *buf, size_t len, int timeout) {
	ssize_t ret;

	for (;;) {
		if (timeout > 0 && zio_write_wait(fd, timeout) < 0)
			return (-1);
		if ((ret = write(fd, buf, len)) < 0 && timeout > 0 && errno == EAGAIN) {
			zmsg_warning("write() returns EAGAIN on a writable file descriptor!");
			sleep(1);
			continue;
		} else if (ret < 0 && errno == EINTR) {
			continue;
		} else {
			return (ret);
		}
	}
}

ssize_t zio_timed_strict_write(int fd, void *buf, size_t len, int timeout) {
	ssize_t ret;
	int left;
	char *ptr;

	/* we simply assume timeout > 0 */
	left = len;
	ptr=buf;

	while(left>0){
		if (zio_write_wait(fd, timeout) < 0)
			return (-2);
		ret = write(fd, ptr, left);
		if(ret < 0){
			if(errno == EAGAIN){
				zmsg_warning("write() returns EAGAIN on a writable file descriptor!");
				sleep(1);
				continue;
			}else if(errno == EINTR){
				continue;
			}else{
				return(ret);
			}
		}else if(ret==0){
			return (len-left);
		}else {
			left-=ret;
			ptr+=ret;
		}
	}
	return len;
}

int zio_non_blocking(int fd, int on) {
	int flags;

	if ((flags = fcntl(fd, F_GETFL, 0)) < 0)
		zmsg_fatal("fcntl: get flags: %m");
	if (fcntl(fd, F_SETFL, on ? flags | O_NONBLOCK : flags & ~O_NONBLOCK) < 0)
		zmsg_fatal("fcntl: set non-blocking flag %s: %m", on ? "on" : "off");
	return ((flags & O_NONBLOCK) != 0);
}

int zio_close_on_exec(int fd, int on){
	int     flags;

	if ((flags = fcntl(fd, F_GETFD, 0)) < 0)
		zmsg_fatal("fcntl: get flags: %m");
	if (fcntl(fd, F_SETFD, on ? flags | FD_CLOEXEC : flags & ~FD_CLOEXEC) < 0)
		zmsg_fatal("fcntl: set close-on-exec flag %s: %m", on ? "on" : "off");
	return ((flags & FD_CLOEXEC) != 0);
}

ssize_t zio_peek(int fd){
	int     count;

	return (ioctl(fd, FIONREAD, (char *) &count) < 0 ? -1 : count);
}

int zio_readable_or_writeable(int fd, int rw)
{
	struct pollfd pollfd;

	pollfd.fd = fd;
	pollfd.events = (rw?POLLIN:POLLOUT);
	for (;;) {
		switch (poll(&pollfd, 1, 0)) {
			case -1:
				if (errno != EINTR)
					zmsg_fatal("poll: %m");
				continue;
			case 0:
				return (0);
			default:
				if (pollfd.revents & POLLNVAL)
					zmsg_fatal("poll: %m");
				return (1);
		}
	}
}

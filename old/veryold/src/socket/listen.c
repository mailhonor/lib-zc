#include "zc.h"

int zsocket_unix_listen(const char *addr, int backlog, int unlinked, mode_t mode)
{
	struct sockaddr_un sun;
	int len = strlen(addr);
	int sock, on;
	int errno2;

	sock = -1;
	if (len >= (int)sizeof(sun.sun_path)) {
		errno = ENAMETOOLONG;
		return -1;
	}

	if (unlinked) {
		if (unlink(addr) < 0 && errno != ENOENT) {
			goto err;
		}
	}

	memset((char *)&sun, 0, sizeof(sun));
	sun.sun_family = AF_UNIX;
	memcpy(sun.sun_path, addr, len + 1);
	if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		return -1;
	}

	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0) {
		goto err;
	}

	if (bind(sock, (struct sockaddr *)&sun, sizeof(sun)) < 0) {
		goto err;
	}

	if (mode != 0) {
		if (fchmod(sock, 0666) < 0) {
			goto err;
		}
	}

	if (listen(sock, backlog) < 0) {
		goto err;
	}

	return (sock);

      err:
	errno2 = errno;
	if (sock > -1) {
		close(sock);
	}
	errno = errno2;
	return -1;
}

int zsocket_inet_listen(char *sip, int port, int backlog)
{
	int sock;
	int on = 1;
	struct sockaddr_in addr;
	int errno2;
	struct linger linger;

	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = (ZEMPTY(sip) ? INADDR_ANY : inet_addr(sip));

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		return -1;
	}

	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0) {
		goto err;
	}

	linger.l_onoff = 0;
	linger.l_linger = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_LINGER, (char *)&linger, sizeof(linger)) < 0) {
		goto err;
	}

	if (bind(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0) {
		goto err;
	}

	if (listen(sock, backlog) < 0) {
		goto err;
	}

	return (sock);

      err:
	errno2 = errno;
	close(sock);
	errno = errno2;
	return -1;
}

int zsocket_fifo_listen(const char *path, int unlinked, mode_t mode)
{
	char buf[100];
	struct stat st;
	int fd;
	int count;
	int errno2;

	fd = -1;
	if (unlinked) {
		if (unlink(path) && errno != ENOENT) {
			goto err;
		}
		if (mkfifo(path, (mode ? mode : 0622)) < 0) {
			goto err;
		}
	}
	if ((fd = open(path, O_RDWR | O_NONBLOCK, 0)) < 0) {
		goto err;
	}

	if (fstat(fd, &st) < 0) {
		goto err;
	}

	if (S_ISFIFO(st.st_mode) == 0) {
		goto err;
	}

	if (fchmod(fd, (mode ? mode : 0622)) < 0) {
		goto err;
	}

	while ((count = zio_peek(fd)) > 0 && read(fd, buf, 100 < count ? 100 : count) > 0) ;

	return (fd);

      err:
	errno2 = errno;
	if (fd != -1) {
		close(fd);
	}
	errno = errno2;
	return -1;
}

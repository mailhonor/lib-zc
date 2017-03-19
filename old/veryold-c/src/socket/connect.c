#include "zc.h"

int zsocket_sane_connect(int sock, struct sockaddr *sa, socklen_t len)
{
	if (sa->sa_family == AF_INET) {
		int on = 1;
		(void)setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char *)&on, sizeof(on));
	}

	return (connect(sock, sa, len));
}

int zsocket_timed_connect(int sock, struct sockaddr *sa, int len, int timeout)
{
	int error;
	socklen_t error_len;

	if (timeout > 0) {
		if (zsocket_sane_connect(sock, sa, len) == 0)
			return (0);
		if (errno != EINPROGRESS)
			return (-1);
		if (zio_write_wait(sock, timeout) < 0)
			return (-1);

		error = 0;
		error_len = sizeof(error);
		if (getsockopt(sock, SOL_SOCKET, SO_ERROR, (char *)&error, &error_len) < 0)
			return (-1);
		if (error) {
			errno = error;
			return (-1);
		}
	} else {
		if (zsocket_sane_connect(sock, sa, len) < 0 && errno != EINPROGRESS) {
			return (-1);
		}
	}

	return (0);
}

int zsocket_unix_connect(const char *addr, int timeout)
{
	struct sockaddr_un sun;
	int len = strlen(addr);
	int sock;
	int errno2;

	if (len >= (int)sizeof(sun.sun_path)) {
		errno = ENAMETOOLONG;
		return -1;
	}

	memset((char *)&sun, 0, sizeof(sun));
	sun.sun_family = AF_UNIX;
	memcpy(sun.sun_path, addr, len + 1);

	if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		return (-1);
	}

	zio_nonblocking(sock, 1);

	if (zsocket_timed_connect(sock, (struct sockaddr *)&sun, sizeof(sun), timeout) < 0) {
		errno2 = errno;
		close(sock);
		errno = errno2;
		return (-1);
	}

	return (sock);
}

int zsocket_inet_connect(char *dip, int port, int timeout)
{
	int sock;
	struct sockaddr_in addr;
	int errno2;

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return (-1);

	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(dip);

	zio_nonblocking(sock, 1);

	if (zsocket_timed_connect(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in), timeout) < 0) {
		errno2 = errno;
		close(sock);
		errno = errno2;
		return (-1);
	}

	return (sock);
}

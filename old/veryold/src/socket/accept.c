#include "zc.h"

int zsocket_sane_accept(int sock, struct sockaddr *sa, socklen_t * len)
{
	static int accept_ok_errors[] = {
		EAGAIN,
		ECONNREFUSED,
		ECONNRESET,
		EHOSTDOWN,
		EHOSTUNREACH,
		EINTR,
		ENETDOWN,
		ENETUNREACH,
		ENOTCONN,
		EWOULDBLOCK,
		ENOBUFS,	/* HPUX11 */
		ECONNABORTED,
		0,
	};
	int count;
	int err;
	int fd;

	if ((fd = accept(sock, sa, len)) < 0) {
		for (count = 0; (err = accept_ok_errors[count]) != 0; count++) {
			if (errno == err) {
				errno = EAGAIN;
				break;
			}
		}
	} else if (sa && (sa->sa_family == AF_INET || sa->sa_family == AF_INET6)) {
		int on = 1;
		(void)setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char *)&on, sizeof(on));
	}

	return (fd);
}

int zsocket_unix_accept(int fd)
{
	return (zsocket_sane_accept(fd, (struct sockaddr *)0, (socklen_t *) 0));
}

int zsocket_inet_accept(int fd)
{
	struct sockaddr_storage ss;
	socklen_t ss_len = sizeof(ss);

	return (zsocket_sane_accept(fd, (struct sockaddr *)&ss, &ss_len));
}

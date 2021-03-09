#include "zyc.h"

/* connect */
int zsocket_sane_connect(int sock, struct sockaddr * sa, socklen_t len) {
	if (sa->sa_family == AF_INET) {
		int     on = 1;
		(void) setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char *) &on, sizeof(on));
	}
	return (connect(sock, sa, len));
}
int zsocket_timed_connect(int sock, struct sockaddr * sa, int len, int timeout)
{
	int error;
	socklen_t error_len;

	if (zsocket_sane_connect(sock, sa, len) == 0)
		return (0);
	if (errno != EINPROGRESS)
		return (-1);
	if (zio_write_wait(sock, timeout) < 0)
		return (-1);

	error = 0;
	error_len = sizeof(error);
	if (getsockopt(sock, SOL_SOCKET, SO_ERROR, (char *) &error, &error_len) < 0)
		return (-1);
	if (error) {
		errno = error;
		return (-1);
	}
	return (0);
}

static int zsocket_connect_2(int sock, struct sockaddr * sa, int len, int timeout){
	if (timeout > 0) {
		zio_set_nonblock(sock);
		if (zsocket_timed_connect(sock, sa, len, timeout) < 0) {
			close(sock);
			return (-1);
		}
		return (sock);
	} else {
		zio_set_block(sock);
		if (zsocket_sane_connect(sock, sa, len) < 0 && errno != EINPROGRESS) {
			close(sock);
			return (-1);
		}
		return (sock);
	}
}

int zsocket_unix_connect(const char *addr, int timeout) {
	struct sockaddr_un sun;
	int    len = strlen(addr);
	int    sock;

	if (len >= (int) sizeof(sun.sun_path))
		zmsg_fatal("unix-domain name too long: %s", addr);
	memset((char *) &sun, 0, sizeof(sun));
	sun.sun_family = AF_UNIX;
	memcpy(sun.sun_path, addr, len + 1);

	if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
		return (-1);
	return zsocket_connect_2(sock, (struct sockaddr *)&sun, sizeof(sun), timeout);
}

int zsocket_inet_connect(char *dip, int port, int timeout)
{
	int sock;
	struct sockaddr_in addr;

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return (-1);

	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family=AF_INET;
	addr.sin_port=htons(port);
	addr.sin_addr.s_addr=inet_addr(dip);

	return zsocket_connect_2(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in), timeout);
}

/* listen */
int zsocket_unix_listen(const char *addr, int backlog)
{
	struct sockaddr_un sun;
	int     len = strlen(addr);
	int     sock, on;

	if (len >= (int) sizeof(sun.sun_path))
		zmsg_fatal("unix-domain name too long: %s", addr);
	memset((char *) &sun, 0, sizeof(sun));
	sun.sun_family = AF_UNIX;
	memcpy(sun.sun_path, addr, len + 1);
	if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
		zmsg_fatal("socket: %m");

	if (unlink(addr) < 0 && errno != ENOENT)
		zmsg_fatal("remove %s: %m", addr);

	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on)) < 0)
		zmsg_fatal("setsockopt(SO_REUSEADDR): %m");

	if (bind(sock, (struct sockaddr *) & sun, sizeof(sun)) < 0)
		zmsg_fatal("bind: %s: %m", addr);

	if (fchmod(sock, 0666) < 0)
		zmsg_fatal("fchmod socket %s: %m", addr);

	zio_set_nonblock(sock);
	if (listen(sock, backlog) < 0)
		zmsg_fatal("listen: %m");
	return (sock);
}

int zsocket_inet_listen(char *sip, int port, int backlog)
{
	int sock;
	int on=1;
	struct sockaddr_in addr;

	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family=AF_INET;
	addr.sin_port=htons(port);
	addr.sin_addr.s_addr=(Z_STREMPTY(sip)?INADDR_ANY:inet_addr(sip));

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		zmsg_fatal("socket: %m");
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on)) < 0)
		zmsg_fatal("setsockopt(SO_REUSEADDR): %m");
	if (bind(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0) {
		zmsg_fatal("bind %s port %s: %m", sip, port);
	}
	zio_set_nonblock(sock);
	if (listen(sock, backlog) < 0)
		zmsg_fatal("listen: %m");
	return (sock);
}

/* accept */
int zsocket_sane_accept(int sock, struct sockaddr * sa, socklen_t *len) {
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
		ENOBUFS,			/* HPUX11 */
		ECONNABORTED,
		0,
	};
	int     count;
	int     err;
	int     fd;

	if ((fd = accept(sock, sa, len)) < 0) {
		for (count = 0; (err = accept_ok_errors[count]) != 0; count++) {
			if (errno == err) {
				errno = EAGAIN;
				break;
			}
		}
	} else if (sa && (sa->sa_family == AF_INET || sa->sa_family == AF_INET6)) {
		int     on = 1;
		(void) setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char *) &on, sizeof(on));
	}
	return (fd);
}

int zsocket_unix_accept(int fd) {
	return (zsocket_sane_accept(fd, (struct sockaddr *) 0, (socklen_t *) 0));
}

int zsocket_inet_accept(int fd) {
	struct sockaddr_storage ss;
	socklen_t ss_len = sizeof(ss);

	return (zsocket_sane_accept(fd, (struct sockaddr *) & ss, &ss_len));
}

int zsocket_inet_connect_host(char *host, int port, int timeout){
	ZARGV *iplist;
	char *ip;
	int i,sock;

	iplist=zaddr_get_addr(host);
	if(iplist==0 || ZARGV_LEN(iplist) == 0){
		return -1;
	}
	for(i=0;i<ZARGV_LEN(iplist);i++){
		ip=iplist->argv[i];
		sock=zsocket_inet_connect(ip, port, timeout);
		if(sock<1){
			return -2;
		}
		return sock;
	}
	return -2;
}

int zsocket_fifo_listen(const char *path){
	char    buf[100];
	struct stat st;
	int     fd;
	int     count;

	if (unlink(path) && errno != ENOENT)
		zmsg_fatal("zsocket_fifo_listen: remove %s: %m", path);
	if (mkfifo(path, 0622) < 0)
		zmsg_fatal("zsocket_fifo_listen: create fifo %s: %m", path);
	if ((fd = open(path, O_RDWR | O_NONBLOCK, 0)) < 0)
		zmsg_fatal("zsocket_fifo_listen: open %s: %m", path);

	if (fstat(fd, &st) < 0)
		zmsg_fatal("zsocket_fifo_listen: fstat %s: %m", path);
	if (S_ISFIFO(st.st_mode) == 0)
		zmsg_fatal("zsocket_fifo_listen: not a fifo: %s", path);
	if (fchmod(fd, 0622) < 0)
		zmsg_fatal("zsocket_fifo_listen: fchmod %s: %m", path);
	zio_set_nonblock(fd);
	while ((count = zio_peek(fd)) > 0 && read(fd, buf, 100 < count ? 100 : count) > 0);
	return (fd);
}

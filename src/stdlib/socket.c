/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-10-20
 * ================================
 */

#include "libzc.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>

/* ################################################################## */
/* accept */

static int ___sane_accept(int sock, struct sockaddr *sa, socklen_t * len)
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
        ENOBUFS,                /* HPUX11 */
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

int zunix_accept(int fd)
{
    return (___sane_accept(fd, (struct sockaddr *)0, (socklen_t *) 0));
}

int zinet_accept(int fd)
{
    struct sockaddr_storage ss;
    socklen_t ss_len = sizeof(ss);

    return (___sane_accept(fd, (struct sockaddr *)&ss, &ss_len));
}

/* ################################################################## */
/* listen */

int zunix_listen(char *addr, int backlog)
{
    struct sockaddr_un sun;
    int len = strlen(addr);
    int sock = -1;
    int errno2;

    if (len >= (int)sizeof(sun.sun_path)) {
        errno = ENAMETOOLONG;
        return -1;
    }

    memset((char *)&sun, 0, sizeof(struct sockaddr_un));
    sun.sun_family = AF_UNIX;
    memcpy(sun.sun_path, addr, len + 1);

    if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        return -1;
    }

    if (unlink(addr) < 0 && errno != ENOENT) {
        zfatal("remove: %m");
    }

    if (bind(sock, (struct sockaddr *)&sun, sizeof(struct sockaddr_un)) < 0) {
        goto err;
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

int zinet_listen(char *sip, int port, int backlog)
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

int zlisten(char *netpath, int backlog)
{
    char _netpath[1024];
    char *p;
    int port;
    int fd;

    fd = 0;
    strncpy(_netpath, netpath, 1000);

    p = strchr(_netpath, ':');
    if (p) {
        *p = 0;
        port = atoi(p + 1);
        fd = zinet_listen(_netpath, port, backlog);
    } else {
        fd = zunix_listen(_netpath, backlog);
    }

    return fd;
}

/* ################################################################## */
/* fifo listen */

int zfifo_listen(char *path)
{
    int fd;
    int errno2;

    fd = -1;
    if ((mkfifo(path, 0666) < 0) && (errno != EEXIST)) {
        goto err;
    }
    if ((fd = open(path, O_RDWR | O_NONBLOCK, 0)) < 0) {
        goto err;
    }

    return (fd);

  err:
    errno2 = errno;
    if (fd != -1) {
        close(fd);
    }
    errno = errno2;

    return -1;
}

/* ################################################################## */
/* connect */

static int ___sane_connect(int sock, struct sockaddr *sa, socklen_t len)
{
    if (sa->sa_family == AF_INET) {
        int on = 1;
        (void)setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char *)&on, sizeof(on));
    }

    return (connect(sock, sa, len));
}

static int ___timed_connect(int sock, struct sockaddr *sa, int len, int timeout)
{
    int error;
    socklen_t error_len;

    if (timeout > 0) {
        znonblocking(sock, 1);
        if (___sane_connect(sock, sa, len) == 0)
            return (0);
        if (errno != EINPROGRESS)
            return (-1);
        if (zwrite_wait(sock, timeout) < 0)
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
#if 0
        if (___sane_connect(sock, sa, len) < 0 && errno != EINPROGRESS) {
            return (-1);
        }
#endif
        if (___sane_connect(sock, sa, len) < 0) {
            return (-1);
        }
        znonblocking(sock, 1);
    }

    return (0);
}

int zunix_connect(char *addr, int timeout)
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

    if (___timed_connect(sock, (struct sockaddr *)&sun, sizeof(sun), timeout) < 0) {
        errno2 = errno;
        close(sock);
        errno = errno2;
        return (-1);
    }

    return (sock);
}

int zinet_connect(char *dip, int port, int timeout)
{
    int sock;
    struct sockaddr_in addr;
    int errno2;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return (-1);
    }

    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(dip);

    if (___timed_connect(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in), timeout) < 0) {
        errno2 = errno;
        close(sock);
        errno = errno2;
        return (-1);
    }

    return (sock);
}

int zhost_connect(char *host, int port, int timeout)
{
    zaddr_t ip_list[8];
    int sock, count, i;

    count = zgetaddr(host, ip_list, 8);
    if (count < 1) {
        return -1;
    }
    for (i = 0; i < count; i++) {
        sock = zinet_connect(ip_list[i].addr, port, timeout);
        if (sock < 1) {
            return -2;
        }
        return sock;
    }

    return -2;
}

int zconnect(char *netpath, int timeout)
{
    char _netpath[1024];
    char *p;
    int port;
    int fd;

    fd = 0;
    strncpy(_netpath, netpath, 1000);

    p = strchr(_netpath, ':');
    if (p) {
        *p = 0;
        port = atoi(p + 1);
        fd = zhost_connect(_netpath, port, timeout);
    } else {
        fd = zunix_connect(_netpath, timeout);
    }

    return fd;
}

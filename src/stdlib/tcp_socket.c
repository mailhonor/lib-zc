/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-10-20
 * ================================
 */

#include "zc.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#ifdef _WIN32
#include <ws2tcpip.h>
#include <pthread.h>
#else // _WIN32
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#endif // _WIN32

/* accept */
static int sane_accept(int sock, struct sockaddr *sa, socklen_t *len)
{
    zWSAStartup();
    static int accept_ok_errors[] = {
#ifdef _WIN32
        WSAECONNREFUSED,
        WSAECONNRESET,
        WSAEHOSTDOWN,
        WSAEHOSTUNREACH,
        WSAENETDOWN,
        WSAENETUNREACH,
        WSAENOTCONN,
        WSAEWOULDBLOCK,
        WSAENOBUFS, /* HPUX11 */
        WSAECONNABORTED,
#else // _WIN32
        EAGAIN,
        ECONNREFUSED,
        ECONNRESET,
        EHOSTDOWN,
        EHOSTUNREACH,
        ENETDOWN,
        ENETUNREACH,
        ENOTCONN,
        EWOULDBLOCK,
        ENOBUFS, /* HPUX11 */
        ECONNABORTED,
#endif // _WIN32
        0,
    };
    int count;
    int err;
    int fd = -1;
    int errno2;

    while (1)
    {
        if ((fd = accept(sock, sa, len)) < 0)
        {
            errno2 = zget_errno();
            if (errno2 == EINTR)
            {
                if (zvar_sigint_flag)
                {
                    errno = EINVAL;
                    break;
                }
                continue;
            }
            for (count = 0; (err = accept_ok_errors[count]) != 0; count++)
            {
                if (errno2 == err)
                {
                    errno = EAGAIN;
                    break;
                }
            }
        }
        else if (sa && (sa->sa_family == AF_INET || sa->sa_family == AF_INET6))
        {
            int on = 1;
            (void)setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char *)&on, sizeof(on));
        }
        break;
    }

    return (fd);
}

int zunix_accept(int fd)
{
    return (sane_accept(fd, (struct sockaddr *)0, (socklen_t *)0));
}

int zinet_accept(int fd)
{
    struct sockaddr_storage ss;
    socklen_t ss_len = sizeof(ss);
    return (sane_accept(fd, (struct sockaddr *)&ss, &ss_len));
}

int zaccept(int sock, int type)
{
    int fd = -1;
    if (type == zvar_tcp_listen_type_inet)
    {
        fd = zinet_accept(sock);
    }
    else if (type == zvar_tcp_listen_type_unix)
    {
        fd = zunix_accept(sock);
    }
    else
    {
        zfatal("accept only support inet/unix");
    }
    return fd;
}

/* listen */
int zunix_listen(char *addr, int backlog)
{
#ifdef __linux__
    struct sockaddr_un sun;
    int len = strlen(addr);
    int sock = -1;
    int errno2;

    if (len >= (int)sizeof(sun.sun_path))
    {
        errno = ENAMETOOLONG;
        return -1;
    }

    memset((char *)&sun, 0, sizeof(struct sockaddr_un));
    sun.sun_family = AF_UNIX;
    memcpy(sun.sun_path, addr, len + 1);

    if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
    {
        return -1;
    }

    if (unlink(addr) < 0 && errno != ENOENT)
    {
        zfatal("unlink: %s(%m)", addr);
    }

    if (bind(sock, (struct sockaddr *)&sun, sizeof(struct sockaddr_un)) < 0)
    {
        goto err;
    }

    if (listen(sock, backlog) < 0)
    {
        goto err;
    }

    return (sock);

err:
    errno2 = errno;
    if (sock > -1)
    {
        zclose(sock);
    }
    errno = errno2;
#endif // __linux__

    return -1;
}

int zinet_listen(const char *sip, int port, int backlog)
{
    int sock;
    int on = 1;
    struct sockaddr_in addr;
    int errno2;

    zWSAStartup();

    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = (zempty(sip) ? INADDR_ANY : inet_addr(sip));

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
#ifdef _WIN32
        errno = zget_errno();
#endif // _WIN32
        return -1;
    }

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0)
    {
        goto err;
    }

#if 0
    struct linger linger;
    linger.l_onoff = 0;
    linger.l_linger = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_LINGER, (char *)&linger, sizeof(linger)) < 0) {
        goto err;
    }
#endif

    if (bind(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0)
    {
        goto err;
    }

    if (listen(sock, backlog) < 0)
    {
        goto err;
    }

    return (sock);

err:
    errno2 = zget_errno();
    zclosesocket(sock);
    errno == errno2;

    return -1;
}

int zlisten(const char *netpath, int *type, int backlog)
{
    zWSAStartup();
    char _netpath[1024], *path, *host, *p;
    int fd = -1, port, tp;

    strncpy(_netpath, netpath, 1000);
    _netpath[1000] = 0;
    if (*_netpath == 0)
    {
        errno = EACCES;
        return -1;
    }
    p = strchr(_netpath, ':');
    if (p)
    {
        *p++ = 0;
        if (!strcmp(_netpath, "local"))
        {
            tp = zvar_tcp_listen_type_unix;
            path = p;
        }
        else if (!strcmp(_netpath, "fifo"))
        {
            tp = zvar_tcp_listen_type_fifo;
            path = p;
        }
        else
        {
            tp = zvar_tcp_listen_type_inet;
            host = _netpath;
            port = atoi(p);
        }
    }
    else
    {
        tp = zvar_tcp_listen_type_unix;
        path = _netpath;
    }
    if ((tp == zvar_tcp_listen_type_inet) && (port < 1))
    {
        errno = EINVAL;
        return -1;
    }
    if (tp == zvar_tcp_listen_type_inet)
    {
        fd = zinet_listen(host, port, backlog);
    }
    else if (tp == zvar_tcp_listen_type_unix)
    {
        fd = zunix_listen(path, backlog);
    }
    else if (tp == zvar_tcp_listen_type_fifo)
    {
        fd = zfifo_listen(path);
    }
    if (type)
    {
        *type = tp;
    }

    return fd;
}

/* fifo listen */
int zfifo_listen(const char *path)
{
#ifdef __linux__
    int fd;
    int errno2;

    fd = -1;
    if ((mkfifo(path, 0666) < 0) && (errno != EEXIST))
    {
        goto err;
    }
    if ((fd = open(path, O_RDWR | O_NONBLOCK, 0)) < 0)
    {
        goto err;
    }

    return (fd);

err:
    errno2 = errno;
    if (fd != -1)
    {
        zclose(fd);
    }
    errno = errno2;
#endif // __linux__
    return -1;
}

/* connect */

/* 非阻塞 connect 过程 :
 * 1, 设置 socket_fd 非阻塞
 * 2, 执行 connnect
 *    2.1 返回0, 表示成功(一般是本机)
 *    2.2 返回-1,
 *       2.2.1 如果errno == EINPROGRESS, 表示成功
 *       2.2.2 否则失败
 * 3, 等待socket读写状态
 *    3.1 可写(当连接成功后，socket_fd 就会处于可写状态，此时表示连接成功)
 *    3.2 可读可写
 *       3.2.1 连接成功, 且对方发送了数据
 *       3.2.2 连接失败
 *       3.3.3 检测方法: 再次执行connect，然后查看error是否等于EISCONN(表示已经连接到该套接字)
 *    3.3 错误
 */

static int sane_connect(int sock, struct sockaddr *sa, int len)
{
    if (sa->sa_family == AF_INET)
    {
        int on = 1;
        (void)setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char *)&on, sizeof(on));
    }
    while (1)
    {
        if (connect(sock, sa, len) == 0)
        {
            break;
        }
        int ec = zget_errno();
        if (ec == EINTR)
        {
            continue;
        }
        if ((ec == EINPROGRESS) || (ec == EWOULDBLOCK))
        {
            break;
        }
#ifdef _WIN32
        errno = ec;
#endif // _WIN32
        return -1;
    }
    return (0);
}

static int connect_and_wait_ok(int sock, struct sockaddr *sa, int len, int timeout)
{
    int ret = sane_connect(sock, sa, len);
    if (ret < 0)
    {
        return ret;
    }

    int readable = 0, writeable = 0;
    ret = ztimed_read_write_wait(sock, timeout, &readable, &writeable);
    if (ret < 1)
    {
        return -1;
    }
    if (writeable == 0)
    {
        return -1;
    }
    if (readable == 0)
    {
        return 0;
    }

    // char buf[1];
    // if (read(sock, buf, 0) == 0) {
    //     return 0;
    // }

    ret = connect(sock, sa, len);
    if (ret == 0)
    {
        return -1;
    }
    int ec = zget_errno();
    switch (ec)
    {
    case EISCONN:
        return 0;
        break;
#if 0
    case EALREADY:
#ifdef _WIN32
        errno = ec;
#endif // _WIN32
        return -1;
        break;
    case EINPROGRESS:
#ifdef _WIN32
        errno = ec;
#endif // _WIN32
        return -1;
        break;
#endif
    default:
#ifdef _WIN32
        errno = ec;
#endif // _WIN32
        return -1;
        break;
    }
#ifdef _WIN32
        errno = ec;
#endif // _WIN32
    return -1;
}

int zunix_connect(const char *addr, int timeout)
{
    int sock = -1;
#ifdef __linux__
    struct sockaddr_un sun;
    int len = strlen(addr);
    int errno2;

    if (len >= (int)sizeof(sun.sun_path))
    {
        errno = ENAMETOOLONG;
        return -1;
    }

    if (timeout < 0)
    {
        timeout = zvar_max_timeout;
    }

    memset((char *)&sun, 0, sizeof(sun));
    sun.sun_family = AF_UNIX;
    memcpy(sun.sun_path, addr, len + 1);

    if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
    {
        return (-1);
    }

    if (timeout > 0)
    {
        znonblocking(sock, 1);
        if (connect_and_wait_ok(sock, (struct sockaddr *)&sun, sizeof(sun), timeout) < 0)
        {
            errno2 = errno;
            zclose(sock);
            errno = errno2;
            return (-1);
        }
        znonblocking(sock, 0);
    }
    else
    {
        if (sane_connect(sock, (struct sockaddr *)&sun, sizeof(sun)) < 0)
        {
            errno2 = errno;
            zclose(sock);
            errno = errno2;
            return (-1);
        }
    }
#endif // __linux__
    return (sock);
}

int zinet_connect(const char *dip, int port, int timeout)
{
    int sock;
    struct sockaddr_in addr;
    int errno2;

    if (timeout < 0)
    {
        timeout = zvar_max_timeout;
    }

    zWSAStartup();

    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
#ifdef _WIN32
        errno = zget_errno();
#endif // _WIN32
        return (-1);
    }

    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(dip);

    if (timeout > 0)
    {
        znonblocking(sock, 1);
        if (connect_and_wait_ok(sock, (struct sockaddr *)&addr, sizeof(addr), timeout) < 0)
        {
            errno2 = errno;
            zclosesocket(sock);
            errno = errno2;
            return (-1);
        }
        znonblocking(sock, 0);
    }
    else
    {
        if (sane_connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        {
            errno2 = errno;
            zclosesocket(sock);
            errno = errno2;
            return (-1);
        }
    }

    return (sock);
}

int zhost_connect(const char *host, int port, int timeout)
{
    int sock = -1;

    if (zis_ip(host))
    {
        sock = zinet_connect(host, port, timeout);
        return sock;
    }

    zargv_t *addrs = zargv_create(0);

    zget_hostaddr(host, addrs);
    ZARGV_WALK_BEGIN(addrs, ip)
    {
        sock = zinet_connect(ip, port, timeout);
        if (sock > -1)
        {
            break;
        }
    }
    ZARGV_WALK_END;
    zargv_free(addrs);

    return sock;
}

static int ___zconnect_offset_idx = 0;
int zconnect(const char *netpath, int timeout)
{
    int fd = -1, count, port, i, offset = ___zconnect_offset_idx++;
    char *np, *p;
    zargv_t *ns = zargv_create(10);
    zargv_split_append(ns, netpath, " \t,;");
    count = ns->argc;

    for (i = 0; i < count; i++)
    {
        np = (ns->argv)[(i + offset) % count];
        p = strchr(np, ':');
        if (p)
        {
            *p = 0;
            port = atoi(p + 1);
            fd = zhost_connect(np, port, timeout);
        }
        else
        {
            fd = zunix_connect(np, timeout);
        }
        if (fd > -1)
        {
            break;
        }
    }
    zargv_free(ns);

    return fd;
}

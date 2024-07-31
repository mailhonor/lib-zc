/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-10-20
 * ================================
 */

// 非阻塞 connect 过程 :
//  1, 设置 socket_fd 非阻塞
//  2, 执行 connnect
//     2.1 返回0, 表示成功(一般是本机)
//     2.2 返回-1,
//        2.2.1 如果errno == ZCC_EINPROGRESS, 表示成功
//        2.2.2 否则失败
//  3, 等待socket读写状态
//     3.1 可写(当连接成功后，socket_fd 就会处于可写状态，此时表示连接成功)
//     3.2 可读可写
//        3.2.1 连接成功, 且对方发送了数据
//        3.2.2 连接失败
//        3.3.3 检测方法: 再次执行connect，然后查看error是否等于EISCONN(表示已经连接到该套接字)
//     3.3 错误

#include "zcc/zcc_errno.h"
#include <mutex>
#ifdef _WIN64
#include <WinSock2.h>
#include <ws2tcpip.h>
#else // _WIN64
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#endif // _WIN64

zcc_namespace_begin;

#ifdef _WIN64
int WSAStartup()
{
    static int err = 1;
    static std::mutex locker;
    static int _init_flag = 0;
    if (_init_flag == 0)
    {
        locker.lock();
        if (_init_flag == 0)
        {
            WSADATA wsaData = {};
            err = ::WSAStartup(MAKEWORD(2, 2), &wsaData);
            if (err)
            {
                err = -1;
            }
            else
            {
                err = 1;
            }
            _init_flag = 1;
        }
        locker.unlock();
    }
    return err;
}
#else  // _WIN64
int WSAStartup()
{
    return 0;
}
#endif // _WIN64

int close_socket(int fd)
{
    int r;
#ifdef _WIN64
    r = ::closesocket(fd);
#else  // _WIN64
    r = ::close(fd);
#endif // _WIN64
    return r;
}

// accept
static int sane_accept(int sock, struct sockaddr *sa, socklen_t *len)
{
    WSAStartup();
    static int accept_ok_errors[] = {
#ifdef _WIN64
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
#else  // _WIN64
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
#endif // _WIN64
        0,
    };
    int count;
    int err;
    int fd = -1;
    int errno2;

    while (1)
    {
        if ((fd = ::accept(sock, sa, len)) < 0)
        {
            errno2 = get_errno();
            if (errno2 == ZCC_EINTR)
            {
                if (var_sigint_flag)
                {
                    set_errno(ZCC_EINVAL);
                    break;
                }
                continue;
            }
            for (count = 0; (err = accept_ok_errors[count]) != 0; count++)
            {
                if (errno2 == err)
                {
                    set_errno(ZCC_EAGAIN);
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

int unix_accept(int fd)
{
    return (sane_accept(fd, (struct sockaddr *)0, (socklen_t *)0));
}

int inet_accept(int fd)
{
    struct sockaddr_storage ss;
    socklen_t ss_len = sizeof(ss);
    return (sane_accept(fd, (struct sockaddr *)&ss, &ss_len));
}

int socket_accept(int fd, int type)
{
    if (type == var_tcp_listen_type_inet)
    {
        return inet_accept(fd);
    }
    else if (type == var_tcp_listen_type_unix)
    {
        return unix_accept(fd);
    }
    else /* if (type == var_tcp_listen_type_fifo) */
    {
        return -1;
    }
}

// listen
int unix_listen(char *addr, int backlog)
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
        zcc_fatal("unlink: %s(%m)", addr);
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
        close_socket(sock);
    }
    errno = errno2;
#endif // __linux__

    return -1;
}

int inet_listen(const char *sip, int port, int backlog)
{
    int sock;
    int on = 1;
    struct sockaddr_in addr;
    int errno2;

    WSAStartup();

    std::memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = (empty(sip) ? INADDR_ANY : inet_addr(sip));

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
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
    errno2 = get_errno();
    close_socket(sock);
    set_errno(errno2);

    return -1;
}

int netpath_listen(const char *netpath, int backlog, int *type)
{
    WSAStartup();
    char _netpath[1024], *path, *host, *p;
    int fd = -1, port, tp;

    std::strncpy(_netpath, netpath, 1000);
    _netpath[1000] = 0;
    if (*_netpath == 0)
    {
        set_errno(ZCC_EACCES);
        return -1;
    }
    p = std::strchr(_netpath, ':');
    if ((!p) || ((_netpath - p == 1) && (_netpath[0] != '0')))
    {
        tp = var_tcp_listen_type_unix;
        path = _netpath;
    }
    else
    {
        *p++ = 0;
        if (!std::strcmp(_netpath, "local"))
        {
            tp = var_tcp_listen_type_unix;
            path = p;
        }
        else if (!strcmp(_netpath, "fifo"))
        {
            tp = var_tcp_listen_type_fifo;
            path = p;
        }
        else
        {
            tp = var_tcp_listen_type_inet;
            host = _netpath;
            port = std::atoi(p);
        }
    }

    if (type)
    {
        *type = tp;
    }

    if ((tp == var_tcp_listen_type_inet) && (port < 1))
    {
        errno = EINVAL;
        return -1;
    }
    if (tp == var_tcp_listen_type_inet)
    {
        fd = inet_listen(host, port, backlog);
    }
    else if (tp == var_tcp_listen_type_unix)
    {
        fd = unix_listen(path, backlog);
    }
    else if (tp == var_tcp_listen_type_fifo)
    {
        fd = fifo_listen(path);
    }

    return fd;
}

// fifo listen
int fifo_listen(const char *path)
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
        close_socket(fd);
    }
    errno = errno2;
#endif // __linux__
    return -1;
}

// connect

static int sane_connect(int sock, struct sockaddr *sa, int len)
{
    if (sa->sa_family == AF_INET)
    {
        int on = 1;
        (void)::setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char *)&on, sizeof(on));
    }
    while (1)
    {
        if (::connect(sock, sa, len) == 0)
        {
            break;
        }
        int ec = get_errno();
        if (ec == ZCC_EINTR)
        {
            continue;
        }
        if ((ec == ZCC_EINPROGRESS) || (ec == ZCC_EWOULDBLOCK))
        {
            break;
        }
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
    ret = timed_read_write_wait(sock, timeout, &readable, &writeable);
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

    ret = ::connect(sock, sa, len);
    if (ret == 0)
    {
        return -1;
    }
    int ec = get_errno();
    switch (ec)
    {
    case ZCC_EISCONN:
        return 0;
        break;
#if 0
    case EALREADY:
        return -1;
        break;
    case EINPROGRESS:
        return -1;
        break;
#endif
    default:
        return -1;
        break;
    }
    return -1;
}

int unix_connect(const char *addr, int timeout)
{
    int sock = -1;
#ifdef __linux__
    struct sockaddr_un sun;
    int len = std::strlen(addr);
    int errno2;

    if (len >= (int)sizeof(sun.sun_path))
    {
        set_errno(ZCC_ENAMETOOLONG);
        return -1;
    }

    if (timeout < 0)
    {
        timeout = var_io_max_timeout;
    }

    std::memset((char *)&sun, 0, sizeof(sun));
    sun.sun_family = AF_UNIX;
    std::memcpy(sun.sun_path, addr, len + 1);

    if ((sock = ::socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
    {
        return (-1);
    }

    if (timeout > 0)
    {
        nonblocking(sock, true);
        if (connect_and_wait_ok(sock, (struct sockaddr *)&sun, sizeof(sun), timeout) < 0)
        {
            errno2 = get_errno();
            ::close(sock);
            set_errno(errno2);
            return (-1);
        }
        nonblocking(sock, false);
    }
    else
    {
        if (sane_connect(sock, (struct sockaddr *)&sun, sizeof(sun)) < 0)
        {
            errno2 = get_errno();
            close(sock);
            set_errno(errno2);
            return (-1);
        }
    }
#endif // __linux__
    return (sock);
}

int inet_connect(const char *dip, int port, int timeout)
{
    int sock;
    struct sockaddr_in addr;
    int errno2;

    if (timeout < 0)
    {
        timeout = var_io_max_timeout;
    }

    WSAStartup();

    if ((sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        return (-1);
    }

    std::memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(dip);

    if (timeout > 0)
    {
        nonblocking(sock, true);
        if (connect_and_wait_ok(sock, (struct sockaddr *)&addr, sizeof(addr), timeout) < 0)
        {
            errno2 = get_errno();
            close_socket(sock);
            set_errno(errno2);
            return (-1);
        }
        nonblocking(sock, false);
    }
    else
    {
        if (sane_connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        {
            errno2 = get_errno();
            close_socket(sock);
            set_errno(errno2);
            return (-1);
        }
    }

    return (sock);
}

int host_connect(const char *host, int port, int timeout)
{
    int sock = -1;

    if (is_ip(host))
    {
        sock = inet_connect(host, port, timeout);
        return sock;
    }

    std::vector<std::string> ips;
    get_hostaddr(host, ips);
    for (auto it = ips.begin(); it != ips.end(); it++)
    {
        const char *ip = it->c_str();
        sock = inet_connect(ip, port, timeout);
        if (sock > -1)
        {
            break;
        }
    }
    return sock;
}

int netpath_connect(const char *netpath, int timeout)
{
    static int64_t _offset = 0;
    int sock = -1;
    std::vector<std::string> hs = split(netpath, ";, \t\r\n");
    int64_t length = hs.size();
    int64_t offset = _offset++;
    for (int64_t i = 0; i < length; i++)
    {
        std::string &path = hs[(offset + i) % length];
        if (path.empty())
        {
            continue;
        }
        auto pos = path.find(':');
        if (pos == std::string::npos)
        {
            sock = unix_connect(path.c_str(), timeout);
        }
        else
        {
            std::string host = path.substr(0, pos);
            int port = std::atoi(path.c_str() + pos + 1);
            sock = host_connect(host.c_str(), port, timeout);
        }
        if (sock > -1)
        {
            break;
        }
    }
    return sock;
}

int get_peername(int sockfd, int *host, int *port)
{
    WSAStartup();
    struct sockaddr_in sa;
    socklen_t sa_length = sizeof(struct sockaddr);

    if (getpeername(sockfd, (struct sockaddr *)&sa, &sa_length) < 0)
    {
        return 0;
    }

    if (host)
    {
        *host = *((int *)&(sa.sin_addr));
    }

    if (port)
    {
        *port = ntohs(sa.sin_port);
    }

    return 1;
}

zcc_namespace_end;

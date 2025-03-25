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

/**
 * @brief 初始化 Windows 套接字库（仅在 Windows 系统下有效）
 * 
 * 该函数使用静态变量确保 Windows 套接字库只初始化一次。
 * @return 初始化成功返回 1，失败返回 -1，非 Windows 系统返回 0。
 */
#ifdef _WIN64
int WSAStartup()
{
    // 静态变量，记录初始化结果
    static int err = 1;
    // 静态互斥锁，用于线程安全
    static std::mutex locker;
    // 静态标志位，记录是否已经初始化
    static int _init_flag = 0;
    if (_init_flag == 0)
    {
        // 加锁保证线程安全
        locker.lock();
        if (_init_flag == 0)
        {
            // 初始化 WSADATA 结构体
            WSADATA wsaData = {};
            // 调用 Windows 套接字初始化函数
            err = ::WSAStartup(MAKEWORD(2, 2), &wsaData);
            if (err)
            {
                // 初始化失败
                err = -1;
            }
            else
            {
                // 初始化成功
                err = 1;
            }
            // 标记已经初始化
            _init_flag = 1;
        }
        // 解锁
        locker.unlock();
    }
    return err;
}
#else  // _WIN64
/**
 * @brief 非 Windows 系统下的 WSAStartup 函数，直接返回 0
 * 
 * 非 Windows 系统不需要初始化 Windows 套接字库，所以直接返回 0。
 * @return 始终返回 0。
 */
int WSAStartup()
{
    return 0;
}
#endif // _WIN64

/**
 * @brief 关闭套接字描述符，根据不同系统调用不同的关闭函数
 * 
 * 在 Windows 系统下调用 closesocket 函数，在非 Windows 系统下调用 close 函数。
 * @param fd 要关闭的套接字描述符。
 * @return 关闭成功返回 0，失败返回 -1。
 */
int close_socket(int fd)
{
    int r;
#ifdef _WIN64
    // Windows 系统下关闭套接字
    r = ::closesocket(fd);
#else  // _WIN64
    // 非 Windows 系统下关闭文件描述符
    r = ::close(fd);
#endif // _WIN64
    return r;
}

/**
 * @brief 安全的 accept 函数，处理各种可能的错误
 * 
 * 该函数会处理信号中断和一些可接受的错误，并在必要时设置 keep-alive 选项。
 * @param sock 监听套接字描述符。
 * @param sa 用于存储客户端地址信息的结构体指针。
 * @param len 客户端地址信息结构体的长度指针。
 * @return 成功返回新的客户端套接字描述符，失败返回 -1。
 */
// accept
static int sane_accept(int sock, struct sockaddr *sa, socklen_t *len)
{
    // 初始化 Windows 套接字库
    WSAStartup();
    // 定义可接受的错误码数组
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
    // 新的客户端套接字描述符
    int fd = -1;
    int errno2;

    while (1)
    {
        // 调用系统的 accept 函数
        if ((fd = ::accept(sock, sa, len)) < 0)
        {
            // 获取错误码
            errno2 = get_errno();
            if (errno2 == ZCC_EINTR)
            {
                // 信号中断
                if (var_sigint_flag)
                {
                    // 设置无效参数错误码
                    set_errno(ZCC_EINVAL);
                    break;
                }
                continue;
            }
            // 检查错误码是否在可接受的错误码数组中
            for (count = 0; (err = accept_ok_errors[count]) != 0; count++)
            {
                if (errno2 == err)
                {
                    // 设置重试错误码
                    set_errno(ZCC_EAGAIN);
                    break;
                }
            }
        }
        else if (sa && (sa->sa_family == AF_INET || sa->sa_family == AF_INET6))
        {
            // 对于 IPv4 或 IPv6 地址，设置 keep-alive 选项
            int on = 1;
            (void)setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char *)&on, sizeof(on));
        }
        break;
    }

    return (fd);
}

/**
 * @brief 用于 Unix 域套接字的 accept 函数
 * 
 * 调用 sane_accept 函数，不传递地址信息。
 * @param fd 监听套接字描述符。
 * @return 成功返回新的客户端套接字描述符，失败返回 -1。
 */
int unix_accept(int fd)
{
    return (sane_accept(fd, (struct sockaddr *)0, (socklen_t *)0));
}

/**
 * @brief 用于网络套接字的 accept 函数
 * 
 * 调用 sane_accept 函数，传递地址信息。
 * @param fd 监听套接字描述符。
 * @return 成功返回新的客户端套接字描述符，失败返回 -1。
 */
int inet_accept(int fd)
{
    struct sockaddr_storage ss;
    socklen_t ss_len = sizeof(ss);
    return (sane_accept(fd, (struct sockaddr *)&ss, &ss_len));
}

/**
 * @brief 根据监听类型调用不同的 accept 函数
 * 
 * 根据传入的监听类型调用相应的 accept 函数。
 * @param fd 监听套接字描述符。
 * @param type 监听类型，如 var_tcp_listen_type_inet、var_tcp_listen_type_unix 等。
 * @return 成功返回新的客户端套接字描述符，失败返回 -1。
 */
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

/**
 * @brief 创建并监听 Unix 域套接字
 * 
 * 在 Linux 系统下创建并监听 Unix 域套接字，处理文件删除、绑定和监听等操作。
 * @param addr Unix 域套接字的路径。
 * @param backlog 监听队列的最大长度。
 * @return 成功返回监听套接字描述符，失败返回 -1。
 */
// listen
int unix_listen(char *addr, int backlog)
{
#ifdef __linux__
    struct sockaddr_un sun;
    int len = strlen(addr);
    // 监听套接字描述符
    int sock = -1;
    int errno2;

    if (len >= (int)sizeof(sun.sun_path))
    {
        // 路径名过长
        errno = ENAMETOOLONG;
        return -1;
    }

    // 清空结构体
    memset((char *)&sun, 0, sizeof(struct sockaddr_un));
    // 设置地址族为 Unix 域
    sun.sun_family = AF_UNIX;
    // 复制路径名
    memcpy(sun.sun_path, addr, len + 1);

    if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
    {
        // 创建套接字失败
        return -1;
    }

    if (unlink(addr) < 0 && errno != ENOENT)
    {
        // 删除文件失败，输出致命错误信息
        zcc_fatal("unlink: %s(%m)", addr);
    }

    if (bind(sock, (struct sockaddr *)&sun, sizeof(struct sockaddr_un)) < 0)
    {
        // 绑定失败，跳转到错误处理
        goto err;
    }

    if (listen(sock, backlog) < 0)
    {
        // 监听失败，跳转到错误处理
        goto err;
    }

    return (sock);

err:
    // 保存错误码
    errno2 = errno;
    if (sock > -1)
    {
        // 关闭套接字
        close_socket(sock);
    }
    errno = errno2;
#endif // __linux__

    return -1;
}

/**
 * @brief 创建并监听网络套接字
 * 
 * 创建并监听 IPv4 网络套接字，处理套接字创建、选项设置、绑定和监听等操作。
 * @param sip 监听的 IP 地址，为空表示监听所有地址。
 * @param port 监听的端口号。
 * @param backlog 监听队列的最大长度。
 * @return 成功返回监听套接字描述符，失败返回 -1。
 */
int inet_listen(const char *sip, int port, int backlog)
{
    // 监听套接字描述符
    int sock;
    int on = 1;
    struct sockaddr_in addr;
    int errno2;

    // 初始化 Windows 套接字库
    WSAStartup();

    // 清空结构体
    std::memset(&addr, 0, sizeof(struct sockaddr_in));
    // 设置地址族为 IPv4
    addr.sin_family = AF_INET;
    // 转换端口号为网络字节序
    addr.sin_port = htons(port);
    // 设置 IP 地址
    addr.sin_addr.s_addr = (empty(sip) ? INADDR_ANY : inet_addr(sip));

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        // 创建套接字失败
        return -1;
    }

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0)
    {
        // 设置选项失败，跳转到错误处理
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
        // 绑定失败，跳转到错误处理
        goto err;
    }

    if (listen(sock, backlog) < 0)
    {
        // 监听失败，跳转到错误处理
        goto err;
    }

    return (sock);

err:
    // 保存错误码
    errno2 = get_errno();
    // 关闭套接字
    close_socket(sock);
    set_errno(errno2);

    return -1;
}

/**
 * @brief 根据网络路径创建并监听套接字
 * 
 * 根据传入的网络路径判断监听类型（网络套接字、Unix 域套接字或 FIFO），并调用相应的监听函数。
 * @param netpath 网络路径，如 "127.0.0.1:8080" 或 "/tmp/socket"。
 * @param backlog 监听队列的最大长度。
 * @param type 用于存储监听类型的指针，可以为 NULL。
 * @return 成功返回监听套接字描述符，失败返回 -1。
 */
int netpath_listen(const char *netpath, int backlog, int *type)
{
    // 初始化 Windows 套接字库
    WSAStartup();
    char _netpath[1024], *path, *host, *p;
    // 监听套接字描述符
    int fd = -1, port = 0, tp;

    // 复制网络路径
    std::strncpy(_netpath, netpath, 1000);
    _netpath[1000] = 0;
    if (*_netpath == 0)
    {
        // 路径为空，设置访问错误码
        set_errno(ZCC_EACCES);
        return -1;
    }
    // 查找冒号位置
    p = std::strchr(_netpath, ':');
    if ((!p) || ((_netpath - p == 1) && (_netpath[0] != '0')))
    {
        // 没有冒号或格式错误，认为是 Unix 域套接字
        tp = var_tcp_listen_type_unix;
        path = _netpath;
    }
    else
    {
        // 分割字符串
        *p++ = 0;
        if (!std::strcmp(_netpath, "local"))
        {
            // 本地路径，认为是 Unix 域套接字
            tp = var_tcp_listen_type_unix;
            path = p;
        }
        else if (!strcmp(_netpath, "fifo"))
        {
            // FIFO 路径
            tp = var_tcp_listen_type_fifo;
            path = p;
        }
        else
        {
            // 网络套接字
            tp = var_tcp_listen_type_inet;
            host = _netpath;
            port = std::atoi(p);
        }
    }

    if (type)
    {
        // 存储监听类型
        *type = tp;
    }

    if ((tp == var_tcp_listen_type_inet) && (port < 1))
    {
        // 网络套接字端口号无效
        errno = EINVAL;
        return -1;
    }
    if (tp == var_tcp_listen_type_inet)
    {
        // 调用网络套接字监听函数
        fd = inet_listen(host, port, backlog);
    }
    else if (tp == var_tcp_listen_type_unix)
    {
        // 调用 Unix 域套接字监听函数
        fd = unix_listen(path, backlog);
    }
    else if (tp == var_tcp_listen_type_fifo)
    {
        // 调用 FIFO 监听函数
        fd = fifo_listen(path);
    }

    return fd;
}

/**
 * @brief 创建并监听 FIFO 文件
 * 
 * 在 Linux 系统下创建并打开 FIFO 文件进行监听。
 * @param path FIFO 文件的路径。
 * @return 成功返回文件描述符，失败返回 -1。
 */
// fifo listen
int fifo_listen(const char *path)
{
#ifdef __linux__
    // 文件描述符
    int fd;
    int errno2;

    fd = -1;
    if ((mkfifo(path, 0666) < 0) && (errno != EEXIST))
    {
        // 创建 FIFO 文件失败，跳转到错误处理
        goto err;
    }
    if ((fd = open(path, O_RDWR | O_NONBLOCK, 0)) < 0)
    {
        // 打开 FIFO 文件失败，跳转到错误处理
        goto err;
    }

    return (fd);

err:
    // 保存错误码
    errno2 = errno;
    if (fd != -1)
    {
        // 关闭文件描述符
        close_socket(fd);
    }
    errno = errno2;
#endif // __linux__
    return -1;
}

/**
 * @brief 安全的 connect 函数，处理各种可能的错误
 * 
 * 该函数会设置 keep-alive 选项，并处理连接过程中的中断和可接受的错误。
 * @param sock 套接字描述符。
 * @param sa 目标地址信息结构体指针。
 * @param len 目标地址信息结构体的长度。
 * @return 成功返回 0，失败返回 -1。
 */
// connect
static int sane_connect(int sock, struct sockaddr *sa, int len)
{
    if (sa->sa_family == AF_INET)
    {
        // 对于 IPv4 地址，设置 keep-alive 选项
        int on = 1;
        (void)::setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char *)&on, sizeof(on));
    }
    while (1)
    {
        // 调用系统的 connect 函数
        if (::connect(sock, sa, len) == 0)
        {
            // 连接成功
            break;
        }
        // 获取错误码
        int ec = get_errno();
        if (ec == ZCC_EINTR)
        {
            // 信号中断，继续尝试
            continue;
        }
        if ((ec == ZCC_EINPROGRESS) || (ec == ZCC_EWOULDBLOCK))
        {
            // 连接正在进行中，认为成功
            break;
        }
        // 连接失败
        return -1;
    }
    return (0);
}

/**
 * @brief 连接并等待连接成功
 * 
 * 调用 sane_connect 函数进行连接，并等待套接字可写表示连接成功。
 * @param sock 套接字描述符。
 * @param sa 目标地址信息结构体指针。
 * @param len 目标地址信息结构体的长度。
 * @param timeout 超时时间，单位为毫秒。
 * @return 成功返回 0，失败返回 -1。
 */
static int connect_and_wait_ok(int sock, struct sockaddr *sa, int len, int timeout)
{
    // 调用安全的 connect 函数
    int ret = sane_connect(sock, sa, len);
    if (ret < 0)
    {
        // 连接失败
        return ret;
    }

    int readable = 0, writeable = 0;
    // 等待套接字可读或可写
    ret = timed_read_write_wait(sock, timeout, &readable, &writeable);
    if (ret < 1)
    {
        // 等待超时或出错
        return -1;
    }
    if (writeable == 0)
    {
        // 不可写，连接失败
        return -1;
    }
    if (readable == 0)
    {
        // 不可读，连接成功
        return 0;
    }

    // 再次尝试连接
    ret = ::connect(sock, sa, len);
    if (ret == 0)
    {
        // 连接失败
        return -1;
    }
    // 获取错误码
    int ec = get_errno();
    switch (ec)
    {
    case ZCC_EISCONN:
        // 已经连接，返回成功
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
        // 其他错误，连接失败
        return -1;
        break;
    }
    return -1;
}

/**
 * @brief 连接到 Unix 域套接字
 * 
 * 在 Linux 系统下创建并连接到 Unix 域套接字，支持超时设置。
 * @param addr Unix 域套接字的路径。
 * @param timeout 超时时间，单位为毫秒，小于 0 表示使用默认超时时间。
 * @return 成功返回套接字描述符，失败返回 -1。
 */
int unix_connect(const char *addr, int timeout)
{
    // 套接字描述符
    int sock = -1;
#ifdef __linux__
    struct sockaddr_un sun;
    int len = std::strlen(addr);
    int errno2;

    if (len >= (int)sizeof(sun.sun_path))
    {
        // 路径名过长，设置错误码
        set_errno(ZCC_ENAMETOOLONG);
        return -1;
    }

    if (timeout < 0)
    {
        // 超时时间小于 0，使用默认超时时间
        timeout = var_io_max_timeout;
    }

    // 清空结构体
    std::memset((char *)&sun, 0, sizeof(sun));
    // 设置地址族为 Unix 域
    sun.sun_family = AF_UNIX;
    // 复制路径名
    std::memcpy(sun.sun_path, addr, len + 1);

    if ((sock = ::socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
    {
        // 创建套接字失败
        return (-1);
    }

    if (timeout > 0)
    {
        // 设置非阻塞模式
        nonblocking(sock, true);
        if (connect_and_wait_ok(sock, (struct sockaddr *)&sun, sizeof(sun), timeout) < 0)
        {
            // 连接失败，保存错误码并关闭套接字
            errno2 = get_errno();
            ::close(sock);
            set_errno(errno2);
            return (-1);
        }
        // 恢复阻塞模式
        nonblocking(sock, false);
    }
    else
    {
        if (sane_connect(sock, (struct sockaddr *)&sun, sizeof(sun)) < 0)
        {
            // 连接失败，保存错误码并关闭套接字
            errno2 = get_errno();
            close(sock);
            set_errno(errno2);
            return (-1);
        }
    }
#endif // __linux__
    return (sock);
}

/**
 * @brief 连接到网络套接字
 * 
 * 创建并连接到 IPv4 网络套接字，支持超时设置。
 * @param dip 目标 IP 地址。
 * @param port 目标端口号。
 * @param timeout 超时时间，单位为毫秒，小于 0 表示使用默认超时时间。
 * @return 成功返回套接字描述符，失败返回 -1。
 */
int inet_connect(const char *dip, int port, int timeout)
{
    // 套接字描述符
    int sock;
    struct sockaddr_in addr;
    int errno2;

    if (timeout < 0)
    {
        // 超时时间小于 0，使用默认超时时间
        timeout = var_io_max_timeout;
    }

    // 初始化 Windows 套接字库
    WSAStartup();

    if ((sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        // 创建套接字失败
        return (-1);
    }

    // 清空结构体
    std::memset(&addr, 0, sizeof(struct sockaddr_in));
    // 设置地址族为 IPv4
    addr.sin_family = AF_INET;
    // 转换端口号为网络字节序
    addr.sin_port = htons(port);
    // 设置目标 IP 地址
    addr.sin_addr.s_addr = inet_addr(dip);

    if (timeout > 0)
    {
        // 设置非阻塞模式
        nonblocking(sock, true);
        if (connect_and_wait_ok(sock, (struct sockaddr *)&addr, sizeof(addr), timeout) < 0)
        {
            // 连接失败，保存错误码并关闭套接字
            errno2 = get_errno();
            close_socket(sock);
            set_errno(errno2);
            return (-1);
        }
        // 恢复阻塞模式
        nonblocking(sock, false);
    }
    else
    {
        if (sane_connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        {
            // 连接失败，保存错误码并关闭套接字
            errno2 = get_errno();
            close_socket(sock);
            set_errno(errno2);
            return (-1);
        }
    }

    return (sock);
}

/**
 * @brief 根据主机名或 IP 地址连接到网络套接字
 * 
 * 如果传入的是 IP 地址，直接调用 inet_connect 函数；如果是主机名，解析主机名得到 IP 地址列表并尝试连接。
 * @param host 主机名或 IP 地址。
 * @param port 目标端口号。
 * @param timeout 超时时间，单位为毫秒，小于 0 表示使用默认超时时间。
 * @return 成功返回套接字描述符，失败返回 -1。
 */
int host_connect(const char *host, int port, int timeout)
{
    // 套接字描述符
    int sock = -1;

    if (is_ip(host))
    {
        // 传入的是 IP 地址，直接连接
        sock = inet_connect(host, port, timeout);
        return sock;
    }

    // 存储解析得到的 IP 地址列表
    std::vector<std::string> ips;
    // 解析主机名
    get_hostaddr(host, ips);
    for (auto it = ips.begin(); it != ips.end(); it++)
    {
        const char *ip = it->c_str();
        // 尝试连接每个 IP 地址
        sock = inet_connect(ip, port, timeout);
        if (sock > -1)
        {
            // 连接成功，跳出循环
            break;
        }
    }
    return sock;
}

/**
 * @brief 根据网络路径连接到套接字
 * 
 * 支持多个网络路径，尝试连接每个路径直到成功。
 * @param netpath 网络路径，多个路径可以用 ";, \t\r\n" 分隔。
 * @param timeout 超时时间，单位为毫秒，小于 0 表示使用默认超时时间。
 * @return 成功返回套接字描述符，失败返回 -1。
 */
int netpath_connect(const char *netpath, int timeout)
{
    static int64_t _offset = 0;
    // 套接字描述符
    int sock = -1;
    // 分割网络路径
    std::vector<std::string> hs = split(netpath, ";, \t\r\n");
    int64_t length = hs.size();
    int64_t offset = _offset++;
    for (int64_t i = 0; i < length; i++)
    {
        std::string &path = hs[(offset + i) % length];
        if (path.empty())
        {
            // 路径为空，跳过
            continue;
        }
        auto pos = path.find(':');
        if (pos == std::string::npos)
        {
            // 没有冒号，认为是 Unix 域套接字路径
            sock = unix_connect(path.c_str(), timeout);
        }
        else
        {
            // 分割主机名和端口号
            std::string host = path.substr(0, pos);
            int port = std::atoi(path.c_str() + pos + 1);
            // 连接到网络套接字
            sock = host_connect(host.c_str(), port, timeout);
        }
        if (sock > -1)
        {
            // 连接成功，跳出循环
            break;
        }
    }
    return sock;
}

/**
 * @brief 获取对端的 IP 地址和端口号
 * 
 * 获取指定套接字的对端 IP 地址和端口号。
 * @param sockfd 套接字描述符。
 * @param host 用于存储对端 IP 地址的指针，可以为 NULL。
 * @param port 用于存储对端端口号的指针，可以为 NULL。
 * @return 成功返回 1，失败返回 0。
 */
int get_peername(int sockfd, int *host, int *port)
{
    // 初始化 Windows 套接字库
    WSAStartup();
    struct sockaddr_in sa;
    socklen_t sa_length = sizeof(struct sockaddr);

    if (getpeername(sockfd, (struct sockaddr *)&sa, &sa_length) < 0)
    {
        // 获取对端地址信息失败
        return 0;
    }

    if (host)
    {
        // 存储对端 IP 地址
        *host = *((int *)&(sa.sin_addr));
    }

    if (port)
    {
        // 存储对端端口号
        *port = ntohs(sa.sin_port);
    }

    return 1;
}

zcc_namespace_end;

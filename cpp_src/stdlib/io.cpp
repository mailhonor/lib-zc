/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-10-12
 * ================================
 */

#include "zcc/zcc_stdlib.h"
#include "zcc/zcc_errno.h"
#ifdef _WIN64
#include <WinSock2.h>
#include <handleapi.h>
#include <io.h>
#else // _WIN64
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#ifdef __linux__
#include <sys/ioctl.h>
#include <sys/socket.h>
#endif // __linux__
#include <poll.h>
#endif // _WIN64

zcc_namespace_begin;

/**
 * @brief 关闭文件描述符（Windows 64位系统版本）
 * @param fd 文件句柄
 * @return 成功返回1，失败返回-1
 */
#ifdef _WIN64
int close(HANDLE fd)
{
    // 调用Windows API关闭句柄
    return CloseHandle(fd) ? 1 : -1;
}
#else  // _WIN64
/**
 * @brief 关闭文件描述符（非Windows 64位系统版本）
 * @param fd 文件描述符
 * @return 系统调用close的返回值
 */
int close(int fd)
{
    // 调用系统的close函数
    return ::close(fd);
}
#endif // _WIN64

/* timed read/write wait */
/**
 * @brief 定时等待文件描述符可读或可写（Windows 64位系统版本）
 * @param fd 文件描述符
 * @param read_wait_timeout 等待超时时间（毫秒）
 * @param readable 指向一个整数，用于存储是否可读的结果
 * @param writeable 指向一个整数，用于存储是否可写的结果
 * @return 成功返回1，超时返回0，错误返回-1
 */
#ifdef _WIN64
int timed_read_write_wait_millisecond(int fd, int read_wait_timeout, int *readable, int *writeable)
{
    int ec;
    // 计算超时时间戳
    int64_t stamp_timeout = millisecond(read_wait_timeout), left_timeout;
    int timeout;

    fd_set fds_r;
    if (readable)
    {
        // 初始化可读文件描述符集合
        *readable = 0;
        FD_ZERO(&fds_r);
        FD_SET(fd, &fds_r);
    }

    fd_set fds_w;
    if (writeable)
    {
        // 初始化可写文件描述符集合
        *writeable = 0;
        FD_ZERO(&fds_w);
        FD_SET(fd, &fds_w);
    }

    fd_set fds_e;
    // 初始化异常文件描述符集合
    FD_ZERO(&fds_e);
    FD_SET(fd, &fds_e);

    for (;;)
    {
        if (read_wait_timeout < 0)
        {
            // 无限等待
            timeout = -1;
        }
        else if (read_wait_timeout == 0)
        {
            // 立即返回
            timeout = 0;
        }
        else
        {
            // 计算剩余超时时间
            left_timeout = millisecond_to(stamp_timeout);
            if (left_timeout < 1)
            {
                return 0;
            }
            else if (left_timeout > 1000 * 3600)
            {
                timeout = 1000 * 3600;
            }
            else
            {
                timeout = left_timeout;
            }
        }

        struct timeval tv, *tp = &tv;
        if (timeout > 0)
        {
            // 设置超时时间
            tv.tv_sec = timeout / 1000;
            tv.tv_usec = (timeout % 1000) * 1000;
        }
        else if (timeout < 0)
        {
            // 无限等待时间
            tv.tv_sec = 3600 * 24 * 365 * 10;
            tv.tv_usec = 0;
        }
        else
        {
            tp = 0;
        }
        for (;;)
        {
            // 调用select函数进行等待
            switch (select(1, readable ? &fds_r : 0, writeable ? &fds_w : 0, &fds_e, tp))
            {
            case -1:
                // 获取错误码
                ec = get_errno();
                if (ec != EINTR)
                {
                    return -1;
                }
                continue;
            case 0:
                if (read_wait_timeout <= 0)
                {
                    return 0;
                }
                return 0;
            default:
                if (readable)
                {
                    // 检查是否可读
                    if (FD_ISSET(fd, &fds_r))
                    {
                        *readable = 1;
                    }
                }
                if (writeable)
                {
                    // 检查是否可写
                    if (FD_ISSET(fd, &fds_w))
                    {
                        *writeable = 1;
                    }
                }
                return 1;
            }
        }
    }

    return 0;
}
#else  // _WIN64
/**
 * @brief 定时等待文件描述符可读或可写（非Windows 64位系统版本）
 * @param fd 文件描述符
 * @param read_wait_timeout 等待超时时间（毫秒）
 * @param readable 指向一个整数，用于存储是否可读的结果
 * @param writeable 指向一个整数，用于存储是否可写的结果
 * @return 成功返回1，超时返回0，错误返回-1
 */
/* read */
/* readable means: 1, have readable data.
 *                 2, peer closed.
 * when receive POLLRDHUP, maybe have some readable data.
 */
int timed_read_write_wait_millisecond(int fd, int read_wait_timeout, int *readable, int *writeable)
{
    struct pollfd pollfd;
    // 计算超时时间戳
    int stamp_timeout = millisecond(read_wait_timeout), left_timeout;
    int timeout;
    if (readable)
    {
        // 初始化可读标志
        *readable = 0;
    }
    if (writeable)
    {
        // 初始化可写标志
        *writeable = 0;
    }

    for (;;)
    {
        pollfd.fd = fd;
        pollfd.events = 0;
        if (readable)
        {
            // 设置可读事件
            pollfd.events |= POLLIN;
        }
        if (writeable)
        {
            // 设置可写事件
            pollfd.events |= POLLOUT;
        }
        if (read_wait_timeout < 0)
        {
            // 无限等待
            timeout = -1;
        }
        else if (read_wait_timeout == 0)
        {
            // 立即返回
            timeout = 0;
        }
        else
        {
            // 计算剩余超时时间
            left_timeout = millisecond_to(stamp_timeout);
            if (left_timeout < 1)
            {
                return 0;
            }
            else if (left_timeout > 1000 * 3600)
            {
                timeout = 1000 * 3600;
            }
            else
            {
                timeout = left_timeout;
            }
        }
        // 调用poll函数进行等待
        switch (poll(&pollfd, 1, timeout))
        {
        case -1:
            if (get_errno() != ZCC_EINTR)
            {
                // 输出错误信息
                zcc_fatal("poll error(%m)");
            }
            continue;
        case 0:
            if (read_wait_timeout <= 0)
            {
                return 0;
            }
            continue;
        default:
            if (pollfd.revents & POLLNVAL)
            {
                return -1;
            }
            if (pollfd.revents & (POLLIN | POLLHUP | POLLERR | POLLRDNORM | POLLWRNORM))
            {
                if (readable)
                {
                    // 设置可读标志
                    *readable = 1;
                }
            }
            if (pollfd.revents & POLLOUT)
            {
                if (writeable)
                {
                    // 设置可写标志
                    *writeable = 1;
                }
            }
            return 1;
        }
    }

    return 0;
}
#endif // _WIN64

/**
 * @brief 定时等待文件描述符可读或可写（秒为单位）
 * @param fd 文件描述符
 * @param read_wait_timeout 等待超时时间（秒）
 * @param readable 指向一个整数，用于存储是否可读的结果
 * @param writeable 指向一个整数，用于存储是否可写的结果
 * @return 调用timed_read_write_wait_millisecond的结果
 */
int timed_read_write_wait(int fd, int read_wait_timeout, int *readable, int *writeable)
{
    // 将秒转换为毫秒并调用timed_read_write_wait_millisecond
    return timed_read_write_wait_millisecond(fd, 1000 * read_wait_timeout, readable, writeable);
}

/**
 * @brief 定时等待文件描述符可读（毫秒为单位）
 * @param fd 文件描述符
 * @param wait_timeout 等待超时时间（毫秒）
 * @return 成功返回1，超时返回0，错误返回-1
 */
int timed_read_wait_millisecond(int fd, int wait_timeout)
{
    int want_read;
    // 调用timed_read_write_wait_millisecond等待可读
    int ret = timed_read_write_wait_millisecond(fd, wait_timeout, &want_read, 0);
    if (ret < 1)
    {
        return ret;
    }
    return want_read;
}

/**
 * @brief 定时等待文件描述符可读（秒为单位）
 * @param fd 文件描述符
 * @param wait_timeout 等待超时时间（秒）
 * @return 成功返回1，超时返回0，错误返回-1
 */
int timed_read_wait(int fd, int wait_timeout)
{
    int want_read;
    // 将秒转换为毫秒并调用timed_read_write_wait_millisecond等待可读
    int ret = timed_read_write_wait_millisecond(fd, 1000L * wait_timeout, &want_read, 0);
    if (ret < 1)
    {
        return ret;
    }
    return want_read;
}

/**
 * @brief 定时读取文件描述符的数据
 * @param fd 文件描述符
 * @param buf 数据缓冲区
 * @param size 缓冲区大小
 * @param wait_timeout 等待超时时间（秒）
 * @return 读取的字节数，超时或错误返回-1
 */
int timed_read(int fd, void *buf, int size, int wait_timeout)
{
    int ret, ec;

    if (wait_timeout == 0)
    {
        for (;;)
        {
            ret = -1;
#ifdef _WIN64
            // 调用Windows API接收数据
            ret = recv(fd, (char *)buf, size, 0);
#else  // _WIN64
       // 调用系统read函数读取数据
            ret = ::read(fd, buf, size);
#endif // _WIN64
            if (ret < 0)
            {
                // 获取错误码
                ec = get_errno();
                if (ec == ZCC_EINTR)
                {
                    continue;
                }
            }
            return ret;
        }
    }

    for (;;)
    {
        // 等待可读
        if ((ret = timed_read_wait_millisecond(fd, 1000L * wait_timeout)) == 0)
        {
            return -1;
        }
        ret = -1;
#ifdef _WIN64
        // 调用Windows API接收数据
        ret = recv(fd, (char *)buf, size, 0);
#else  // _WIN64
       // 调用系统read函数读取数据
        ret = read(fd, buf, size);
#endif // _WIN64
        if (ret < 0)
        {
            // 获取错误码
            ec = get_errno();
            if (ec == ZCC_EINTR)
            {
                continue;
            }
            if (ec == ZCC_EAGAIN)
            {
                continue;
            }
        }
        return ret;
    }
    return -1;
}

/* write */
/**
 * @brief 定时等待文件描述符可写（毫秒为单位）
 * @param fd 文件描述符
 * @param wait_timeout 等待超时时间（毫秒）
 * @return 成功返回1，超时返回0，错误返回-1
 */
int timed_write_wait_millisecond(int fd, int wait_timeout)
{
    int want_write;
    // 调用timed_read_write_wait_millisecond等待可写
    int ret = timed_read_write_wait_millisecond(fd, wait_timeout, 0, &want_write);
    if (ret < 1)
    {
        return ret;
    }
    return want_write;
}

/**
 * @brief 定时等待文件描述符可写（秒为单位）
 * @param fd 文件描述符
 * @param wait_timeout 等待超时时间（秒）
 * @return 成功返回1，超时返回0，错误返回-1
 */
int timed_write_wait(int fd, int wait_timeout)
{
    int want_write;
    // 将秒转换为毫秒并调用timed_read_write_wait_millisecond等待可写
    int ret = timed_read_write_wait_millisecond(fd, 1000L * wait_timeout, 0, &want_write);
    if (ret < 1)
    {
        return ret;
    }
    return want_write;
}

/**
 * @brief 定时向文件描述符写入数据
 * @param fd 文件描述符
 * @param buf 数据缓冲区
 * @param size 缓冲区大小
 * @param wait_timeout 等待超时时间（秒）
 * @return 写入的字节数，超时或错误返回-1
 */
int timed_write(int fd, const void *buf, int size, int wait_timeout)
{
    int ret, ec;

    if (wait_timeout == 0)
    {
        for (;;)
        {
            ret = -1;
#ifdef _WIN64
            // 调用Windows API发送数据
            ret = send(fd, (const char *)buf, size, 0);
#else  // _WIN64
       // 调用系统write函数写入数据
            ret = write(fd, buf, size);
#endif // _WIN64
            if (ret < 0)
            {
                // 获取错误码
                ec = get_errno();
                if (ec == ZCC_EINTR)
                {
                    continue;
                }
            }
            return ret;
        }
    }
    for (;;)
    {
        // 等待可写
        if (timed_write_wait_millisecond(fd, 1000L * wait_timeout) == 0)
        {
            return -1;
        }
#ifdef _WIN64
        // 调用Windows API发送数据
        ret = send(fd, (const char *)buf, size, 0);
#else  // _WIN64
       // 调用系统write函数写入数据
        ret = write(fd, buf, size);
#endif // _WIN64
        if (ret < 0)
        {
            // 获取错误码
            ec = get_errno();
            if (ec == ZCC_EINTR)
            {
                continue;
            }
            if (ec == ZCC_EAGAIN)
            {
                continue;
            }
            return -1;
        }
        else if (ret == 0)
        {
            continue;
        }
        return ret;
    }
    return -1;
}
/**
 * @brief 检查文件描述符是否可读或可写（Windows 64位系统版本）
 * @param fd 文件描述符
 * @param read_flag 是否检查可读
 * @param write_flag 是否检查可写
 * @return 可读或可写返回1，不可读且不可写返回0，错误返回-1
 */
#ifdef _WIN64
static int rwable_true_do(int fd, int read_flag, int write_flag)
{
    int ec;

    fd_set fds_r;
    if (read_flag)
    {
        // 初始化可读文件描述符集合
        FD_ZERO(&fds_r);
        FD_SET(fd, &fds_r);
    }

    fd_set fds_w;
    if (write_flag)
    {
        // 初始化可写文件描述符集合
        FD_ZERO(&fds_w);
        FD_SET(fd, &fds_w);
    }

    fd_set fds_e;
    // 初始化异常文件描述符集合
    FD_ZERO(&fds_e);
    FD_SET(fd, &fds_e);

    for (;;)
    {
        // 调用select函数检查
        switch (select(1, read_flag ? (&fds_r) : 0, write_flag ? (&fds_w) : 0, &fds_e, 0))
        {
        case -1:
            // 获取错误码
            ec = get_errno();
            if ((ec == ZCC_EINPROGRESS) || (ec == ZCC_EWOULDBLOCK))
            {
                continue;
            }
            return -1;
        case 0:
            return 0;
        default:
            return 1;
            // if (FD_ISSET(fd, &fds_r) || FD_ISSET(fd, &fds_w))
            // {
            //     return 1;
            // }
            // break;
        }
    }

    return 0;
}
#else // _WIN64
/**
 * @brief 检查文件描述符是否可读或可写（非Windows 64位系统版本）
 * @param fd 文件描述符
 * @param read_flag 是否检查可读
 * @param write_flag 是否检查可写
 * @return 可读或可写返回1，不可读且不可写返回0，错误返回-1
 */
static int rwable_true_do(int fd, int read_flag, int write_flag)
{
    struct pollfd pollfd;
    int flags = 0, revs;

    if (read_flag)
    {
        // 设置可读事件
        flags |= POLLIN;
    }
    if (write_flag)
    {
        // 设置可写事件
        flags |= POLLOUT;
    }

    pollfd.fd = fd;
    pollfd.events = flags;
    for (;;)
    {
        // 调用poll函数检查
        switch (poll(&pollfd, 1, 0))
        {
        case -1:
            if (get_errno() != ZCC_EINTR)
            {
                return -1;
            }
            continue;
        case 0:
            return 0;
        default:
            revs = pollfd.revents;
            if (revs & POLLNVAL)
            {
                return -1;
            }
            return 1;
            if (revs & (POLLIN | POLLOUT))
            {
                return 1;
            }
            return -1;
            if (revs & (POLLERR | POLLHUP
#ifdef __linux__
                        | POLLRDHUP
#endif
                        ))
            {
                return -1;
            }
        }
    }

    return 0;
}
#endif // _WIN64

/**
 * @brief 检查文件描述符是否可读或可写
 * @param fd 文件描述符
 * @return 可读或可写返回1，不可读且不可写返回0，错误返回-1
 */
int rwable(int fd)
{
    // 调用rwable_true_do检查可读和可写
    return rwable_true_do(fd, 1, 1);
}

/**
 * @brief 检查文件描述符是否可读
 * @param fd 文件描述符
 * @return 可读返回1，不可读返回0，错误返回-1
 */
int readable(int fd)
{
    // 调用rwable_true_do检查可读
    return rwable_true_do(fd, 1, 0);
}

/**
 * @brief 检查文件描述符是否可写
 * @param fd 文件描述符
 * @return 可写返回1，不可写返回0，错误返回-1
 */
int writeable(int fd)
{
    // 调用rwable_true_do检查可写
    return rwable_true_do(fd, 0, 1);
}

/**
 * @brief 设置文件描述符为非阻塞模式
 * @param fd 文件描述符
 * @param tf 是否设置为非阻塞模式
 * @return 失败返回-1, 0: 原始是阻塞的, 1: 原始是非阻塞的
 */
int nonblocking(int fd, bool tf)
{
#ifdef _WIN64
    // 设置Windows套接字为非阻塞模式
    u_long flags = (tf ? 1 : 0);
    if (ioctlsocket(fd, FIONBIO, &flags) == SOCKET_ERROR)
    {
        return -1;
    }
    return flags;
#else  // _WIN64
    int flags;
    // 获取文件描述符标志
    if ((flags = fcntl(fd, F_GETFL, 0)) < 0)
    {
        return -1;
    }
    // 设置文件描述符为非阻塞模式
    if (fcntl(fd, F_SETFL, tf ? flags | O_NONBLOCK : flags & ~O_NONBLOCK) < 0)
    {
        return -1;
    }
    return ((flags & O_NONBLOCK) ? 1 : 0);
#endif // _WIN64
    return -1;
}

/**
 * @brief 设置文件描述符在执行exec时关闭
 * @param fd 文件描述符
 * @param tf 是否设置在执行exec时关闭
 * @return 成功返回1，失败返回-1
 */
int close_on_exec(int fd, bool tf)
{
#ifdef _WIN64
    {
        // 获取Windows句柄
        HANDLE h = (HANDLE)_get_osfhandle(fd);
        // (HANDLE)(SOCKET)fd
        if (!SetHandleInformation(h, HANDLE_FLAG_INHERIT, 0))
        {
            return -1;
        }
    }
    return 1;
#else  // _WIN64
    int flags;
    // 获取文件描述符标志
    if ((flags = fcntl(fd, F_GETFD, 0)) < 0)
    {
        return -1;
    }
    // 设置文件描述符在执行exec时关闭
    if (fcntl(fd, F_SETFD, tf ? flags | FD_CLOEXEC : flags & ~FD_CLOEXEC) < 0)
    {
        return -1;
    }
    return ((flags & FD_CLOEXEC) ? 1 : 0);
#endif // _WIN64
    return -1;
}

/**
 * @brief 获取文件描述符可读的字节数
 * @param fd 文件描述符
 * @return 可读的字节数，错误返回-1
 */
int get_readable_count(int fd)
{
#ifdef _WIN64
    unsigned long count;
    // 调用Windows API获取可读字节数
    return (ioctlsocket(fd, FIONREAD, (unsigned long *)&count) < 0 ? -1 : count);
#else // _WIN64
    int count = -1;
#ifdef FIONREAD
    // 调用系统ioctl函数获取可读字节数
    return (ioctl(fd, FIONREAD, (char *)&count) < 0 ? -1 : count);
#else
    return count;
#endif
#endif
}

zcc_namespace_end;

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
#include <windows.h>
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

#ifdef _WIN64
int close(HANDLE fd)
{
    return CloseHandle(fd) ? 1 : -1;
}
#else  // _WIN64
int close(int fd)
{
    return ::close(fd);
}
#endif // _WIN64

/* timed read/write wait */
#ifdef _WIN64
int timed_read_write_wait_millisecond(int fd, int read_wait_timeout, int *readable, int *writeable)
{
    int ec;
    int64_t stamp_timeout = millisecond(read_wait_timeout), left_timeout;
    int timeout;

    fd_set fds_r;
    if (readable)
    {
        *readable = 0;
        FD_ZERO(&fds_r);
        FD_SET(fd, &fds_r);
    }

    fd_set fds_w;
    if (writeable)
    {
        *writeable = 0;
        FD_ZERO(&fds_w);
        FD_SET(fd, &fds_w);
    }

    fd_set fds_e;
    FD_ZERO(&fds_e);
    FD_SET(fd, &fds_e);

    for (;;)
    {
        if (read_wait_timeout < 0)
        {
            timeout = -1;
        }
        else if (read_wait_timeout == 0)
        {
            timeout = 0;
        }
        else
        {
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
            tv.tv_sec = timeout / 1000;
            tv.tv_usec = (timeout % 1000) * 1000;
        }
        else if (timeout < 0)
        {
            tv.tv_sec = 3600 * 24 * 365 * 10;
            tv.tv_usec = 0;
        }
        else
        {
            tp = 0;
        }
        for (;;)
        {
            switch (select(1, readable ? &fds_r : 0, writeable ? &fds_w : 0, &fds_e, tp))
            {
            case -1:
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
                    if (FD_ISSET(fd, &fds_r))
                    {
                        *readable = 1;
                    }
                }
                if (writeable)
                {
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
/* read */
/* readable means: 1, have readable data.
 *                 2, peer closed.
 * when receive POLLRDHUP, maybe have some readable data.
 */
int timed_read_write_wait_millisecond(int fd, int read_wait_timeout, int *readable, int *writeable)
{
    struct pollfd pollfd;
    int stamp_timeout = millisecond(read_wait_timeout), left_timeout;
    int timeout;
    if (readable)
    {
        *readable = 0;
    }
    if (writeable)
    {
        *writeable = 0;
    }

    for (;;)
    {
        pollfd.fd = fd;
        pollfd.events = 0;
        if (readable)
        {
            pollfd.events |= POLLIN;
        }
        if (writeable)
        {
            pollfd.events |= POLLOUT;
        }
        if (read_wait_timeout < 0)
        {
            timeout = -1;
        }
        else if (read_wait_timeout == 0)
        {
            timeout = 0;
        }
        else
        {
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
        switch (poll(&pollfd, 1, timeout))
        {
        case -1:
            if (get_errno() != ZCC_EINTR)
            {
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
                    *readable = 1;
                }
            }
            if (pollfd.revents & POLLOUT)
            {
                if (writeable)
                {
                    *writeable = 1;
                }
            }
            return 1;
        }
    }

    return 0;
}
#endif // _WIN64

int timed_read_write_wait(int fd, int read_wait_timeout, int *readable, int *writeable)
{
    return timed_read_write_wait_millisecond(fd, 1000 * read_wait_timeout, readable, writeable);
}

int timed_read_wait_millisecond(int fd, int wait_timeout)
{
    int want_read;
    int ret = timed_read_write_wait_millisecond(fd, wait_timeout, &want_read, 0);
    if (ret < 1)
    {
        return ret;
    }
    return want_read;
}

int timed_read_wait(int fd, int wait_timeout)
{
    int want_read;
    int ret = timed_read_write_wait_millisecond(fd, 1000L * wait_timeout, &want_read, 0);
    if (ret < 1)
    {
        return ret;
    }
    return want_read;
}

int timed_read(int fd, void *buf, int size, int wait_timeout)
{
    int ret, ec;

    if (wait_timeout == 0)
    {
        for (;;)
        {
            ret = -1;
#ifdef _WIN64
            ret = recv(fd, (char *)buf, size, 0);
#else  // _WIN64
            ret = ::read(fd, buf, size);
#endif // _WIN64
            if (ret < 0)
            {
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
        if ((ret = timed_read_wait_millisecond(fd, 1000L * wait_timeout)) == 0)
        {
            return -1;
        }
        ret = -1;
#ifdef _WIN64
        ret = recv(fd, (char *)buf, size, 0);
#else  // _WIN64
        ret = read(fd, buf, size);
#endif // _WIN64
        if (ret < 0)
        {
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
int timed_write_wait_millisecond(int fd, int wait_timeout)
{
    int want_write;
    int ret = timed_read_write_wait_millisecond(fd, wait_timeout, 0, &want_write);
    if (ret < 1)
    {
        return ret;
    }
    return want_write;
}

int timed_write_wait(int fd, int wait_timeout)
{
    int want_write;
    int ret = timed_read_write_wait_millisecond(fd, 1000L * wait_timeout, 0, &want_write);
    if (ret < 1)
    {
        return ret;
    }
    return want_write;
}

int timed_write(int fd, const void *buf, int size, int wait_timeout)
{
    int ret, ec;

    if (wait_timeout == 0)
    {
        for (;;)
        {
            ret = -1;
#ifdef _WIN64
            ret = send(fd, (const char *)buf, size, 0);
#else  // _WIN64
            ret = write(fd, buf, size);
#endif // _WIN64
            if (ret < 0)
            {
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
        if (timed_write_wait_millisecond(fd, 1000L * wait_timeout) == 0)
        {
            return -1;
        }
#ifdef _WIN64
        ret = send(fd, (const char *)buf, size, 0);
#else  // _WIN64
        ret = write(fd, buf, size);
#endif // _WIN64
        if (ret < 0)
        {
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
#ifdef _WIN64
static int rwable_true_do(int fd, int read_flag, int write_flag)
{
    int ec;

    fd_set fds_r;
    if (read_flag)
    {
        FD_ZERO(&fds_r);
        FD_SET(fd, &fds_r);
    }

    fd_set fds_w;
    if (write_flag)
    {
        FD_ZERO(&fds_w);
        FD_SET(fd, &fds_w);
    }

    fd_set fds_e;
    FD_ZERO(&fds_e);
    FD_SET(fd, &fds_e);

    for (;;)
    {
        switch (select(1, read_flag ? (&fds_r) : 0, write_flag ? (&fds_w) : 0, &fds_e, 0))
        {
        case -1:
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
static int rwable_true_do(int fd, int read_flag, int write_flag)
{
    struct pollfd pollfd;
    int flags = 0, revs;

    if (read_flag)
    {
        flags |= POLLIN;
    }
    if (write_flag)
    {
        flags |= POLLOUT;
    }

    pollfd.fd = fd;
    pollfd.events = flags;
    for (;;)
    {
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

int rwable(int fd)
{
    return rwable_true_do(fd, 1, 1);
}

int readable(int fd)
{
    return rwable_true_do(fd, 1, 0);
}

int writeable(int fd)
{
    return rwable_true_do(fd, 0, 1);
}

int nonblocking(int fd, bool tf)
{
#ifdef _WIN64
    u_long flags = (tf ? 1 : 0);
    if (ioctlsocket(fd, FIONBIO, &flags) == SOCKET_ERROR)
    {
        return -1;
    }
    return flags;
#else  // _WIN64
    int flags;
    if ((flags = fcntl(fd, F_GETFL, 0)) < 0)
    {
        return -1;
    }
    if (fcntl(fd, F_SETFL, tf ? flags | O_NONBLOCK : flags & ~O_NONBLOCK) < 0)
    {
        return -1;
    }
    return ((flags & O_NONBLOCK) ? 1 : 0);
#endif // _WIN64
    return -1;
}

int close_on_exec(int fd, bool tf)
{
#ifdef _WIN64
    {
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
    if ((flags = fcntl(fd, F_GETFD, 0)) < 0)
    {
        return -1;
    }
    if (fcntl(fd, F_SETFD, tf ? flags | FD_CLOEXEC : flags & ~FD_CLOEXEC) < 0)
    {
        return -1;
    }
    return ((flags & FD_CLOEXEC) ? 1 : 0);
#endif // _WIN64
    return -1;
}

int get_readable_count(int fd)
{
#ifdef _WIN64
    unsigned long count;
    return (ioctlsocket(fd, FIONREAD, (unsigned long *)&count) < 0 ? -1 : count);
#else // _WIN64
    int count = -1;
#ifdef FIONREAD
    return (ioctl(fd, FIONREAD, (char *)&count) < 0 ? -1 : count);
#else
    return count;
#endif
#endif
}

#ifdef __linux__
/* postfix src/util/unix_send_fd.c */
int send_fd(int fd, int sendfd)
{
    struct msghdr msg;
    struct iovec iov[1];

    union
    {
        struct cmsghdr just_for_alignment;
        char control[CMSG_SPACE(sizeof(sendfd))];
    } control_un;
    struct cmsghdr *cmptr;

    std::memset(&msg, 0, sizeof(msg));
    msg.msg_control = control_un.control;
    msg.msg_controllen = sizeof(control_un.control);

    cmptr = CMSG_FIRSTHDR(&msg);
    cmptr->cmsg_len = CMSG_LEN(sizeof(sendfd));
    cmptr->cmsg_level = SOL_SOCKET;
    cmptr->cmsg_type = SCM_RIGHTS;

    int *int_ptr = (int *)(CMSG_DATA(cmptr));
    *int_ptr = sendfd;

    msg.msg_name = 0;
    msg.msg_namelen = 0;

    iov->iov_base = var_blank_buffer;
    iov->iov_len = 1;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    int sendmsg_ret;
    do
    {
        sendmsg_ret = ::sendmsg(fd, &msg, 0);
    } while ((sendmsg_ret < 0) && (errno == EINTR));
    if (sendmsg_ret >= 0)
    {
        return 1;
    }

    return (-1);
}

/* postfix src/util/unix_recv_fd.c */
int recv_fd(int fd)
{
    struct msghdr msg;
    int newfd;
    struct iovec iov[1];
    char buf[1];

    union
    {
        struct cmsghdr just_for_alignment;
        char control[CMSG_SPACE(sizeof(newfd))];
    } control_un;
    struct cmsghdr *cmptr;

    std::memset(&msg, 0, sizeof(msg));
    msg.msg_control = control_un.control;
    msg.msg_controllen = sizeof(control_un.control);

    msg.msg_name = 0;
    msg.msg_namelen = 0;

    iov->iov_base = buf;
    iov->iov_len = sizeof(buf);
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    int recvmsg_ret;
    do
    {
        recvmsg_ret = ::recvmsg(fd, &msg, 0);
    } while ((recvmsg_ret < 0) && (errno == EINTR));
    if (recvmsg_ret < 0)
    {
        return -1;
    }

    if (((cmptr = CMSG_FIRSTHDR(&msg)) != 0) && (cmptr->cmsg_len == CMSG_LEN(sizeof(newfd))))
    {
        if (cmptr->cmsg_level != SOL_SOCKET)
        {
            zcc_fatal("control level %d != SOL_SOCKET", cmptr->cmsg_level);
        }
        if (cmptr->cmsg_type != SCM_RIGHTS)
        {
            zcc_fatal("control type %d != SCM_RIGHTS", cmptr->cmsg_type);
        }
        int *int_ptr = (int *)(CMSG_DATA(cmptr));
        newfd = *int_ptr;
        return newfd;
    }

    return (-1);
}
#endif // __linux__

zcc_namespace_end;

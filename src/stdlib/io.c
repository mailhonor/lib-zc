/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-11-26
 * ================================
 */

#include "zc.h"
#include <errno.h>
#include <sys/file.h>
#ifdef __linux__
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#endif // __linux__
#ifdef _WIN32
#include <Winsock2.h>
#include <fcntl.h>
#endif // WIN32

#ifdef __linux__
static int zrwable_true_do(int fd, int read_flag, int write_flag)
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
            if (errno != EINTR)
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
            if (revs & (POLLERR | POLLHUP | POLLRDHUP))
            {
                return -1;
            }
        }
    }

    return 0;
}
#endif // __linux__

#ifdef _WIN32
static int zrwable_true_do(int fd, int read_flag, int write_flag)
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
            ec = zget_errno();
            if ((ec == EINPROGRESS) || (ec == EWOULDBLOCK))
            {
                continue;
            }
            errno = ec;
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
#endif // _WIN32

int zrwable(int fd)
{
    return zrwable_true_do(fd, 1, 1);
}

int zreadable(int fd)
{
    return zrwable_true_do(fd, 1, 0);
}

int zwriteable(int fd)
{
    return zrwable_true_do(fd, 0, 1);
}

int znonblocking(int fd, int no)
{
#ifdef __linux__
    int flags;
    if ((flags = fcntl(fd, F_GETFL, 0)) < 0)
    {
        return -1;
    }
    if (fcntl(fd, F_SETFL, no ? flags | O_NONBLOCK : flags & ~O_NONBLOCK) < 0)
    {
        return -1;
    }
    return ((flags & O_NONBLOCK) ? 1 : 0);
#endif // __linux__

#ifdef _WIN32
    u_long flags = (no ? 1 : 0);
    if (ioctlsocket(fd, FIONBIO, &flags) == SOCKET_ERROR)
    {
        errno = EINVAL;
        return -1;
    }
    return flags;
#endif // _WIN32
    return -1;
}

int zclose_on_exec(int fd, int on)
{
#ifdef __linux__
    int flags;
    if ((flags = fcntl(fd, F_GETFD, 0)) < 0)
    {
        return -1;
    }
    if (fcntl(fd, F_SETFD, on ? flags | FD_CLOEXEC : flags & ~FD_CLOEXEC) < 0)
    {
        return -1;
    }
    return ((flags & FD_CLOEXEC) ? 1 : 0);
#endif // __linux__
#ifdef _WIN32
    return on;
#endif // _WIN32
    return -1;
}

int zget_readable_count(int fd)
{
#ifdef _WIN32
    unsigned long count;
    return (ioctlsocket(fd, FIONREAD, (unsigned long *)&count) < 0 ? -1 : count);
#else // _WIN32
    int count;
    return (ioctl(fd, FIONREAD, (char *)&count) < 0 ? -1 : count);
#endif
}

#ifdef __linux__
/* postfix src/util/unix_send_fd.c */
int zsend_fd(int fd, int sendfd)
{
    struct msghdr msg;
    struct iovec iov[1];

    union
    {
        struct cmsghdr just_for_alignment;
        char control[CMSG_SPACE(sizeof(sendfd))];
    } control_un;
    struct cmsghdr *cmptr;

    memset(&msg, 0, sizeof(msg));
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

    iov->iov_base = "";
    iov->iov_len = 1;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    int sendmsg_ret;
    do
    {
        sendmsg_ret = sendmsg(fd, &msg, 0);
    } while ((sendmsg_ret < 0) && (errno == EINTR));
    if (sendmsg_ret >= 0)
    {
        return 1;
    }

    return (-1);
}

/* postfix src/util/unix_recv_fd.c */
int zrecv_fd(int fd)
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

    memset(&msg, 0, sizeof(msg));
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
        recvmsg_ret = recvmsg(fd, &msg, 0);
    } while ((recvmsg_ret < 0) && (errno == EINTR));
    if (recvmsg_ret < 0)
    {
        return -1;
    }

    if (((cmptr = CMSG_FIRSTHDR(&msg)) != 0) && (cmptr->cmsg_len == CMSG_LEN(sizeof(newfd))))
    {
        if (cmptr->cmsg_level != SOL_SOCKET)
        {
            zfatal("control level %d != SOL_SOCKET", cmptr->cmsg_level);
        }
        if (cmptr->cmsg_type != SCM_RIGHTS)
        {
            zfatal("control type %d != SCM_RIGHTS", cmptr->cmsg_type);
        }
        int *int_ptr = (int *)(CMSG_DATA(cmptr));
        newfd = *int_ptr;
        return newfd;
    }

    return (-1);
}
#endif // __linux__

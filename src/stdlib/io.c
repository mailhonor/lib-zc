/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2015-11-26
 * ================================
 */

#include "zc.h"
#include <poll.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <errno.h>

int zrwable(int fd)
{
    struct pollfd pollfd;
    int flags = POLLIN|POLLOUT, revs;

    pollfd.fd = fd;
    pollfd.events = flags;
    for (;;) {
        switch (poll(&pollfd, 1, 0)) {
        case -1:
            if (errno != EINTR) {
                return -1;
            }
            continue;
        case 0:
            return 0;
        default:
            revs = pollfd.revents;
            if (revs & POLLNVAL) {
                return -1;
            }
            return 1;
            if (revs & (POLLIN|POLLOUT)) {
                return 1;
            }
            return -1;
            if (revs & (POLLERR | POLLHUP | POLLRDHUP)) {
                return -1;
            }
        }
    }

    return 0;
}

int zreadable(int fd)
{
    struct pollfd pollfd;
    int flags = POLLIN, revs;

    pollfd.fd = fd;
    pollfd.events = flags;
    for (;;) {
        switch (poll(&pollfd, 1, 0)) {
        case -1:
            if (errno != EINTR) {
                return -1;
            }
            continue;
        case 0:
            return 0;
        default:
            revs = pollfd.revents;
            if (revs & POLLNVAL) {
                return -1;
            }
            return 1;
            if (revs & (POLLIN)) {
                return 1;
            }
            return -1;
            if (revs & (POLLERR | POLLHUP | POLLRDHUP)) {
                return -1;
            }
        }
    }

    return 0;
}

int zwriteable(int fd)
{
    struct pollfd pollfd;
    int flags = POLLOUT, revs;

    pollfd.fd = fd;
    pollfd.events = flags;
    for (;;) {
        switch (poll(&pollfd, 1, 0)) {
        case -1:
            if (errno != EINTR) {
                return -1;
            }
            continue;
        case 0:
            return 0;
        default:
            revs = pollfd.revents;
            if (revs & POLLNVAL) {
                return -1;
            }
            return 1;
            if (revs & (POLLOUT)) {
                return 1;
            }
            return -1;
            if (revs & (POLLERR | POLLHUP | POLLRDHUP)) {
                return -1;
            }
        }
    }

    return 0;
}

int znonblocking(int fd, int no)
{
    int flags;

    if ((flags = fcntl(fd, F_GETFL, 0)) < 0) {
        return -1;
    }

    if (fcntl(fd, F_SETFL, no ? flags | O_NONBLOCK : flags & ~O_NONBLOCK) < 0) {
        return -1;
    }

    return ((flags & O_NONBLOCK)?1:0);
}

int zclose_on_exec(int fd, int on)
{
    int flags;

    if ((flags = fcntl(fd, F_GETFD, 0)) < 0) {
        return -1;
    }

    if (fcntl(fd, F_SETFD, on ? flags | FD_CLOEXEC : flags & ~FD_CLOEXEC) < 0) {
        return -1;
    }

    return ((flags & FD_CLOEXEC)?1:0);
}

int zget_readable_count(int fd)
{
    int count;
    return (ioctl(fd, FIONREAD, (char *)&count) < 0 ? -1 : count);
}

/* robust io ################################################### */
#define ___ROBUST_DO(exp) \
    int ret; \
    do { \
        ret = exp; \
    } while((ret<0) && (errno==EINTR)); \
    return ret;

int zopen(const char *pathname, int flags, mode_t mode)
{
    ___ROBUST_DO(open(pathname, flags, mode));
}

ssize_t zread(int fd, void *buf, size_t count)
{
    ssize_t ret;
    int ec;
    for (;;) {
        if ((ret = read(fd, buf, count)) < 0) {
            ec = errno;
            if (ec == EINTR) {
                continue;
            }
            return -1;
        }
        return ret;
    }
    return -1;
}

ssize_t zwrite(int fd, const void *buf, size_t count)
{
    ssize_t ret;
    int ec, is_closed = 0;
    const char *ptr = (const char *)buf;
    long left = count;
    for (;;) {
        ret = write(fd, ptr, left);
        if (ret < 0) {
            ec = errno;
            if (ec == EINTR) {
                continue;
            }
            if (ec == EPIPE) {
                is_closed = 1;
                break;
            }
            break;
        } else if (ret == 0) {
            continue;
        } else {
            left -= ret;
            ptr += ret;
        }
    }
    if (count > left) {
        return count - left;
    }
    if (is_closed) {
        return 0;
    }
    return -1;
}

int zclose(int fd)
{
    ___ROBUST_DO(close(fd));
}

int zflock(int fd, int operation)
{
    ___ROBUST_DO(flock(fd, operation));
}

int zflock_share(int fd)
{
    ___ROBUST_DO(flock(fd, LOCK_SH));
}

int zflock_exclusive(int fd)
{
    ___ROBUST_DO(flock(fd, LOCK_EX));
}

int zfunlock(int fd)
{
    ___ROBUST_DO(flock(fd, LOCK_UN));
}

int zrename(const char *oldpath, const char *newpath)
{
    ___ROBUST_DO(rename(oldpath, newpath));
}

int zunlink(const char *pathname)
{
    ___ROBUST_DO(unlink(pathname));
}

/* postfix src/util/unix_send_fd.c */
int zsend_fd(int fd, int sendfd)
{
    struct msghdr msg;
    struct iovec iov[1];

    union {
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
    do {
        sendmsg_ret = sendmsg(fd, &msg, 0);
    } while((sendmsg_ret<0) && (errno==EINTR));
    if (sendmsg_ret >= 0) {
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

    union {
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
    do {
        recvmsg_ret = recvmsg(fd, &msg, 0);
    } while((recvmsg_ret<0) && (errno==EINTR));
    if (recvmsg_ret < 0) {
        return -1;
    }

    if (((cmptr = CMSG_FIRSTHDR(&msg)) != 0) && (cmptr->cmsg_len == CMSG_LEN(sizeof(newfd)))) {
        if (cmptr->cmsg_level != SOL_SOCKET) {
            zfatal("FATAL control level %d != SOL_SOCKET", cmptr->cmsg_level);
        }
        if (cmptr->cmsg_type != SCM_RIGHTS) {
            zfatal("FATAL control type %d != SCM_RIGHTS", cmptr->cmsg_type);
        }
        int *int_ptr = (int *)(CMSG_DATA(cmptr));
        newfd = *int_ptr;
        return newfd;
    }

    return (-1);
}

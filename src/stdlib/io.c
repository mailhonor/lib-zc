/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2015-11-26
 * ================================
 */

#include "zc.h"
#include <poll.h>
#include <sys/file.h>
#include <sys/ioctl.h>
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
            if (zvar_proc_stop) {
                return 0;
            }
            continue;
        case 0:
            return 0;
        default:
            revs = pollfd.revents;
            if (revs & (POLLIN|POLLOUT)) {
                return 1;
            }
            return -1;
            if (revs & POLLNVAL) {
                return -1;
            }
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
            if (zvar_proc_stop) {
                return 0;
            }
            continue;
        case 0:
            return 0;
        default:
            revs = pollfd.revents;
            if (revs & (POLLIN)) {
                return 1;
            }
            return -1;
            if (revs & POLLNVAL) {
                return -1;
            }
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
            if (zvar_proc_stop) {
                return 0;
            }
            continue;
        case 0:
            return 0;
        default:
            revs = pollfd.revents;
            if (revs & (POLLOUT)) {
                return 1;
            }
            return -1;
            if (revs & POLLNVAL) {
                return -1;
            }
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


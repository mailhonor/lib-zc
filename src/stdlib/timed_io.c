/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2015-10-12
 * ================================
 */

#include "zc.h"
#include <poll.h>
#include <errno.h>

/* timed read/write wait */
int ztimed_read_write_wait_millisecond(int fd, long read_write_wait_timeout, int *readable, int *writeable)
{
    struct pollfd pollfd;
    long stamp_timeout = ztimeout_set_millisecond(read_write_wait_timeout), left_timeout;
    int timeout;
    *readable = *writeable = 0;

    for (;;) {
        pollfd.fd = fd;
        pollfd.events = POLLIN | POLLOUT;
        if (read_write_wait_timeout < 0) {
            timeout = -1;
        } else if (read_write_wait_timeout == 0) {
            timeout = 0;
        } else {
            left_timeout = ztimeout_left_millisecond(stamp_timeout);
            if( left_timeout < 1) {
                return 0;
            } else if (left_timeout > 1000 * 3600) {
                timeout = 1000 * 3600;
            } else {
                timeout = left_timeout;
            }
        }
        switch (poll(&pollfd, 1, timeout)) {
        case -1:
            if (errno != EINTR) {
                zfatal("FATAL poll error (%m)");
            }
            continue;
        case 0:
            if (read_write_wait_timeout <= 0) {
                return 0;
            }
            continue;
        default:
            if (pollfd.revents & POLLNVAL) {
                return -1;
            }
            if (pollfd.revents & POLLIN) {
                *readable = 1;
            }
            if (pollfd.revents & POLLOUT) {
                *writeable = 1;
            }
            return 1;
        }
    }

    return 0;
}

int ztimed_read_write_wait(int fd, int read_write_wait_timeout, int *readable, int *writeable)
{
    return ztimed_read_write_wait_millisecond(fd, 1000L * read_write_wait_timeout, readable, writeable);
}

/* read */
/* readable means: 1, have readable data.
 *                 2, peer closed.
 * when receive POLLRDHUP, maybe have some readable data.
 */
int ztimed_read_wait_millisecond(int fd, long read_wait_timeout)
{
    struct pollfd pollfd;
    long stamp_timeout = ztimeout_set_millisecond(read_wait_timeout), left_timeout;
    int timeout;

    for (;;) {
        pollfd.fd = fd;
        pollfd.events = POLLIN;
        if (read_wait_timeout < 0) {
            timeout = -1;
        } else if (read_wait_timeout == 0) {
            timeout = 0;
        } else {
            left_timeout = ztimeout_left_millisecond(stamp_timeout);
            if( left_timeout < 1) {
                return 0;
            } else if (left_timeout > 1000 * 3600) {
                timeout = 1000 * 3600;
            } else {
                timeout = left_timeout;
            }
        }
        switch (poll(&pollfd, 1, timeout)) {
        case -1:
            if (errno != EINTR) {
                zfatal("FATAL poll error (%m)");
            }
            continue;
        case 0:
            if (read_wait_timeout <= 0) {
                return 0;
            }
            continue;
        default:
            if (pollfd.revents & POLLNVAL) {
                return -1;
            }
            return 1;
            if (pollfd.revents & POLLIN) {
                return 1;
            }
            if (pollfd.revents & (POLLNVAL | POLLERR | POLLHUP)) {
                return -1;
            }
            if (pollfd.revents & POLLRDHUP) {
                return -1;
            }
            return -1;
        }
    }

    return 0;
}

int ztimed_read_wait(int fd, int read_wait_timeout)
{
    return ztimed_read_wait_millisecond(fd, 1000L * read_wait_timeout);
}

int ztimed_read(int fd, void *buf, int size, int read_wait_timeout)
{
    int ret, ec;

    if (read_wait_timeout == 0) {
        for (;;) {
            if ((ret = read(fd, buf, size)) < 0) {
                if (errno == EINTR) {
                    continue;
                }
            }
            return ret;
        }
    }

    for (;;) {
        if ((ret = ztimed_read_wait_millisecond(fd, 1000L * read_wait_timeout)) == 0) {
            errno = ETIMEDOUT;
            return -1;
        }
        if ((ret = read(fd, buf, size)) < 0) {
            ec = errno;
            if (ec == EINTR) {
                continue;
            }
            if (ec == EAGAIN) {
                continue;
            }
        }
        return ret;
    }

    errno = ETIMEDOUT;
    return -1;
}

/* write */
int ztimed_write_wait_millisecond(int fd, long write_wait_timeout)
{
    struct pollfd pollfd;
    long stamp_timeout = ztimeout_set_millisecond(write_wait_timeout), left_timeout;
    int timeout;

    for (;;) {
        pollfd.fd = fd;
        pollfd.events = POLLOUT;
        if (write_wait_timeout < 0) {
            timeout = -1;
        } else if (write_wait_timeout == 0) {
            timeout = 0;
        } else {
            left_timeout = ztimeout_left_millisecond(stamp_timeout);
            if( left_timeout < 1) {
                return 0;
            } else if (left_timeout > 1000 * 3600) {
                timeout = 1000 * 3600;
            } else {
                timeout = left_timeout;
            }
        }
        switch (poll(&pollfd, 1, timeout)) {
        case -1:
            if (errno != EINTR) {
                zfatal("FATAL poll error (%m)");
            }
            continue;
        case 0:
            if (write_wait_timeout <= 0) {
                return 0;
            }
            continue;
        default:
            if (pollfd.revents & POLLNVAL) {
                return -1;
            }
            return 1;
            if (pollfd.revents & POLLOUT) {
                return 1;
            }
            if (pollfd.revents & (POLLNVAL | POLLERR | POLLHUP)) {
                return -1;
            }
            if (pollfd.revents & POLLRDHUP) {
                return -1;
            }
            return -1;
        }
    }

    return 0;
}

int ztimed_write_wait(int fd, int timeout)
{
    return ztimed_write_wait_millisecond(fd, 1000L * timeout);
}

int ztimed_write(int fd, const void *buf, int size, int write_wait_timeout)
{
    int ret, ec;

    if (write_wait_timeout == 0) {
        for (;;) {
            ret = write(fd, buf, size);
            if (ret < 0) {
                if (errno == EINTR) {
                    continue;
                }
            }
            return ret;
        }
    }
    for (;;) {
        if (ztimed_write_wait_millisecond(fd, 1000L * write_wait_timeout) == 0) {
            errno = ETIMEDOUT;
            return -1;
        }
        ret = write(fd, buf, size);
        if (ret < 0) {
            ec = errno;
            if (ec == EINTR) {
                continue;
            }
            if (ec == EAGAIN) {
                continue;
            }
            return -1;
        } else if (ret == 0) {
            continue;
        }
        return ret;
    }

    errno = ETIMEDOUT;
    return -1;
}

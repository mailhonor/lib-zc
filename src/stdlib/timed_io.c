/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2015-10-12
 * ================================
 */

#include "zc.h"
#include <poll.h>
#include <errno.h>

/* timed read/write wait */
int ztimed_read_write_wait_millisecond(int fd, long timeout, int *readable, int *writeable)
{
    struct pollfd pollfd;
    long critical_time = 0, left_time;
    *readable = *writeable = 0;
    if (timeout < 0) {
        left_time = zvar_max_timeout_millisecond;
    } else {
        left_time = timeout;
    }
    for (;left_time>=0;left_time=ztimeout_left_millisecond(critical_time)) {
        if (critical_time == 0) {
            critical_time = ztimeout_set_millisecond(left_time);
        }
        int ccc = 0;
        if (left_time > 1000 * 3600) {
            left_time = 1000 * 3600;
            ccc = 1;
        }
        pollfd.fd = fd;
        pollfd.events = POLLIN | POLLOUT;
        switch (poll(&pollfd, 1, left_time)) {
        case -1:
            if (errno != EINTR) {
                zfatal("poll error (%m)");
            }
            if (zvar_proc_stop) {
                return 0;
            }
            continue;
        case 0:
            if (ccc) {
                continue;
            }
            return 0;
        default:
            if (pollfd.revents & POLLIN) {
                *readable = 1;
            }
            if (pollfd.revents & POLLOUT) {
                *writeable = 1;
            }
            if (pollfd.revents & (POLLIN | POLLOUT)) {
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

int ztimed_read_write_wait(int fd, int timeout, int *readable, int *writeable)
{
    return ztimed_read_write_wait_millisecond(fd, 1000L * timeout, readable, writeable);
}

/* read */
/* readable means: 1, have readable data.
 *                 2, peer closed.
 * when receive POLLRDHUP, maybe have some readable data.
 */
int ztimed_read_wait_millisecond(int fd, long timeout)
{
    struct pollfd pollfd;
    long critical_time = 0, left_time;

    if (timeout < 0) {
        left_time = zvar_max_timeout_millisecond;
    } else {
        left_time = timeout;
    }
    for (;left_time>=0;left_time=ztimeout_left_millisecond(critical_time)) {
        if (critical_time == 0) {
            critical_time = ztimeout_set_millisecond(left_time);
        }
        int ccc = 0;
        if (left_time > 1000 * 3600) {
            left_time = 1000 * 3600;
            ccc = 1;
        }
        pollfd.fd = fd;
        pollfd.events = POLLIN;
        switch (poll(&pollfd, 1, left_time)) {
        case -1:
            if (errno != EINTR) {
                zfatal("poll error (%m)");
            }
            if (zvar_proc_stop) {
                return 0;
            }
            continue;
        case 0:
            if (ccc) {
                continue;
            }
            return 0;
        default:
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

int ztimed_read_wait(int fd, int timeout)
{
    return ztimed_read_wait_millisecond(fd, 1000L * timeout);
}

int ztimed_read(int fd, void *buf, int size, int timeout)
{
    int ret;
    long critical_time = 0, left_time;

    if (timeout == 0) {
        for (;;) {
            if ((ret = read(fd, buf, size)) < 0) {
                int ec = errno;
                if (ec == EINTR) {
                    if (zvar_proc_stop) {
                        break;
                    }
                    continue;
                }
                if (zvar_proc_stop) {
                    return 0;
                }
                return -1;
            }
            return ret;
        }
        return -1;
    }

    if (timeout < 0) {
        left_time = zvar_max_timeout_millisecond;
    } else {
        left_time = timeout;
    }
    for (;left_time>=0;left_time=ztimeout_left_millisecond(critical_time)) {
        if (critical_time == 0) {
            critical_time = ztimeout_set_millisecond(left_time);
        }
        if (ztimed_read_wait_millisecond(fd, left_time) < 1) {
            return (-1);
        }
        if ((ret = read(fd, buf, size)) < 0) {
            int ec = errno;
            if (ec == EINTR) {
                if (zvar_proc_stop) {
                    break;
                }
                continue;
            }
            if (ec == EAGAIN) {
                continue;
            }
            return -1;
        }
        return (ret);
    }

    return -1;
}

int ztimed_readn(int fd, void *buf, int size, int timeout)
{
    int is_closed = 0;
    int ret;
    int left;
    char *ptr;
    long critical_time = 0, left_time;

    left = size;
    ptr = (char *)buf;

    if (timeout == 0) {
        for (;;) {
            ret = read(fd, ptr, left);
            if (ret < 0) {
                int ec = errno;
                if (ec == EINTR) {
                    if (zvar_proc_stop) {
                        break;
                    }
                    continue;
                }
                break;
            } else if (ret == 0) {
                is_closed = 1;
                break;
            } else {
                left -= ret;
                ptr += ret;
            }
        }
        if (size > left) {
            return size - left;
        }
        if (is_closed) {
            return 0;
        }
        return -1;
    }
    if (timeout < 0) {
        left_time = zvar_max_timeout_millisecond;
    } else {
        left_time = timeout;
    }
    for (;(left>=0) && (left_time>=0);left_time=ztimeout_left_millisecond(critical_time)) {
        if (critical_time == 0) {
            critical_time = ztimeout_set_millisecond(left_time);
        }
        if (ztimed_read_wait_millisecond(fd, left_time) < 1) {
            break;
        }
        ret = read(fd, ptr, left);
        if (ret < 0) {
            int ec = errno;
            if (ec == EINTR) {
                if (zvar_proc_stop) {
                    break;
                }
                continue;
            }
            if (ec == EAGAIN) {
                continue;
            }
            break;
        } else if (ret == 0) {
            is_closed = 1;
            break;
        } else {
            left -= ret;
            ptr += ret;
        }
    }

    if (size > left) {
        return size - left;
    }
    if (is_closed) {
        return 0;
    }
    return -1;
}

/* write */
int ztimed_write_wait_millisecond(int fd, long timeout)
{
    struct pollfd pollfd;
    long critical_time = 0, left_time;

    if (timeout < 0) {
        left_time = zvar_max_timeout_millisecond;
    } else {
        left_time = timeout;
    }
    for (;left_time>=0;left_time=ztimeout_left_millisecond(critical_time)) {
        if (critical_time == 0) {
            critical_time = ztimeout_set_millisecond(left_time);
        }
        int ccc = 0;
        if (left_time > 1000 * 3600) {
            left_time = 1000 * 3600;
            ccc = 1;
        }
        pollfd.fd = fd;
        pollfd.events = POLLOUT;
        switch (poll(&pollfd, 1, left_time)) {
        case -1:
            if (errno != EINTR) {
                zfatal("poll error (%m)");
            }
            if (zvar_proc_stop) {
                return 0;
            }
            continue;
        case 0:
            if (ccc) {
                continue;
            }
            return 0;
        default:
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

int ztimed_write(int fd, const void *buf, int size, int timeout)
{
    int is_closed = 0;
    int ret;
    int left;
    char *ptr;
    long critical_time = 0, left_time;

    left = size;
    ptr = (char *)(void *)buf;

    if (timeout == 0) {
        for (;;) {
            ret = write(fd, ptr, left);
            if (ret < 0) {
                int ec = errno;
                if (ec == EINTR) {
                    if (zvar_proc_stop) {
                        break;
                    }
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
        if (size > left) {
            return size - left;
        }
        if (is_closed) {
            return 0;
        }
        return -1;
    }
    if (timeout < 0) {
        left_time = zvar_max_timeout_millisecond;
    } else {
        left_time = timeout;
    }
    for (;(left>=0) && (left_time>=0);left_time=ztimeout_left_millisecond(critical_time)) {
        if (critical_time == 0) {
            critical_time = ztimeout_set_millisecond(left_time);
        }
        if (ztimed_write_wait_millisecond(fd, left_time) < 1) {
            break;
        }
        ret = write(fd, ptr, left);
        if (ret < 0) {
            int ec = errno;
            if (ec == EINTR) {
                if (zvar_proc_stop) {
                    break;
                }
                continue;
            }
            if (ec == EAGAIN) {
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

    if (size > left) {
        return size - left;
    }
    if (is_closed) {
        return 0;
    }
    return -1;
}

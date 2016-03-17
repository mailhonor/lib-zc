/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-11-26
 * ================================
 */

#include "libzc.h"
#include <poll.h>
#include <sys/file.h>
#include <sys/ioctl.h>

/* ################################################################## */
/* read */
int zread_wait(int fd, int timeout)
{
    struct pollfd pollfd;

    pollfd.fd = fd;
    pollfd.events = POLLIN;
    timeout = timeout * 1000;

    for (;;)
    {
        switch (poll(&pollfd, 1, timeout))
        {
        case -1:
            if (errno != EINTR)
            {
                return -1;
            }
            continue;
        case 0:
            errno = ETIMEDOUT;
            return -1;
        default:
            if (pollfd.revents & POLLNVAL)
            {
                return -1;
            }
            return 1;
        }
    }

    return 1;
}

int ztimed_read(int fd, void *buf, int len, int timeout)
{
    int ret;
    long start_time;
    long left_time;

    start_time = ztimeout_set(timeout);

    for (;;)
    {
        left_time = ztimeout_left(start_time);
        if (left_time < 1)
        {
            return -1;
        }
        if ((ret = zread_wait(fd, left_time)) < 0)
        {
            return (ret);
        }
        if ((ret = read(fd, buf, (size_t) len)) < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            return -1;
        }
        return (ret);
    }

    return 0;
}

int ztimed_strict_read(int fd, void *buf, int len, int timeout)
{
    int ret;
    int left;
    char *ptr;
    long start_time;
    long left_time;

    left = len;
    ptr = buf;

    start_time = ztimeout_set(timeout);

    while (left > 0)
    {
        left_time = ztimeout_left(start_time);
        if (left_time < 1)
        {
            return -1;
        }
        if ((ret = zread_wait(fd, left_time)) < 0)
        {
            return -1;
        }
        ret = read(fd, ptr, (size_t) left);
        if (ret < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            return -1;
        }
        else if (ret == 0)
        {
            return -1;
        }
        else
        {
            left -= ret;
            ptr += ret;
        }
    }

    return len;
}

int ztimed_read_delimiter(int fd, void *buf, int len, int delimiter, int timeout)
{
    int ret;
    int left;
    char *ptr, *p;
    long start_time;
    long left_time;

    left = len;
    ptr = buf;

    start_time = ztimeout_set(timeout);

    while (left > 0)
    {
        left_time = ztimeout_left(start_time);
        if (left_time < 1)
        {
            return -1;
        }
        if ((ret = zread_wait(fd, left_time)) < 0)
        {
            return ret;
        }
        ret = read(fd, ptr, (size_t) left);
        if (ret < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            return -1;
        }
        else if (ret == 0)
        {
            return -1;
        }
        else
        {
            p = memchr(ptr, delimiter, ret);
            if (p)
            {
                return (len - left + (p - ptr + 1));
            }
            left -= ret;
            ptr += ret;
        }
    }

    return len;
}

/* ################################################################## */
/* write */
int zwrite_wait(int fd, int timeout)
{
    struct pollfd pollfd;

    pollfd.fd = fd;
    pollfd.events = POLLOUT;
    timeout = timeout * 1000;

    for (;;)
    {
        switch (poll(&pollfd, 1, timeout))
        {
        case -1:
            if (errno != EINTR)
            {
                return -1;
            }
            continue;
        case 0:
            errno = ETIMEDOUT;
            return -1;
        default:
            if (pollfd.revents & POLLNVAL)
            {
                return -1;
            }
            return 1;
        }
    }

    return 1;
}

int ztimed_write(int fd, void *buf, int len, int timeout)
{
    int ret;
    long start_time;
    long left_time;

    start_time = ztimeout_set(timeout);

    for (;;)
    {
        left_time = ztimeout_left(start_time);
        if (left_time < 1)
        {
            return -1;
        }
        if ((ret = zwrite_wait(fd, left_time)) < 0)
        {
            return -1;
        }
        if ((ret = write(fd, buf, (size_t) len)) < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            return -1;
        }
        return (ret);
    }

    return 0;
}

int ztimed_strict_write(int fd, void *buf, int len, int timeout)
{
    int ret;
    int left;
    char *ptr;
    long start_time;
    long left_time;

    start_time = ztimeout_set(timeout);

    left = len;
    ptr = buf;

    while (left > 0)
    {
        left_time = ztimeout_left(start_time);
        if (left_time < 1)
        {
            return -1;
        }
        if ((ret = zwrite_wait(fd, left_time)) < 0)
        {
            return -1;
        }
        ret = write(fd, ptr, (size_t) left);
        if (ret < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            return -1;
        }
        else if (ret == 0)
        {
            return -1;
        }
        else
        {
            left -= ret;
            ptr += ret;
        }
    }

    return len;
}

/* ################################################################## */
/* read or write able */
static int ___poll(int fd, int events, int flag)
{
    struct pollfd pollfd;
    int revs, myevs;

    pollfd.fd = fd;
    pollfd.events = events;
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
            return (0);
        default:
            revs = pollfd.revents;
            if (revs & POLLNVAL)
            {
                return -1;
            }
            if (flag && (revs & (POLLERR | POLLHUP | POLLRDHUP)))
            {
                return -1;
            }
            myevs = 0;
            if (revs & POLLIN)
            {
                myevs |= ZEV_READ;
            }
            if (revs & POLLOUT)
            {
                myevs |= ZEV_WRITE;
            }
            if (revs & POLLERR)
            {
                myevs |= ZEV_ERROR;
            }
            if (revs & POLLHUP)
            {
                myevs |= ZEV_HUP;
            }
            if (revs & POLLRDHUP)
            {
                myevs |= ZEV_RDHUP;
            }
            return revs;
        }
    }

    return 0;
}

int zrwable(int fd)
{
    return ___poll(fd, POLLIN | POLLOUT, 1);
}

int zreadable(int fd)
{
    return ___poll(fd, POLLIN, 1);
}

int zwriteable(int fd)
{
    return ___poll(fd, POLLIN, 1);
}

/* ################################################################## */
int zflock(int fd, int flags)
{
    int ret;
    while ((ret = flock(fd, flags)) < 0 && errno == EINTR)
        sleep(1);

    return ret;
}

int znonblocking(int fd, int on)
{
    int flags;

    if ((flags = fcntl(fd, F_GETFL, 0)) < 0)
    {
        return -1;
    }

    if (fcntl(fd, F_SETFL, on ? flags | O_NONBLOCK : flags & ~O_NONBLOCK) < 0)
    {
        return -1;
    }

    return ((flags & O_NONBLOCK) != 0);
}

int zclose_on_exec(int fd, int on)
{
    int flags;

    if ((flags = fcntl(fd, F_GETFD, 0)) < 0)
    {
        return -1;
    }

    if (fcntl(fd, F_SETFD, on ? flags | FD_CLOEXEC : flags & ~FD_CLOEXEC) < 0)
    {
        return -1;
    }

    return ((flags & FD_CLOEXEC) != 0);
}

int zpeek(int fd)
{
    int count;

    return (ioctl(fd, FIONREAD, (char *)&count) < 0 ? -1 : count);
}

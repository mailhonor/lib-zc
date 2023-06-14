/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-10-12
 * ================================
 */

#include "zc.h"
#include <errno.h>
#ifdef __linux__
#include <poll.h>
#endif // __linux__
#ifdef _WIN32
#include <Winsock2.h>
#endif // WIN32

/* timed read/write wait */
#ifdef __linux__
/* read */
/* readable means: 1, have readable data.
 *                 2, peer closed.
 * when receive POLLRDHUP, maybe have some readable data.
 */
int ztimed_read_write_wait_millisecond(int fd, long long read_write_wait_timeout, int *readable, int *writeable)
{
    struct pollfd pollfd;
    long long stamp_timeout = ztimeout_set_millisecond(read_write_wait_timeout), left_timeout;
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
        if (read_write_wait_timeout < 0)
        {
            timeout = -1;
        }
        else if (read_write_wait_timeout == 0)
        {
            timeout = 0;
        }
        else
        {
            left_timeout = ztimeout_left_millisecond(stamp_timeout);
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
            if (errno != EINTR)
            {
                zfatal("poll error (%m)");
            }
            continue;
        case 0:
            if (read_write_wait_timeout <= 0)
            {
                return 0;
            }
            continue;
        default:
            if (pollfd.revents & POLLNVAL)
            {
                return -1;
            }
            if (pollfd.revents & POLLIN)
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
#endif // __linux__

#ifdef _WIN32
int ztimed_read_write_wait_millisecond(int fd, long long read_write_wait_timeout, int *readable, int *writeable)
{
    int ec;
    long long stamp_timeout = ztimeout_set_millisecond(read_write_wait_timeout), left_timeout;
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
        if (read_write_wait_timeout < 0)
        {
            timeout = -1;
        }
        else if (read_write_wait_timeout == 0)
        {
            timeout = 0;
        }
        else
        {
            left_timeout = ztimeout_left_millisecond(stamp_timeout);
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
                ec = zget_errno();
                if (ec != EINTR)
                {
                    errno = ec;
                    return -1;
                }
                continue;
            case 0:
                if (read_write_wait_timeout <= 0)
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
#endif // _WIN32

int ztimed_read_write_wait(int fd, int read_write_wait_timeout, int *readable, int *writeable)
{
    return ztimed_read_write_wait_millisecond(fd, 1000L * read_write_wait_timeout, readable, writeable);
}

int ztimed_read_wait_millisecond(int fd, long long read_wait_timeout)
{
    int want_read;
    int ret = ztimed_read_write_wait_millisecond(fd, read_wait_timeout, &want_read, 0);
    if (ret < 1)
    {
        return ret;
    }
    return want_read;
}

int ztimed_read_wait(int fd, int read_wait_timeout)
{
    int want_read;
    int ret = ztimed_read_write_wait_millisecond(fd, 1000L * read_wait_timeout, &want_read, 0);
    if (ret < 1)
    {
        return ret;
    }
    return want_read;
}

int ztimed_read(int fd, void *buf, int size, int read_wait_timeout)
{
    int ret, ec;

    if (read_wait_timeout == 0)
    {
        for (;;)
        {
            ret = -1;
#ifdef __linux__
            ret = read(fd, buf, size);
#endif // __linux__
#ifdef _WIN32
            ret = recv(fd, buf, size, 0);
#endif // _WIN32
            if (ret < 0)
            {
                ec = zget_errno();
                if (ec == EINTR)
                {
                    continue;
                }
#ifdef _WIN32
                errno = ec;
#endif // _WIN32
            }
            return ret;
        }
    }

    for (;;)
    {
        if ((ret = ztimed_read_wait_millisecond(fd, 1000L * read_wait_timeout)) == 0)
        {
            return -1;
        }
        ret = -1;
#ifdef __linux__
        ret = read(fd, buf, size);
#endif // __linux__
#ifdef _WIN32
        ret = recv(fd, buf, size, 0);
#endif // _WIN32
        if (ret < 0)
        {
            ec = zget_errno();
            if (ec == EINTR)
            {
                continue;
            }
            if (ec == EAGAIN)
            {
                continue;
            }
#ifdef _WIN32
            errno = ec;
#endif // _WIN32
        }
        return ret;
    }
    return -1;
}

/* write */
int ztimed_write_wait_millisecond(int fd, long long write_wait_timeout)
{
    int want_write;
    int ret = ztimed_read_write_wait_millisecond(fd, write_wait_timeout, 0, &want_write);
    if (ret < 1)
    {
        return ret;
    }
    return want_write;
}

int ztimed_write_wait(int fd, int write_wait_timeout)
{
    int want_write;
    int ret = ztimed_read_write_wait_millisecond(fd, 1000L * write_wait_timeout, 0, &want_write);
    if (ret < 1)
    {
        return ret;
    }
    return want_write;
}

int ztimed_write(int fd, const void *buf, int size, int write_wait_timeout)
{
    int ret, ec;

    if (write_wait_timeout == 0)
    {
        for (;;)
        {
            ret = -1;
#ifdef __linux__
            ret = write(fd, buf, size);
#endif // __linux__
#ifdef _WIN32
            ret = send(fd, buf, size, 0);
#endif // _WIN32
            if (ret < 0)
            {
                ec = zget_errno();
                if (ec == EINTR)
                {
                    continue;
                }
#ifdef _WIN32
                errno = ec;
#endif // _WIN32
            }
            return ret;
        }
    }
    for (;;)
    {
        if (ztimed_write_wait_millisecond(fd, 1000L * write_wait_timeout) == 0)
        {
            return -1;
        }
#ifdef __linux__
        ret = write(fd, buf, size);
#endif // __linux__
#ifdef _WIN32
        ret = send(fd, buf, size, 0);
#endif // _WIN32
        if (ret < 0)
        {
            ec = zget_errno();
            if (ec == EINTR)
            {
                continue;
            }
            if (ec == EAGAIN)
            {
                continue;
            }
#ifdef _WIN32
            errno = ec;
#endif // _WIN32
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

/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-10-12
 * ================================
 */

#include "zc.h"
#ifdef __linux__
#include <poll.h>
#endif // __linux__
#include <sys/time.h>
#include <time.h>
#ifdef _WIN64
#include <timezoneapi.h>
#endif // _WIN64

ssize_t zmillisecond(void)
{
    ssize_t r;
    struct timeval tv;
    gettimeofday(&tv, 0);
    r = tv.tv_sec;
    r = r * 1000 + tv.tv_usec / 1000;
    return r;
}

void zsleep_millisecond(int delay)
{
    int left = delay;
    ssize_t end = zmillisecond() + left + 1;
    while (left > 0)
    {
#ifdef __linux__
        poll(0, 0, left);
#endif // __linux__
#ifdef _WIN64
        usleep(left * 1000);
#endif // _WIN64
        left = end - zmillisecond();
    }
}

ssize_t ztimeout_set_millisecond(ssize_t timeout)
{
    if (timeout < 0)
    {
        timeout = zvar_max_timeout_millisecond;
    }
    ssize_t r;
    struct timeval tv;
    gettimeofday(&tv, 0);
    r = tv.tv_sec;
    r = r * 1000 + tv.tv_usec / 1000;
    return (r + timeout);
}

ssize_t ztimeout_left_millisecond(ssize_t stamp)
{
    ssize_t r;
    struct timeval tv;
    gettimeofday(&tv, 0);
    r = ((ssize_t)tv.tv_sec) * 1000 + tv.tv_usec / 1000;
    ssize_t timeout = stamp - r + 1;
    if (timeout > zvar_max_timeout_millisecond)
    {
        timeout = zvar_max_timeout_millisecond;
    }
    return timeout;
}

ssize_t zsecond(void)
{
    return (ssize_t)time(0);
}

void zsleep(int delay)
{
    int left = delay * 1000;
    ssize_t end = zmillisecond() + left + 1;
    while (left > 0)
    {
#ifdef __linux__
        poll(0, 0, left);
#endif // __linux__
#ifdef _WIN64
        usleep(left * 1000);
#endif // _WIN64
        left = end - zmillisecond();
    }
}

ssize_t ztimeout_set(int timeout)
{
    if (timeout < 0)
    {
        timeout = zvar_max_timeout;
    }
    return (time(0) + timeout);
}

int ztimeout_left(ssize_t stamp)
{
    ssize_t timeout = stamp - zsecond() + 1;
    if (timeout > zvar_max_timeout)
    {
        timeout = zvar_max_timeout;
    }
    return timeout;
}

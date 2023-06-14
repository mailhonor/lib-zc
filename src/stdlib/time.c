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
#ifdef _WIN32
#include <timezoneapi.h>
#endif // _WIN32

long long zmillisecond(void)
{
    long long r;
    struct timeval tv;
    gettimeofday(&tv, 0);
    r = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    return r;
}

void zsleep_millisecond(int delay)
{
    int left = delay;
    long long end = zmillisecond() + left + 1;
    while (left > 0)
    {
#ifdef __linux__
        poll(0, 0, left);
#endif // __linux__
#ifdef _WIN32
        usleep(left * 1000);
#endif // _WIN32
        left = end - zmillisecond();
    }
}

long long ztimeout_set_millisecond(long long timeout)
{
    if (timeout < 0)
    {
        timeout = zvar_max_timeout_millisecond;
    }
    long long r;
    struct timeval tv;
    gettimeofday(&tv, 0);
    r = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    return (r + timeout);
}

long long ztimeout_left_millisecond(long long stamp)
{
    long long r;
    struct timeval tv;
    gettimeofday(&tv, 0);
    r = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    long long timeout = stamp - r + 1;
    if (timeout > zvar_max_timeout_millisecond)
    {
        timeout = zvar_max_timeout_millisecond;
    }
    return timeout;
}

long long zsecond(void)
{
    return (long)time(0);
}

void zsleep(int delay)
{
    int left = delay * 1000;
    long long end = zmillisecond() + left + 1;
    while (left > 0)
    {
#ifdef __linux__
        poll(0, 0, left);
#endif // __linux__
#ifdef _WIN32
        usleep(left * 1000);
#endif // _WIN32
        left = end - zmillisecond();
    }
}

long long ztimeout_set(int timeout)
{
    if (timeout < 0)
    {
        timeout = zvar_max_timeout;
    }
    return (time(0) + timeout);
}

int ztimeout_left(long long stamp)
{
    long long timeout = stamp - zsecond() + 1;
    if (timeout > zvar_max_timeout)
    {
        timeout = zvar_max_timeout;
    }
    return timeout;
}

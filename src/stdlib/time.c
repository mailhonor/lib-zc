/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2015-10-12
 * ================================
 */

#include "zc.h"
#include <poll.h>
#include <sys/time.h>
#include <time.h>

long zmillisecond(void)
{
    long r;
    struct timeval tv;
    gettimeofday(&tv, 0);
    r = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    return r;
}

void zsleep_millisecond(int delay)
{
    int left = delay;
    long end = zmillisecond() + left + 1;
    while (left > 0) {
        poll(0, 0, left);
        left = end - zmillisecond();
    }
}

long ztimeout_set_millisecond(long timeout)
{
    if (timeout < 0) {
        timeout = zvar_max_timeout_millisecond;
    }
    long r;
    struct timeval tv;
    gettimeofday(&tv, 0);
    r = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    return (r+timeout);
}

long ztimeout_left_millisecond(long stamp)
{
    long r;
    struct timeval tv;
    gettimeofday(&tv, 0);
    r = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    long timeout = stamp - r + 1;
    if (timeout > zvar_max_timeout_millisecond) {
        timeout = zvar_max_timeout_millisecond;
    }
    return timeout;
}

long zsecond(void)
{
    return (long) time(0);
}

void zsleep(int delay)
{
    int left = delay * 1000;
    long end = zmillisecond() + left + 1;
    while (left > 0) {
        poll(0, 0, left);
        left = end - zmillisecond();
    }
}

long ztimeout_set(int timeout)
{
    if (timeout < 0) {
        timeout = zvar_max_timeout;
    }
    return (time(0)+timeout);
}

int ztimeout_left(long stamp)
{
    long timeout = stamp - zsecond() + 1;
    if (timeout > zvar_max_timeout) {
        timeout = zvar_max_timeout;
    }
    return timeout;
}

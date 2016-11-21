/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-10-13
 * ================================
 */

#include "libzc.h"
#include <sys/time.h>
#include <time.h>

/* ################################################################## */
/* timeout millisecond`*/
long ztimeout_set(long timeout)
{
    long r;
    struct timeval tv;

    gettimeofday(&tv, 0);
    r = tv.tv_sec * 1000 + tv.tv_usec / 1000 + timeout;

    return r;
}

long ztimeout_left(long timeout)
{
    long r;
    struct timeval tv;

    gettimeofday(&tv, 0);
    r = timeout + 1 - (tv.tv_sec * 1000 + tv.tv_usec / 1000);

    return r;
}

/* ################################################################## */
/* sleep */

void zmsleep(long delay)
{
    struct timespec req, rem;

    rem.tv_sec = delay / 1000;
    rem.tv_nsec = (delay % 1000) * 1000 * 1000;

    while ((rem.tv_sec) || (rem.tv_nsec > 1000)) {
        req.tv_sec = rem.tv_sec;
        req.tv_nsec = rem.tv_nsec;
        rem.tv_sec = 0;
        rem.tv_nsec = 0;
        if ((nanosleep(&req, &rem) < 0) && (errno != EINTR)) {
            zfatal("zmsleep: nanosleep: %m");
        }
    }
}

void zsleep(long delay)
{
    zmsleep(delay * 1000);
}

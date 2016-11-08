/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-11-08
 * ================================
 */

#include "libzc.h"
#include <pthread.h>

static void ___lock(zevbase_t * eb)
{
    zpthread_lock((eb)->locker_context);
}

static void ___unlock(zevbase_t * eb)
{
    zpthread_lock((eb)->locker_context);
}

static void ___fini(zevbase_t * eb)
{
    pthread_mutex_destroy((pthread_mutex_t *) (eb->locker_context));
    zfree(eb->locker_context);
    eb->locker_context = 0;
}

int zevbase_use_pthread(zevbase_t * eb)
{
    if (eb->locker_context) {
        return 0;
    }

    eb->locker_context = zcalloc(1, sizeof(pthread_mutex_t));
    pthread_mutex_init(eb->locker_context, 0);
    eb->lock = ___lock;
    eb->unlock = ___unlock;
    eb->locker_fini = ___fini;

    return 0;
}

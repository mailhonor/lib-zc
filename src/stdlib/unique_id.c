/*
 * ================================
 * eli960@qq.com
 * www.mailhonor.com
 * 2019-01-04
 * ================================
 */

#include "zc.h"
#include <sys/time.h>
#include <sys/types.h>
#include <pthread.h>


#if defined(PTHREAD_SPINLOCK_INITIALIZER)
static pthread_spinlock_t build_unique_id_lock = PTHREAD_SPINLOCK_INITIALIZER;
#else 
static pthread_mutex_t build_unique_id_lock = PTHREAD_MUTEX_INITIALIZER;
#endif

static ssize_t build_unique_id_plus = 0;
static pid_t build_unique_id_tid = 0;

char *zbuild_unique_id(char *buf)
{
    size_t plus;
#if defined(PTHREAD_SPINLOCK_INITIALIZER)
    pthread_spin_lock(&build_unique_id_lock);
#else
    pthread_mutex_lock(&build_unique_id_lock);
#endif
    plus = build_unique_id_plus++;
    if (build_unique_id_tid == 0) {
        build_unique_id_tid = getpid();
    }
#if defined(PTHREAD_SPINLOCK_INITIALIZER)
    pthread_spin_unlock(&build_unique_id_lock);
#else
    pthread_mutex_unlock(&build_unique_id_lock);
#endif

    struct timeval tv;
    gettimeofday(&tv, 0);
    unsigned int plus2 = plus&0XFFF;
    unsigned char dec2hex[18] = "0123456789abcdef";

    zsprintf(buf, "%05x%c%zx%c%05x%c", (unsigned int)tv.tv_usec, dec2hex[(plus2>>8)],
            (size_t)tv.tv_sec, dec2hex[((plus2>>4)&0XF)], 
            ((unsigned int)build_unique_id_tid)&0XFFFFF, dec2hex[(plus2&0XF)]);

    return buf;
}

ssize_t zget_time_from_unique_id(const char *buf)
{
    ssize_t r = 0;
    for (int i = 6; i < 14; i++) {
        r = (r<<4) + buf[i] - '0';
    }
    return r;
}

zbool_t zis_unique_id(const char *buf)
{
    int i=0, ch;
    for(i=0;i<zvar_unique_id_size+1;i++) {
        ch = buf[i];
        if (ch == 0) {
            return 1;
        }
        if ((ch >= '0') &&  (ch <= '9')) {
            continue;
        }
        if ((ch >= 'a') &&  (ch <= 'f')) {
            continue;
        }
        return 0;
    }
    return 1;
}

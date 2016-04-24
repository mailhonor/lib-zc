/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-11-26
 * ================================
 */

#include "libzc.h"
#include <time.h>

int timer_cb(zevtimer_t * zt)
{
    time_t t = time(0);
    zinfo("go: %s", ctime(&t));
    zevtimer_start(zt, timer_cb, 1 * 1000);

    return 0;
}

int count = 0;
int timer_cb2(zevtimer_t * zt)
{
    if (count++ > 2)
    {
        zevtimer_fini(zt);
        zevtimer_free(zt);
        return 0;
    }
    time_t t = time(0);
    zinfo("GO: %s", ctime(&t));
    zevtimer_start(zt, timer_cb2, 2 * 1000);

    return 0;
}

int main(int argc, char **argv)
{
    zevtimer_t tm;
    zevtimer_t *tmp;

    zvar_evbase = zevbase_create();

    zevtimer_init(&tm, zvar_evbase);
    zevtimer_start(&tm, timer_cb, 1 * 1000);

    tmp = zevtimer_create();
    zevtimer_init(tmp, zvar_evbase);
    zevtimer_start(tmp, timer_cb2, 1 * 1000);

    while (1)
    {
        zevbase_dispatch(zvar_evbase, 0);
    }

    return 0;
}

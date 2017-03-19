/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-11-26
 * ================================
 */

#include "zc.h"
#include <time.h>

void timer_cb(zevtimer_t * zt)
{
    time_t t = time(0);
    zinfo("go: %s", ctime(&t));
    zevtimer_start(zt, timer_cb, 1 * 1000);
}

int count = 0;
void timer_cb2(zevtimer_t * zt)
{
    if (count++ > 2) {
        zevtimer_fini(zt);
        zevtimer_free(zt);
        return;
    }
    time_t t = time(0);
    zinfo("GO: %s", ctime(&t));
    zevtimer_start(zt, timer_cb2, 2 * 1000);
}

int main(int argc, char **argv)
{
    zevtimer_t tm;
    zevtimer_t *tmp;

    zevbase_t *evbase = zevbase_create();

    zevtimer_init(&tm, evbase);
    zevtimer_start(&tm, timer_cb, 1 * 1000);

    tmp = zevtimer_create();
    zevtimer_init(tmp, evbase);
    zevtimer_start(tmp, timer_cb2, 1 * 1000);

    while (1) {
        zevbase_dispatch(evbase, 0);
    }

    return 0;
}

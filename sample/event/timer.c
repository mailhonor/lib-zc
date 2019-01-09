/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2015-11-26
 * ================================
 */

#include "zc.h"
#include <time.h>

static void timer_cb(zetimer_t * zt)
{
    time_t t = time(0);
    zinfo("go: %s", ctime(&t));
    zetimer_start(zt, timer_cb, 1);
}

static int count = 0;
static void timer_cb2(zetimer_t * zt)
{
    if (count++ > 2) {
        zinfo("count == 2");
        zetimer_free(zt);
        return;
    }
    time_t t = time(0);
    zinfo("GO: %s", ctime(&t));
    zetimer_start(zt, timer_cb2, 2);
}

int main(int argc, char **argv)
{
    zevent_base_t *evbase = zevent_base_create();

    zetimer_start(zetimer_create(evbase), timer_cb, 1);

    zetimer_start(zetimer_create(evbase), timer_cb2, 1);

    while(zevent_base_dispatch(evbase)) {
    }

    return 0;
}

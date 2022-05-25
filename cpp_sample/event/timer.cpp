/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2015-11-26
 * ================================
 */

#include "zc.h"
#include <time.h>

typedef struct info_t info_t;
struct info_t {
    int id;
    int count;
};

static char *get_current_time(char *tbuf)
{
    time_t t = time(0);
    ctime_r(&t, tbuf);
    char *p = strchr(tbuf, '\n');
    if (p) {
        *p = 0;
    }
    return tbuf;
}

static void timer_exit(zcc::aio_base *ab)
{
    ab->stop_notify();
}

static void timer_cb(zcc::aio *tm, info_t *info)
{
    info->count ++;
    char tbuf[128];
    zinfo("go%d count:%02d, time:%s", info->id, info->count, get_current_time(tbuf));
    if (info->count == 10) {
        if (info->id == 1) {
            zinfo("go%d count == 10, exit after 2s", info->id);
            tm->get_aio_base()->timer(std::bind(timer_exit, tm->get_aio_base()), 2);
        }
        zfree(info);
        delete tm;
        return;
    }
    tm->sleep(std::bind(timer_cb, tm, info), 1);
}

int main(int argc, char **argv)
{
    zmain_argument_run(argc, argv);

    zcc::aio_base ab;

    for (int id = 1; id < 3; id++) {
        zcc::aio *aio = new zcc::aio(-1, &ab);
        info_t *info = (info_t *)zcalloc(1, sizeof(info_t));
        info->id = id;
        aio->sleep(std::bind(timer_cb, aio, info), 1);
    }

    ab.run();

    return 0;
}

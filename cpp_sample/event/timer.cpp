/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-11-26
 * ================================
 */

#include "zcc/zcc_aio.h"
#include <time.h>

struct info_t
{
    int id;
    int count;
};

static char *get_current_time(char *tbuf)
{
    time_t t = time(0);
    ctime_r(&t, tbuf);
    char *p = strchr(tbuf, '\n');
    if (p)
    {
        *p = 0;
    }
    return tbuf;
}

static void timer_exit(zcc::aio_base *ab)
{
    ab->stop_notify();
}

static void timer_cb(zcc::aio_timer *tm, info_t *info)
{
    info->count++;
    char tbuf[128];
    zcc_info("go%d count:%02d, time:%s", info->id, info->count, get_current_time(tbuf));
    if (info->count == 10)
    {
        if (info->id == 1)
        {
            zcc_info("go%d count == 10, exit after 2s", info->id);
            tm->get_aio_base()->enter_timer(std::bind(timer_exit, tm->get_aio_base()), 2);
        }
        delete info;
        delete tm;
        return;
    }
    tm->sleep(std::bind(timer_cb, tm, info), 1);
}

int main(int argc, char **argv)
{
    zcc::main_argument::run(argc, argv);

    zcc::aio_base ab;

    for (int id = 1; id < 3; id++)
    {
        zcc::aio_timer *timer = new zcc::aio_timer(&ab);
        info_t *info = new info_t();
        info->id = id;
        info->count = 0;
        timer->after(std::bind(timer_cb, timer, info), 1);
    }

    ab.run();

    return 0;
}

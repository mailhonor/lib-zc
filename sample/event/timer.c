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

static zaio_t *tm2;
static void timer_cb(zaio_t *tm)
{
    info_t *info = (info_t *)zaio_get_context(tm);
    info->count ++;
    char tbuf[128];
    zinfo("go%d count:%02d, time:%s", info->id, info->count, get_current_time(tbuf));
    if (info->id==1 && info->count == 20) {
        zinfo("count == 20, exit");
        zaio_base_stop_notify(zaio_get_aio_base(tm));
        zfree(zaio_get_context(tm));
        zaio_free(tm, 1);
        zfree(zaio_get_context(tm2));
        zaio_free(tm2, 1);
        return;
    }
    zaio_sleep(tm, timer_cb, info->id);
}

int main(int argc, char **argv)
{
    zmain_argument_run(argc, argv, 0);

    zaio_base_t *base = zaio_base_create();

    {
        zaio_t *tm = zaio_create(-1, base);
        info_t *info = (info_t *)zcalloc(1, sizeof(info_t));
        info->id = 1;
        zaio_set_context(tm, info);
        zaio_sleep(tm, timer_cb, 1);
    }

    {
        zaio_t *tm = zaio_create(-1, base);
        info_t *info = (info_t *)zcalloc(1, sizeof(info_t));
        info->id = 2;
        zaio_set_context(tm, info);
        zaio_sleep(tm, timer_cb, 1);
        tm2 = tm;
    }

    zaio_base_run(base, 0);
    zaio_base_free(base);

    return 0;
}

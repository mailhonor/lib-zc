/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2021-03-19
 * ================================
 */


#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"

#include "zc.h"

static void usage(void)
{
    zprintf("USAGE: %s min_pthread_count max_pthread_count idle_timeout\n", zvar_progname);
    exit(1);
}

static void job1(void *ctx)
{
    int i = (int)(ssize_t)ctx;
    zsleep_millisecond(100);
    zsleep_millisecond(10000);
    zprintf("job1, %d\n", i);
}

static void timer1(void *ctx)
{
    int i = (int)(ssize_t)ctx;
    zprintf("timer1, %d\n", i);
}

int main(int argc, char **argv)
{
    zmain_argument_run(argc, argv);
    if (zvar_main_redundant_argc != 3) {
        usage();
    }
    int min = atoi(zvar_main_redundant_argv[0]);
    int max = atoi(zvar_main_redundant_argv[1]);
    int idle = atoi(zvar_main_redundant_argv[2]);

    zpthread_pool_t *ptp = zpthread_pool_create();
    zpthread_pool_set_debug_flag(ptp, 1);
    zpthread_pool_set_min_max_count(ptp, min, max);
    zpthread_pool_set_idle_timeout(ptp, idle);
    zpthread_pool_start(ptp);

    for (int i = 0; i < 100; i++) {
        zpthread_pool_job(ptp, job1, (void *)(ssize_t)i);
    }

    for (int i = 0; i < 100; i++) {
        zpthread_pool_timer(ptp, timer1, (void *)(ssize_t)i, i/10 + 1);
    }

    zsleep(5);
    zprintf("running max second: %zd\n", zpthread_pool_get_max_running_millisecond(ptp) / 1000);

    zsleep(10 + 1);
    zprintf("wait idle pthread quit\n");
    zsleep(10 + 1);
    zprintf("soft stop\n");
    zsleep(1 + 1);
    zprintf("running max second: %zd\n", zpthread_pool_get_max_running_millisecond(ptp) / 1000);
    zpthread_pool_softstop(ptp);
    zpthread_pool_wait_all_stopped(ptp, 2);
    zpthread_pool_free(ptp);

    return 0;
}

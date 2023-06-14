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
    printf("USAGE: %s min_pthread_count max_pthread_count idle_timeout\n", zvar_progname);
    exit(1);
}

static void timer1(void *ctx)
{
    int i = (int)(long)ctx;
    printf("timer1, %d\n", i);
    sleep(3);
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
        zpthread_pool_timer(ptp, timer1, (void *)(long)i, i/10 + 1);
    }

    sleep(1000);
    zpthread_pool_softstop(ptp);
    zpthread_pool_wait_all_stopped(ptp, 2);
    zpthread_pool_free(ptp);

    return 0;
}

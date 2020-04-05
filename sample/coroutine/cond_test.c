/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2017-07-02
 * ================================
 */

#include "coroutine.h"

#include <stdio.h>
#include <unistd.h>
 
zcoroutine_cond_t *cond;
zcoroutine_mutex_t *mutex;
static void *wait_cond_then_do_something(void *context)
{
    for (int i = 0; i < 2; i++) {
        /* zcoroutine_mutex_lock(mutex); */
        zcoroutine_cond_wait(cond, mutex);
        printf("get signal, id=%d, loop=%d\n", (int)(long)context, i+1);
        /* zcoroutine_mutex_unlock(mutex); */
    }
    /* should execute unlock, here */
    /* auto release all mutexs when a coroutine exit */
    return context;
}

static void *begin_my_test(void *context)
{
    cond  = zcoroutine_cond_create();
    mutex  = zcoroutine_mutex_create();
    printf("start 10 coroutine to wait cond.\n");
    for (int i = 0; i < 10; i++) {
        zcoroutine_go(wait_cond_then_do_something, (void *)(long)i, 0);
    }
    printf("waiting 1 second ...\n");

    printf("\n\ncoroutine_cond_signal, then sleep 1s  ... \n");
    zcoroutine_cond_signal(cond);
    sleep(1);

    printf("\n\ncoroutine_cond_broadcast ... \n");
    zcoroutine_cond_broadcast(cond);

    printf("\n\ncoroutine_cond_broadcast, then sleep 1s ... \n");
    zcoroutine_cond_broadcast(cond);
    sleep(1);

    printf("\n\ncoroutine_cond_broadcast ... \n");
    zcoroutine_cond_broadcast(cond);

    printf("\n\ncoroutine_cond_free, coroutine_mutex_free then sleep 1s ... \n");
    zcoroutine_cond_free(cond);
    zcoroutine_mutex_free(mutex);
    sleep(1);

    printf("\n\nnotify stop ... \n");
    zcoroutine_base_stop_notify(0);

    return 0;
}

int main(int argc, char **argv)
{
    zcoroutine_base_init();
    zcoroutine_go(begin_my_test, 0, 0);
    zcoroutine_base_run(0);
    printf("\n\nzcoroutine_base_run over, then zcoroutine_base_fini \n");
    zcoroutine_base_fini();
    printf("\n");
    return 0;
}

/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-07-02
 * ================================
 */

#include "zc.h"
 
static void *test_sleep1(void *context)
{
    while(1){
        zcoroutine_sleep_millisecond(1000);
        printf("sleep coroutine_msleep, 1 * 1000(ms)\n");
    }
    return context;
}

static void *test_sleep2(void *context)
{
    while(1){
        sleep(10);
        printf("sleep system sleep, 10 * 1000(ms)\n");
    }
    return context;
}

int main(int argc, char **argv)
{
    zcoroutine_base_init();
    zcoroutine_go(test_sleep1, 0, 0);
    zcoroutine_go(test_sleep2, 0, 0);
    zcoroutine_base_run();
    zcoroutine_base_fini();
    return 0;
}

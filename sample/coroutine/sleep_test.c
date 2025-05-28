/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2017-07-02
 * ================================
 */

#include "coroutine.h"

#include <stdio.h>
#include <unistd.h>
 
static void *test_sleep1(void *context)
{
    while(1){
        sleep(1);
        zcoroutine_sleep_millisecond(10);
        printf("test_sleep1, 1s + 10ms \n");
    }
    return context;
}

static void *test_sleep2(void *context)
{
    while(1){
        sleep(2);
        zcoroutine_sleep_millisecond(300);
        printf("test_sleep2, 2s + 300ms \n");
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

/*
 * ================================
 * eli960@qq.com
 * http://linxumail.cn/
 * 2020-01-02
 * ================================
 */

#include "coroutine.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syscall.h>

static pid_t my_gettid(void)
{
    return syscall(__NR_gettid);
}

static void *slow_action(void *arg)
{
    /* 做一些不适合在协程环境做的事情:
     *     cpu密集型
     *     无法协程化,而导致阻塞的
     *     阻塞类操作
     *     io频繁切换, 如 操作本地数据库, sqlite3, bdb
     */

    printf("my pthread:%ld\n", (long)my_gettid());
    sleep(1);
    return 0;
}

static void *one_coroutine(void *arg)
{
    for (int i=0;i < 100; i++) {
        zcoroutine_block_do(slow_action, 0);
    }
    return 0;
}

static void *timer_exit(void *arg)
{
    sleep(100);
    exit(1);
    return 0;
}

int main(int argc, char **argv)
{
    int i;
    zcoroutine_base_init();

    zvar_coroutine_block_pthread_count_limit = 3;
    zvar_coroutine_fileio_use_block_pthread = 1;

    for (i=0;i<10;i++) {
        zcoroutine_go(one_coroutine, 0, 0);
    }
    zcoroutine_go(timer_exit, 0, 0);

    printf("exit after 100s\n");
    printf("block do running in worker pthread\n");
    printf("strace -p pthrad_id\n");
    zcoroutine_base_run();
    zcoroutine_base_fini();
    sleep(1);
    return 0;
}

/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2018-07-10
 * ================================
 */

#include "coroutine.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <fcntl.h>

static int co_count = 0;
static void *file_do(void *arg)
{
    char *fn = "a.txt";
    int fd = open(fn, O_RDWR|O_CREAT|O_TRUNC, 0777);
    if (fd == -1) {
        fprintf(stderr, "can not open %s(%m)\n", fn);
        return 0;
    }
    for (int i=0;i < 100; i++) {
        if (write(fd, "0123456789", 8) != 8) {
            fprintf(stderr, "write error\n");
        }
        if (lseek(fd, 0, SEEK_SET) == (off_t)-1) {
            fprintf(stderr, "lseek error (%m)");
        }
        sleep(1);
    }
    close(fd);
    co_count--;
    if (co_count == 0) {
        fprintf(stderr, "zcoroutine_base_stop_notify\n");
        zcoroutine_base_stop_notify(0);
    }
    return 0;
}

int main(int argc, char **argv)
{
    int i;
    zcoroutine_base_init();

    zvar_coroutine_block_pthread_count_limit = 3;
    zvar_coroutine_fileio_use_block_pthread = 1;

    co_count = 10;
    for (i=0;i<co_count;i++) {
        zcoroutine_go(file_do, 0, 0);
    }
    printf("exit after 100s\n");
    printf("file io running in worker pthread\n");
    printf("strace -p pthrad_id\n");
    zcoroutine_base_run();
    zcoroutine_base_fini();
    sleep(1);
    return 0;
}

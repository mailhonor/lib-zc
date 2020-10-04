/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2018-07-12
 * ================================
 */

#include "coroutine.h"

#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int runover = 0;
static void *foo(void *arg)
{
    for (int i = 0; i < 10; i++) {
        struct hostent *hp = gethostbyname((char *)arg);
        sleep(1);
        if (hp) {
            printf("%s: %s\n", (char *)arg, hp->h_name);
        } else {
            printf("%s: not found\n", (char *)arg);
        }
    }
    runover++;
    return 0;
}

static void *foo2(void *arg) 
{
    while(runover!=2) {
        sleep(1);
    }
    return 0;
}

int main(int argc, char **argv)
{
    if (argc == 1) {
        printf("%s domain1 domain2\n", argv[0]);
        return 0;
    }
    zcoroutine_base_init();
    zcoroutine_go(foo, argv[1], 0);
    zcoroutine_go(foo, argv[2], 0);
    zcoroutine_go(foo2, 0, 0);
    zcoroutine_base_run(0);
    zcoroutine_base_fini();
    return 0;
}

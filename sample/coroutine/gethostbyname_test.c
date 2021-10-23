/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2018-07-12
 * ================================
 */

#include "coroutine.h"

#include <resolv.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static int instances = 10;
static int test_type = 0;
static void *foo(void *arg)
{
    const char *name = (char *)arg;
    char answer[1024+1];
#ifdef res_nquery
    struct __res_state state;
    if (test_type == 2) {
        res_ninit(&state);
    }
#endif

    for (int i = 0; i < 10; i++) {
        if ((i == 3) || (i == 6)) {
            sleep(1);
        }
        if (test_type == 0) {
            struct hostent *hp = gethostbyname(name);
            if (hp) {
                printf("%s: %s\n", name, hp->h_name);
            } else {
                printf("%s: not found\n", name);
            }
        }
        if (test_type == 1) {
            int ret = res_query(name, C_IN, T_A, (unsigned char *)answer, 1024);
            printf("res_query %s: %d\n", name, ret);
        }
#ifdef res_nquery
        if (test_type == 2) {
            int ret = __res_nquery(&state, name, C_IN, T_A, (unsigned char *)answer, 1024);
            printf("res_nquery %s: %d\n", name, ret);
        }
#endif
    }
#ifdef res_nquery
    if (test_type == 2) {
        res_nclose(&state);
    }
#endif
    instances--;
    return 0;
}

static void *foo2(void *arg) 
{
    while(instances) {
        sleep(1);
    }
    sleep(1);
    zcoroutine_base_stop_notify(0);
    return 0;
}

static int _init(int argc, char **argv)
{
    printf("USAGE: \n%s domain1 domain2 [ test_type ] [ disable_udp]\n", argv[0]);
    printf("  test_type:\n");
    printf("    0\t\t # gethostbyname(default)\n");
    printf("    1\t\t # res_query\n");
#ifdef res_nquery
    printf("    2\t\t # res_nquery\n");
#else
    printf("    2\t\t # res_nquery, unsupported\n");
#endif
    printf("  disable_udp:\n");
    printf("    udp\t\t # disable all udp coroutine swap\n");
    printf("    53\t\t # disable udp(53) coroutine swap\n");

    if (argc < 3) {
        return 0;
    }

    printf("\n");
    int type_show = 0;
    for (int i = 3; i < argc; i++) {
        const char *s = argv[i];
        if (!strcmp(s, "0")) {
            printf("######## test gethostbyname\n");
            type_show = 1;
            test_type = 0;
        } else if (!strcmp(s, "1")) {
            printf("######## test req_query\n");
            type_show = 1;
            test_type = 1;
        } else if (!strcmp(s, "2")) {
            printf("######## test req_nquery\n");
            type_show = 1;
            test_type = 2;
        } else if (!strcmp(s, "udp")) {
            zvar_coroutine_disable_udp = 1;
            printf("######## disable all udp\n");
        } else if (!strcmp(s, "53")) {
            zvar_coroutine_disable_udp_53 = 1;
            printf("######## disable udp(53)\n");
        } else {
            printf("######## unknown %s\n", s);
        }
    }
    if (type_show == 0) {
        printf("######## test gethostbyname, default\n");
    }
#ifndef res_nquery
    printf("######## res_nquery unsupported in your system\n");
#endif
    printf("\n");
    return 1;
}

int main(int argc, char **argv)
{
    if (_init(argc, argv) == 0) {
        return 0;
    }
    zcoroutine_base_init();
    for (int i = 0; i < instances/2; i++) {
        zcoroutine_go(foo, argv[1], 0);
        zcoroutine_go(foo, argv[2], 0);
    }
    zcoroutine_go(foo2, 0, 0);
    zcoroutine_base_run();
    zcoroutine_base_fini();
    return 0;
}

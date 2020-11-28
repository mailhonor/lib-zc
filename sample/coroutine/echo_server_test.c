/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2017-06-26
 * ================================
 */

#include "coroutine.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

static int test_listen(int port)
{
    int sock;
    int on = 1;
    struct sockaddr_in addr;
    int errno2;

    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr("0");

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("ERR socket(%m)\n");
        return -1;
    }

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0) {
        printf("ERR getsockopt(%m)\n");
        goto err;
    }


    if (bind(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0) {
        printf("ERR bind(%m)\n");
        goto err;
    }

    if (listen(sock, 5) < 0) {
        printf("ERR listen(%m)\n");
        goto err;
    }

    return (sock);

err:
    errno2 = errno;
    close(sock);
    errno = errno2;

    printf("ERR listen on 0:%d\n", port);
    exit(1);

    return -1;
}

static void *echo_service(void *context)
{
    int fd = (int)(long)context;
    char buf[10240+1];
    int ret;

    while((ret = read(fd, buf, 10240)) > 0) {
        buf[ret] = 0;
        printf("your input: %s", buf);
        write(fd, buf, ret);
        if ((!strncasecmp(buf, "exit", 4)) || (!strncasecmp(buf, "quit", 4))) {
            break;
        }
    }

    return 0;
}

void *do_listen(void *context)
{
    int fd;
    int sock = test_listen(atoi(context));

    struct sockaddr_storage sa;
    socklen_t slen = sizeof(struct sockaddr_storage);

    while(1) {
        if ((fd = accept(sock, (struct sockaddr *)&sa, &slen)) < 0) {
            if (errno == EINTR) {
                continue;
            }
            printf("ERR accept(%m)");
            exit(1);
        }
        zcoroutine_go(echo_service, (void *)((long)fd), 0);
    }

    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        printf("usage %s port\n", argv[0]);
        return -1;
    }
    zcoroutine_base_init();
    zcoroutine_go(do_listen, argv[1], 0);
    zcoroutine_base_run(0);
    zcoroutine_base_fini();
    return 0;
}

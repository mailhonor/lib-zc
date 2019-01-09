/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2016-04-22
 * ================================
 */

#include "zc.h"
#include <pthread.h>
#include <time.h>

static zevent_base_t *evbase;
static void after_write(zaio_t * aio);
static void service_error(zaio_t * aio)
{
    int fd = zaio_get_fd(aio);
    zinfo("%d: error or idle too long", fd);
    zaio_free(aio, 1);
}

static void before_write(zaio_t * aio)
{
    zaio_cache_printf_1024(aio, "AAAAAAAAA:%lu\n", time(0));
    zaio_cache_flush(aio, after_write, 2);
}

static void after_read(zaio_t * aio)
{
    int ret;
    ZSTACK_BUF(bf, 10240);

    ret = zaio_get_result(aio);
    if (ret < 1) {
        service_error(aio);
        return;
    }
    zaio_fetch_rbuf(aio, bf, ret);
    zaio_sleep(aio, before_write, 1);
}

static void after_write(zaio_t * aio)
{
    if (zaio_get_result(aio) < 1) {
        return service_error(aio);
    }

    zaio_gets(aio, 1024, after_read, 10);
}

int limit = 100;

void *connect_action(void *arg)
{
    int i;
    int port = 8899;
    int err_times = 0;

    pthread_detach(pthread_self());

    sleep(3);
    printf("\n");
    for (i = 0; i < limit; i++) {
        int fd = zinet_connect("127.0.0.1", port, 1, 1);
        if (fd < 0) {
            err_times++;
            continue;
        }
        zaio_t *aio = zaio_create(fd, evbase);
        zaio_gets(aio, 1024, after_read, 10);
        printf("%d\r", fd);
    }
    printf("\n");
    printf("err_times: %d\n", err_times);

    return arg;
}

int main(int argc, char **argv)
{
    if (argc > 1) {
        limit = atoi(argv[1]);
        if (limit < 1) {
            limit = 1;
        } else if (limit > 10) {
            limit = 10;
        }
    }
    evbase = zevent_base_create();

    pthread_t pth;
    pthread_create(&pth, 0, connect_action, 0);

    while(zevent_base_dispatch(evbase)) {
    }

    return 0;
}

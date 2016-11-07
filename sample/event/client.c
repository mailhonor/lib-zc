/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-04-22
 * ================================
 */

#include "libzc.h"
#include <time.h>

static int after_write(zaio_t * aio);
static int service_error(zaio_t * aio)
{
    int fd;

    fd = zaio_get_fd(aio);

    zinfo("%d: error or idle too long", fd);
    zaio_fini(aio);
    zaio_free(aio);
    close(fd);

    return -1;
}

static int before_write(zaio_t * aio)
{
    zaio_printf(aio, "AAAAAAAAA:%lu\n", time(0));
    zaio_write_cache_flush(aio, after_write, 1 * 1000);

    return 0;
}

static int after_read(zaio_t * aio)
{
    int ret;
    char rbuf[102400];

    ret = zaio_get_ret(aio);
    if (ret < 1) {
        return service_error(aio);
    }
    zaio_fetch_rbuf(aio, rbuf, ret);

    zaio_sleep(aio, before_write, 100);

    return 0;
}

static int after_write(zaio_t * aio)
{
    int ret;

    ret = zaio_get_ret(aio);

    if (ret < 1) {
        return service_error(aio);
    }

    zaio_read_delimiter(aio, '\n', 1024, after_read, 10 * 1000);

    return 0;
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
        int fd = zinet_connect("127.0.0.1", port, 1000);
        if (fd < 0) {
            err_times++;
            continue;
        }
        znonblocking(fd, 1);
        zaio_t *aio = zaio_create();
        zaio_init(aio, zvar_evbase, fd);
        zaio_read_delimiter(aio, '\n', 1024, after_read, 10 * 1000);
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
        } else if (limit > 10 * 10000) {
            limit = 10 * 10000;
        }
    }
    zvar_evbase = zevbase_create();

    pthread_t pth;
    pthread_create(&pth, 0, connect_action, 0);

    while (1) {
        zevbase_dispatch(zvar_evbase, 0);
    }

    return 0;
}

/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-11-24
 * ================================
 */

#include "libzc.h"
#include <time.h>

static int after_write(zaio_t * aio);
static int service_error(zaio_t * aio)
{
    printf("service_error\n");
    int fd;

    fd = zaio_get_fd(aio);

    zinfo("%d: error or idle too long", fd);
    zaio_fini(aio);
    zaio_free(aio);
    close(fd);

    return -1;
}

static int after_read(zaio_t * aio)
{
    printf("after_read\n");
    int ret, fd, len;
    char rbuf[102400];
    char *p;

    ret = zaio_get_ret(aio);
    fd = zaio_get_fd(aio);
    if (ret < 1) {
        return service_error(aio);
    }
    zaio_fetch_rbuf(aio, rbuf, ret);

    if (ret > 3 && !strncmp(rbuf, "exit", 4)) {
        zaio_fini(aio);
        zaio_free(aio);
        close(fd);
        return 0;
    }

    rbuf[ret] = 0;
    p = strchr(rbuf, '\r');
    if (p) {
        *p = 0;
    }
    p = strchr(rbuf, '\n');
    if (p) {
        *p = 0;
    }
    len = strlen(rbuf);

    zaio_write_cache_append(aio, "your input:   ", 12);
    zaio_write_cache_append(aio, rbuf, len);
    zaio_write_cache_append(aio, "\n", 1);
    zaio_write_cache_flush(aio, after_write, 1000);

    return 0;
}

static int after_write(zaio_t * aio)
{
    int ret;
    printf("before_write\n");

    ret = zaio_get_ret(aio);

    if (ret < 1) {
        return service_error(aio);
    }

    zaio_read_delimiter(aio, '\n', 1024, after_read, 10 * 1000);

    return 0;
}

static void welcome(zaio_t * aio)
{
    time_t t = time(0);

    zaio_printf_1024(aio, "welcome aio: %s\n", ctime(&t));
    zaio_write_cache_flush(aio, after_write, 1000);
}

static int before_accept(zev_t * ev)
{
    printf("before_accept\n");
    int sock;
    int fd;
    zaio_t *aio;

    sock = zev_get_fd(ev);
    fd = zinet_accept(sock);
    if (fd < -1) {
        printf("accept fail\n");
        return 0;
    }
    znonblocking(fd, 1);
    aio = zaio_create();
    zaio_init(aio, zvar_evbase, fd);

    welcome(aio);

    return 0;
}

static int timer_cb(zevtimer_t * zt)
{
    zinfo("now exit!");
    exit(1);

    return 0;
}

int main(int argc, char **argv)
{
    int port;
    int sock;
    zev_t *ev;
    zevtimer_t tm;

    port = 8899;
    zvar_evbase = zevbase_create();

    sock = zinet_listen(0, port, 5);
    znonblocking(sock, 1);
    ev = zev_create();
    zev_init(ev, zvar_evbase, sock);
    zev_read(ev, before_accept);

    zevtimer_init(&tm, zvar_evbase);
    zevtimer_start(&tm, timer_cb, 200 * 1000);

    while (1) {
        zevbase_dispatch(zvar_evbase, 0);
    }

    return 0;
}

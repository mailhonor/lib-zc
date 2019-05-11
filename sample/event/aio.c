/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2015-11-24
 * ================================
 */

#include "zc.h"
#include <time.h>

static zevent_base_t *evbase;
static void after_write(zaio_t * aio);
static void service_error(zaio_t * aio)
{
    zinfo("%d: error or idle too long", zaio_get_fd(aio));
    zaio_free(aio, 1);
}

static void after_read(zaio_t * aio)
{
    printf("after_read\n");
    ZSTACK_BUF(bf, 10240);

    int ret = zaio_get_result(aio);
    if (ret < 1) {
        service_error(aio);
        return;
    }
    zaio_fetch_rbuf(aio, bf, ret);

    if (ret > 3 && !strncmp(zbuf_data(bf), "exit", 4)) {
        zaio_free(aio, 1);
        return;
    }
    zbuf_trim_right_rn(bf);

    zaio_cache_puts(aio, "your input:   ");
    zaio_cache_write(aio, zbuf_data(bf), zbuf_len(bf));
    zaio_cache_write(aio, "\n", 1);
    zaio_cache_flush(aio, after_write, 1);
}

static void after_write(zaio_t * aio)
{
    int ret;
    printf("before_write\n");

    ret = zaio_get_result(aio);

    if (ret < 1) {
        return service_error(aio);
    }

    zaio_gets(aio, 1024, after_read, 10 * 1000);
}

static void welcome(zaio_t * aio)
{
    time_t t = time(0);

    zaio_cache_printf_1024(aio, "welcome aio: %s\n", ctime(&t));
    zaio_cache_flush(aio, after_write, 1000);
}

static void before_accept(zeio_t * ev)
{
    printf("before_accept\n");
    int sock;
    int fd;
    zaio_t *aio;

    sock = zeio_get_fd(ev);
    fd = zinet_accept(sock);
    if (fd < -1) {
        printf("accept fail\n");
        return;
    }
    znonblocking(fd, 1);
    aio = zaio_create(fd, evbase);
    welcome(aio);
}

static void timer_cb(zetimer_t * zt)
{
    zinfo("now exit!");
    exit(1);
}

int main(int argc, char **argv)
{
    int port;
    int sock;
    zeio_t *ev;

    port = 8899;
    evbase = zevent_base_create();

    sock = zinet_listen(0, port, 5, 1);
    ev = zeio_create(sock, evbase);
    zeio_enable_read(ev, before_accept);

    zetimer_start(zetimer_create(evbase), timer_cb, 200);
    
    while(zevent_base_dispatch(evbase)) {
    }

    return 0;
}

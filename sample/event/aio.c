/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-11-24
 * ================================
 */

#include "libzc.h"
#include <time.h>


static int after_write(zaio_t * aio, char *wbuf);
static int service_error(zaio_t * aio)
{
    int fd;

    fd = zaio_get_fd(aio);

    zinfo("%d: error or idle too long", fd);
    zaio_fini(aio);
    zfree(aio);
    close(fd);

    return -1;
}

static int after_read(zaio_t * aio, char *rbuf)
{
    int ret, fd, len;
    char *p;

    ret = zaio_get_ret(aio);
    fd = zaio_get_fd(aio);
    if (ret < 1)
    {
        return service_error(aio);
    }

    if (ret > 3 && !strncmp(rbuf, "exit", 4))
    {
        zaio_fini(aio);
        zfree(aio);
        close(fd);
        return 0;
    }

    rbuf[ret] = 0;
    p = strchr(rbuf, '\r');
    if (p)
    {
        *p = 0;
    }
    p = strchr(rbuf, '\n');
    if (p)
    {
        *p = 0;
    }
    len = strlen(rbuf);

    zaio_write_cache_append(aio, "your input:   ", 12);
    zaio_write_cache_append(aio, rbuf, len);
    zaio_write_cache_append(aio, "\n", 1);
    zaio_write_cache_flush(aio, after_write);

    return 0;
}

static int after_write(zaio_t * aio, char *wbuf)
{
    int ret;

    ret = zaio_get_ret(aio);

    if (ret < 1)
    {
        return service_error(aio);
    }

    zaio_read_delimiter(aio, '\n', 1024, after_read);

    return 0;
}

static int attach_to_evbase(zaio_t * aio, char *unused)
{
    char buf[1024];
    time_t t = time(0);

    sprintf(buf, "welcome aio: %s\n", ctime(&t));
    zaio_write_cache_append(aio, buf, strlen(buf));
    zaio_write_cache_flush(aio, after_write);

    return 0;
}

static int before_accept(zev_t * ev)
{
    int sock = zev_get_fd(ev);
    int fd = zinet_accept(sock);
    zaio_t *aio = zaio_create();

    if (fd < -1)
    {
        printf("accept fail\n");
        return 0;
    }
    znonblocking(fd, 1);
    zaio_init(aio, zvar_evbase, fd);
    zaio_attach(aio, attach_to_evbase);

    return 0;
}

static int timer_cb(ztimer_t * zt)
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
    ztimer_t tm;

    port = 8899;
    zvar_evbase = zevbase_create();

    sock = zinet_listen(0, port, 5);
    znonblocking(sock, 1);
    ev = zev_create();
    zev_init(ev, zvar_evbase, sock);
    zev_read(ev, before_accept);

    ztimer_init(&tm, zvar_evbase);
    ztimer_start(&tm, timer_cb, 200 * 1000);

    while (1)
    {
        zevbase_dispatch(zvar_evbase, 0);
    }

    return 0;
}

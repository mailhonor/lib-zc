/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-03-15
 * ================================
 */

#include "libzc.h"
#include <time.h>

static int server_error(zev_t * ev)
{
    int events, fd;
    zstream_t *fp;

    events = zev_get_events(ev);
    fd = zev_get_fd(ev);
    fp = zev_get_context(ev);

    if (events & ZEV_EXCEPTION) {
        zinfo("%d: error", fd);
    }

    zev_fini(ev);
    zfree(ev);
    if (fp) {
        zstream_close_FD(fp);
    }
    close(fd);

    return 0;
}

static int server_read(zev_t * ev)
{
    int fd, events, ret;
    char buf[1024], *p;
    zstream_t *fp;

    events = zev_get_events(ev);
    fd = zev_get_fd(ev);

    if (events & ZEV_EXCEPTION) {
        server_error(ev);
        return 1;
    }

    fp = (zstream_t *) zev_get_context(ev);
    ret = zstream_read_line(fp, buf, 1024);

    if (ret < 0) {
        server_error(ev);
        return -1;
    }
    buf[ret] = 0;
    if (!strncmp(buf, "exit", 4)) {
        zstream_close_FD(fp);
        zev_fini(ev);
        zfree(ev);
        close(fd);

        return 0;
    }
    p = strchr(buf, '\r');
    if (p) {
        *p = 0;
    }
    p = strchr(buf, '\n');
    if (p) {
        *p = 0;
    }

    zstream_printf_1024(fp, "your input: %s\n", buf);
    ret = ZSTREAM_FLUSH(fp);
    if (ret < 0) {
        server_error(ev);
        return -1;
    }

    zev_read(ev, server_read);

    return 0;
}

static int server_welcome(zev_t * ev)
{
    int events, ret;
    zstream_t *fp;

    events = zev_get_events(ev);
    if (events & ZEV_EXCEPTION) {
        server_error(ev);
        return 1;
    }

    fp = zstream_open_FD(zev_get_fd(ev));
    time_t t = time(0);
    zstream_printf_1024(fp, "welcome ev: %s\n", ctime(&t));
    ret = ZSTREAM_FLUSH(fp);
    if (ret < 0) {
        server_error(ev);
        return 1;
    }

    zev_set_context(ev, fp);
    zev_read(ev, server_read);

    return 0;
}

static int before_accept(zev_t * ev)
{
    int sock = zev_get_fd(ev);
    int fd = zinet_accept(sock);
    zev_t *nev = zev_create();
    zev_init(nev, zvar_evbase, fd);
    zev_write(nev, server_welcome);
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

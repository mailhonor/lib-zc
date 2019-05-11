/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2016-03-15
 * ================================
 */

#include "zc.h"
#include <time.h>

static zevent_base_t *evbase;
static int server_error(zeio_t * ev)
{
    int ret, fd;
    zstream_t *fp;

    ret = zeio_get_result(ev);
    fd = zeio_get_fd(ev);
    fp = (zstream_t *)zeio_get_context(ev);

    if (ret < 0) {
        zinfo("%d: error", fd);
    }

    zeio_free(ev, 1);
    if (fp) {
        zstream_close(fp, 1);
    }
    close(fd);

    return 0;
}

static void server_read(zeio_t * ev)
{
    int ret;

    if (zeio_get_result(ev) < 1) {
        server_error(ev);
        return;
    }

    ZSTACK_BUF(bf, 1025);
    zstream_t *fp= zstream_open_fd(zeio_get_fd(ev));
    ret = zstream_gets(fp, bf, 1024);
    if (ret < 1) {
        zstream_close(fp, 0);
        server_error(ev);
        return;
    }
    if (!strncmp(zbuf_data(bf), "exit", 4)) {
        zstream_close(fp, 0);
        server_error(ev);
        return;
    }
    zbuf_trim_right_rn(bf);

    zstream_printf_1024(fp, "your input: %s\n", zbuf_data(bf));
    ret = zstream_flush(fp);
    if (ret < 0) {
        zstream_close(fp, 0);
        server_error(ev);
        return;
    }

    zeio_enable_read(ev, server_read);
}

static void server_welcome(zeio_t * ev)
{
    int ret;
    zstream_t *fp;

    if (zeio_get_result(ev) < 0) {
        server_error(ev);
        return;
    }

    fp = zstream_open_fd(zeio_get_fd(ev));
    time_t t = time(0);
    zstream_printf_1024(fp, "welcome ev: %s\n", ctime(&t));
    ret = zstream_flush(fp);
    zstream_close(fp, 0);
    if (ret < 0) {
        server_error(ev);
        return;
    }

    zeio_enable_read(ev, server_read);
}

static void before_accept(zeio_t * ev)
{
    int sock = zeio_get_fd(ev);
    int fd = zinet_accept(sock);
    if (fd < 0) {
        return;
    }
    zeio_t *nev = zeio_create(fd, evbase);
    zeio_enable_write(nev, server_welcome);
}

static void timer_cb(zetimer_t * zt)
{
    zinfo("now exit!");
    exit(1);
}

int main(int argc, char **argv)
{
    int sock;
    zeio_t *ev;
    zetimer_t *tm;
    char *server;

    zmain_argument_run(argc, argv, 0);
    server = zconfig_get_str(zvar_default_config, "server", "");
    if (zempty(server)) {
        printf("USAGE: %s -O server 0:8899\n", zvar_progname);
        exit(1);
    }

    sock = zlisten(server, 0, 1, 5);
    if (sock < 0) {
        printf("ERR listen on %s(%m)\n", server);
        return 1;
    }

    printf("### echo server\n");
    evbase = zevent_base_create();
    ev = zeio_create(sock, evbase);
    zeio_enable_read(ev, before_accept);

    tm = zetimer_create(evbase);
    zetimer_start(tm, timer_cb, 200);

    while(zevent_base_dispatch(evbase)) {
    }

    return 0;
}

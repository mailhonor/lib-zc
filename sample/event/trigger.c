/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2016-03-15
 * ================================
 */

#include "zc.h"

static int current_client = 0;
static int all_client = 0;
static zaio_t *tm;
static zaio_t *listen_aio;

static void connect_quit(zaio_t *aio, const char *msg)
{
    if (msg) {
        zinfo("%s", msg);
    }
    zaio_free(aio, 1);
    current_client--;
}

static void timer_cb(zaio_t *tm)
{
    if (zvar_sigint_flag == 1) {
        /* 这段代码是为了检查内存泄露 */
        fprintf(stderr, "\r                          \n");
        zinfo("signal SIGINT, then EXIT");
        zaio_base_stop_notify(zaio_get_aio_base(tm));
        zaio_free(tm, 1);
        zaio_free(listen_aio, 1);
        return;
    }
    const char title[] = "LIB-ZC";
    static int s = 0;
    zinfo("%c all:%d, current:%d", title[s++%(sizeof(title)-1)], all_client, current_client);
    zaio_sleep(tm, timer_cb, 1);
}

static void server_read(zaio_t *aio)
{
    if (zaio_get_result(aio) < 1) {
        connect_quit(aio, "readable error");
        return;
    }

    ZSTACK_BUF(bf, 10240 + 1);
    zstream_t *fp= zstream_open_fd(zaio_get_fd(aio));
    int ret = zstream_gets(fp, bf, 10240);
    if (ret < 1) {
        zstream_close(fp, 0);
        connect_quit(aio, "read error");
        return;
    }
    zbuf_trim_right_rn(bf);

    zstream_write(fp, zbuf_data(bf), zbuf_len(bf));
    zstream_puts(fp, "\n");
    ret = zstream_flush(fp);
    zstream_close(fp, 0);
    if (ret < 0) {
        connect_quit(aio, "write error");
        return;
    }

    if (!strcmp(zbuf_data(bf), "exit")) {
        connect_quit(aio, 0);
        return;
    }

    zaio_readable(aio, server_read);
}

static void server_welcome(zaio_t *aio)
{
    if (zaio_get_result(aio) < 0) {
        connect_quit(aio, "writeable error");
        return;
    }

    zstream_t *fp = zstream_open_fd(zaio_get_fd(aio));
    zstream_puts(fp, "echo server, support command: exit\n");
    int ret = zstream_flush(fp);
    zstream_close(fp, 0);
    if (ret < 0) {
        connect_quit(aio, "write error");
        return;
    }

    zaio_readable(aio, server_read);
}

static void before_accept(zaio_t *aio)
{
    int sock = zaio_get_fd(aio);
    int fd = zinet_accept(sock);
    if (fd < 0) {
        return;
    }
    all_client++;
    current_client++;
    zaio_t *naio = zaio_create(fd, zaio_get_aio_base(aio));
    zaio_writeable(naio, server_welcome);
}

int main(int argc, char **argv)
{
    zmain_argument_run(argc, argv);
    
    char *listen = zconfig_get_str(zvar_default_config, "listen", 0);
    if (zempty(listen)) {
        zinfo("default listen on 0:8899");
        zinfo("USAGE %s -listen 0:8899", argv[0]);
        listen = "0:8899";
    }

    int sock = zlisten(listen, 0, 5);
    if (sock < 0) {
        zinfo("ERROR listen on %s(%m)", listen);
        return 1;
    }
    znonblocking(sock, 1);

    zinfo("### echo server start");

    zaio_base_t *aiobase = zaio_base_create();

    listen_aio = zaio_create(sock, aiobase);
    zaio_readable(listen_aio, before_accept);

    tm = zaio_create(-1, aiobase);
    zaio_sleep(tm, timer_cb, 1);

    zaio_base_run(aiobase);

    zaio_base_free(aiobase);

    return 0;
}

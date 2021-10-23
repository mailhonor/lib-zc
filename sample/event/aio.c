/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2015-11-24
 * ================================
 */

#include "zc.h"

static int current_client = 0;
static int all_client = 0;
static zaio_t *tm;
static zaio_t *listen_aio;

static int read_wait_timeout = -1;
static int write_wait_timeout = -1;

#define connect_quit(aio, msg)  { \
    if (msg) { \
        zinfo("%s", msg?msg:""); \
    } \
    zaio_free(aio, 1); \
    current_client--; \
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

static void after_read(zaio_t *aio);

static void after_write(zaio_t *aio)
{
    if (zaio_get_result(aio) < 1) {
        connect_quit(aio, "write error");
        return;
    }
    zaio_gets(aio, 10240, after_read);
}

static void after_write_and_exit(zaio_t *aio)
{
    if (zaio_get_result(aio) < 1) {
        connect_quit(aio, "write error");
    } else {
        connect_quit(aio, 0);
    }
}

static void after_read(zaio_t *aio)
{
    ZSTACK_BUF(bf, 10240);
    int ret = zaio_get_result(aio);
    if (ret < 1) {
        connect_quit(aio, "read error");
        return;
    }
    zaio_get_read_cache(aio, bf, ret);

    zbuf_trim_right_rn(bf);

    zaio_cache_write(aio, zbuf_data(bf), zbuf_len(bf));
    zaio_cache_write(aio, "\n", 1);
    if (!strcmp(zbuf_data(bf), "exit")) {
        zaio_cache_flush(aio, after_write_and_exit);
        return;
    }
    zaio_cache_flush(aio, after_write);
}

static void before_accept(zaio_t *aio)
{
    int sock = zaio_get_fd(aio);
    int fd = zinet_accept(sock);
    if (fd < -1) {
        return;
    }

    current_client++;
    all_client++;

    znonblocking(fd, 1);
    zaio_t *naio = zaio_create(fd, zaio_get_aio_base(aio));
    zaio_set_read_wait_timeout(naio, read_wait_timeout);
    zaio_set_write_wait_timeout(naio, write_wait_timeout);

    zaio_cache_puts(naio, "echo server, support command: exit\n");
    zaio_cache_flush(naio, after_write);
}

int main(int argc, char **argv)
{
    zmain_argument_run(argc, argv, 0);
    
    read_wait_timeout = zconfig_get_second(zvar_default_config, "read_wait_timeout", 3600*24);
    write_wait_timeout = zconfig_get_second(zvar_default_config, "write_wait_timeout", 3600*24);

    char *listen = zconfig_get_str(zvar_default_config, "listen", 0);
    if (zempty(listen)) {
        zinfo("default listen on 0:8899");
        zinfo("USAGE %s -listen 0:8899 [ -read_wait_timeout 1d ] [ -write_wait_timeout 1d ]    #", argv[0]);
        listen = "0:8899";
    }

    int sock = zlisten(listen, 0, 5);
    if (sock < 0) {
        zinfo("ERR listen on %s(%m)", listen);
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

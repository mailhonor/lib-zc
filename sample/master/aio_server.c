/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2015-11-24
 * ================================
 */

#include "zc.h"

#define connect_quit(aio, msg)  { \
    if (msg) { \
        zinfo("%s", msg?msg:""); \
    } \
    zaio_free(aio, 1); \
    current_client--; \
}

static int current_client = 0;
static int all_client = 0;

static void timer_cb(zaio_t *tm)
{
    if (zvar_sigint_flag == 1) {
        /* 这段代码是为了检查内存泄露 */
        fprintf(stderr, "\r                          \n");
        zinfo("signal SIGINT, then EXIT");
        zaio_base_stop_notify(zvar_default_aio_base);
        zaio_free(tm, 1);
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
        return;
    }
    connect_quit(aio, 0);
}

static void after_write_and_DETACH(zaio_t *aio)
{
    if (zaio_get_result(aio) < 1) {
        connect_quit(aio, "write error");
        return;
    }
    connect_quit(aio, 0);
    zaio_server_detach_from_master();
}

static void after_read(zaio_t *aio)
{
    ZSTACK_BUF(bf, 10240+1);
    char *buf = zbuf_data(bf);

    int ret = zaio_get_result(aio);
    if (ret < 1) {
        connect_quit(aio, "read error");
        return;
    }
    zaio_get_read_cache(aio, bf, ret);

    zbuf_trim_right_rn(bf);
    zaio_cache_write(aio, "your input:   ", 12);
    zaio_cache_write(aio, buf, zbuf_len(bf));
    zaio_cache_puts(aio, "\n");
    if (!strcmp(buf, "exit")) {
        zaio_cache_flush(aio, after_write_and_exit);
    } else if (!strcmp(buf, "DETACH")) {
        zaio_cache_flush(aio, after_write_and_DETACH);
    } else {
        zaio_cache_flush(aio, after_write);
    }
}

static void simple_service(int fd)
{
    current_client++;
    all_client++;
    znonblocking(fd, 1);
    zaio_t *aio = zaio_create(fd, zvar_default_aio_base);
    zaio_cache_puts(aio, "echo server, support command: exit, DETACH\n");
    zaio_cache_flush(aio, after_write);
}

static void service_register (const char *service, int fd, int fd_type)
{
    zaio_server_general_aio_register(zvar_default_aio_base, fd, fd_type, simple_service);
}

static void before_service()
{
    zaio_sleep(zaio_create(-1, zvar_default_aio_base), timer_cb, 1);
}

int main(int argc, char **argv)
{
    zaio_server_before_service = before_service;
    zaio_server_service_register = service_register;
    zaio_server_main(argc, argv);
    return 0;
}

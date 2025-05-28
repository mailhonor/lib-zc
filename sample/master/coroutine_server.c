/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-11-24
 * ================================
 */

#include "zc.h"
#include "zc_coroutine.h"
#include <errno.h>

void *do_log_test(void *arg)
{
    for (int i = 0; i <1000; i++) {
        zsleep(1);
        zinfo("coroutine-server do_log_test %d", i);
    }
    return 0;
}

void * do_echo(void *arg)
{
    ztype_convert_t ct;
    ct.VOID_PTR = arg;
    int fd = ct.INT;
    zstream_t *fp = zstream_open_fd(fd);

    zstream_puts(fp, "welcome coroutine\n");
    zstream_flush(fp);

    zbuf_t *line = zbuf_create(1024);
    while(!zstream_is_exception(fp)) {
        zbuf_reset(line);
        zstream_gets(fp, line, 10240);
        if (zbuf_len(line)) {
            zstream_write(fp, zbuf_data(line), zbuf_len(line));
            if (!strncmp(zbuf_data(line), "exit", 4)) {
                break;
            }
            if (!strncmp(zbuf_data(line), "EXIT", 4)) {
                zcoroutine_base_stop_notify(0);
                break;
            }
            if (!strncmp(zbuf_data(line), "DETACH", 6)) {
                zcoroutine_server_detach_from_master();
                continue;
            }
        }
    }
    zstream_flush(fp);

    zstream_close(fp, 1);
    zbuf_free(line);

    return 0;
}

static void * do_accept(void *arg)
{
    ztype_convert_t ct;
    ct.VOID_PTR = arg;
    int sock = ct.INT;
    while(1) {
        int fd = zinet_accept(sock);
        if (fd < 0) {
            if (errno == EAGAIN) {
                if (zvar_sigint_flag) {
                    break;
                }
                continue;
            }
            if (errno == EINTR) {
                continue;
            }
            zfatal("accept(%m)");
        }
        ct.INT = fd;
        zcoroutine_go(do_echo, ct.VOID_PTR, -1);
    }
    return 0;
}

static void service_register(const char *service_name, int fd, int fd_type)
{
    ztype_convert_t ct;
    ct.INT = fd;
    zcoroutine_go(do_accept, ct.VOID_PTR, -1);
}

int main(int argc, char **argv)
{
    zcoroutine_server_service_register = service_register;
    zcoroutine_server_main(argc, argv);
    return 0;
}


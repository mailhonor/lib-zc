/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2017-06-26
 * ================================
 */

#include "zc.h"

static char *server_address=0;
static void ___usage()
{
    printf("USAGE: %s -O server address\n", zvar_progname);
    exit(1);
}

static void *echo_service(void *context)
{
    int ret;
    int fd = (int)(long)context;
    zstream_t *fp = zstream_open_fd(fd);
    zbuf_t *bf = zbuf_create(0);
    while(1) {
        zbuf_reset(bf);
        ret = zstream_gets(fp, bf, 1024);
        if (ret < 0) {
            printf("socket error\n");
            break;
        }
        if (ret == 0) {
            printf("socket closed\n");
            break;
        }
        zstream_write(fp, zbuf_data(bf), zbuf_len(bf));
        zstream_flush(fp);
    }
    zstream_close(fp, 1);
    zbuf_free(bf);
    return context;
}
void *do_listen(void *context)
{
    int sock_type;
    int sock = zlisten(server_address, &sock_type, 5, 1); 
    while(1) {
        if (ztimed_read_wait(sock, 10) < 0) {
            printf("system error");
            exit(1);
        }
        int fd = zaccept(sock, sock_type);
        if (fd < 0) {
            continue;
        }
        printf("accept ok: %d\n", fd);
        void *arg = (void *)((long)fd);
        zcoroutine_go(echo_service, arg, 0);
    }
    return 0;
}

int main(int argc, char **argv)
{
    zmain_argument_run(argc, argv, 0);
    server_address = zconfig_get_str(zvar_default_config, "server", 0);
    if (zempty(server_address)) {
        ___usage();
    }
    zcoroutine_base_init();
    zcoroutine_go(do_listen, 0, 0);
    zcoroutine_base_run();
    zcoroutine_base_fini();
    return 0;
}

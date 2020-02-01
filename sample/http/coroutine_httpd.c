/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2017-04-05
 * ================================
 */

/*
ab -n 1000 -c 1000 http://127.0.0.1:8080/
*/

#ifndef ___INNER_DONOT_USE_SSL___
#define ___INNER_USE_SSL___
#endif

#include "./lib_httpd.h"

static void *accept_incoming(void *arg)
{
    long_info_t linfo;
    linfo.ptr = arg;
    int sock = linfo.sockinfo.fd;
    int sock_type =linfo.sockinfo.sock_type;

    while (1) {
        ztimed_read_wait(sock, 10);
        int fd = zaccept(sock, sock_type);
        if (fd < 0) {
            if (errno == EAGAIN) {
                continue;
            }
            if (errno == EINTR) {
                continue;
            }
            zfatal("accept: %m");
        }
        linfo.sockinfo.fd = fd;
        zcoroutine_go(do_httpd, linfo.ptr, -1);
    }
    return arg;
}

int main(int argc, char **argv)
{
    signal(SIGPIPE, SIG_IGN);
    zmain_argument_run(argc, argv, 0);

    char *listen = zconfig_get_str(zvar_default_config, "listen", "");
    if (zempty(listen)) {
        usage();
    }
    load_ssl();

    explore_data_init();

    zcoroutine_base_init();

    long_info_t linfo;
    int fd, type;
    fd = zlisten(listen, &type, 5);
    if (fd < 0) {
        printf("ERR can not open %s(%m)\n", listen);
        exit(1);
    }
    linfo.sockinfo.fd = fd;
    linfo.sockinfo.sock_type = type;

    zcoroutine_go(accept_incoming, linfo.ptr, -1);

    zcoroutine_base_run(0);

    zclose(fd);

    zcoroutine_base_fini();
    explore_data_fini();

    zinfo("EXIT");

    return 0;
}

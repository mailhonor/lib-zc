/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn
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

int main(int argc, char **argv)
{
    zmain_argument_run(argc, argv);
    char *listen = zconfig_get_str(zvar_default_config, "listen", "");
    if (zempty(listen)) {
        usage();
    }
    load_ssl();

    explore_data_init();

    int sock, type;
    sock = zlisten(listen, &type, 5);
    if (sock < 0) {
        zprintf("ERROR can not open %s(%m)\n", listen);
        exit(1);
    }

    while (1) {
        int fd = zaccept(sock, type);
        if (fd < 0)
        {
            if (errno == EAGAIN) {
                continue;
            }
            if (errno == EINTR) {
                continue;
            }
            zfatal("accept: %m");
        }
        long_info_t linfo;
        linfo.sockinfo.fd = fd;
        linfo.sockinfo.sock_type = type;
        do_httpd(linfo.ptr);
    }

    zclosesocket(sock);
    explore_data_fini();

    return 0;
}

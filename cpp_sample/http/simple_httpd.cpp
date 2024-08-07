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

#include "./lib_httpd.h"

int main(int argc, char **argv)
{
    zcc::main_argument::run(argc, argv);
    init();

    int sock, type;
    sock = zcc::netpath_listen(listen_address, 5, &type);
    if (sock < 0)
    {
        zcc_error_and_exit("open %s(%m)", listen_address);
    }

    while (1)
    {
        int fd = zcc::socket_accept(sock, type);
        if (fd < 0)
        {
            int err = zcc::get_errno();
            if (err == ZCC_EAGAIN)
            {
                continue;
            }
            if (err == ZCC_EINTR)
            {
                continue;
            }
            zcc_fatal("accept: %m");
        }
        zcc::iostream *fp = new zcc::iostream(fd);
        do_httpd_serve(*fp);
        if (zcc::var_memleak_check_enable && zcc::var_sigint_flag)
        {
            break;
        }
    }

    zcc::close_socket(sock);
    fini();

    return 0;
}

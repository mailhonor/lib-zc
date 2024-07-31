/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2017-04-05
 * ================================
 */

/*
ab -n 1000 -c 1000 http://127.0.0.1:8080/
*/

#ifdef __linux__
#define TEST_USE_COROUTINE
#include "./lib_httpd.h"

struct void_long_t
{
    int fd;
    int type;
};

static void *do_httpd_coroutine(void *arg)
{
    do_httpd_serve(*(zcc::stream *)arg);
    return arg;
}

static void *accept_incoming(void *arg)
{
    void_long_t *vl = (void_long_t *)arg;
    int sock = vl->fd;
    int type = vl->type;
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
        if (zcc::var_memleak_check_enable && zcc::var_sigint_flag)
        {
            break;
        }
        zcc::iostream *fp = new zcc::iostream(fd);
        zcoroutine_go(do_httpd_coroutine, fp, -1);
    }
    return arg;
}

int main(int argc, char **argv)
{
    zcc::signal_ignore(SIGPIPE);
    zcc::main_argument::run(argc, argv);
    init();
    zcoroutine_base_init();

    int sock, type;
    sock = zcc::netpath_listen(listen, 5, &type);
    if (sock < 0)
    {
        zcc_error_and_exit("open %s(%m)", listen);
    }
    void_long_t vl;
    vl.fd = sock;
    vl.type = type;

    zcoroutine_go(accept_incoming, &vl, -1);

    zcoroutine_base_run();
    zcc::close_socket(sock);
    zcoroutine_base_fini();
    fini();
    return 0;
}
#else  // __linux__
int main()
{
    return 0;
}
#endif // __linux__
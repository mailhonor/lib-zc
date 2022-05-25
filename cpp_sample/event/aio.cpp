/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-11-24
 * ================================
 */

#include "zc.h"

static int current_client = 0;
static int all_client = 0;
static zcc::aio *tm;
static zcc::aio *listen_aio;

static int read_wait_timeout = -1;
static int write_wait_timeout = -1;

static void after_read(zcc::aio *aio);

static void connect_quit(zcc::aio *aio, const char *msg)
{
    if (msg) {
        zinfo("%s", msg);
    }
    delete aio;
    current_client--;
}

static void timer_cb(zcc::aio *tm)
{
    if (zvar_sigint_flag == 1) {
        /* 这段代码是为了检查内存泄露 */
        fprintf(stderr, "\r                          \n");
        zinfo("signal SIGINT, then EXIT");
        tm->get_aio_base()->stop_notify();
        delete tm;
        delete listen_aio;
        return;
    }
    const char title[] = "LIB-ZC";
    static int s = 0;
    zinfo("%c all:%d, current:%d", title[s++%(sizeof(title)-1)], all_client, current_client);
    tm->sleep(std::bind(timer_cb, tm), 1);
}

static void after_write(zcc::aio *aio)
{
    if (aio->get_result() < 1) {
        connect_quit(aio, "write error");
        return;
    }
    aio->gets(10240, std::bind(after_read, aio));
}

static void after_write_and_exit(zcc::aio *aio)
{
    if (aio->get_result() < 1) {
        connect_quit(aio, "write error");
    } else {
        connect_quit(aio, 0);
    }
}

static void after_read(zcc::aio *a)
{
    std::string bf;
    int ret = a->get_result();
    if (ret < 1) {
        connect_quit(a, "read error");
        return;
    }
    a->get_read_cache(bf, ret);

    zcc::trim_right(bf, "\r\n");

    a->cache_write(bf);
    a->cache_write("\n", 1);
    if (bf == "exit") {
        a->cache_flush(std::bind(after_write_and_exit, a));
        return;
    }
    a->cache_flush(std::bind(after_write, a));
}

static void before_accept(zcc::aio *aio)
{
    int sock = aio->get_fd();
    int fd = zinet_accept(sock);
    if (fd < -1) {
        return;
    }

    current_client++;
    all_client++;

    znonblocking(fd, 1);
    zcc::aio *naio = new zcc::aio(fd, aio->get_aio_base());
    naio->set_read_wait_timeout(read_wait_timeout);
    naio->set_write_wait_timeout(write_wait_timeout);

    naio->cache_puts("echo server, support command: exit\n");
    naio->cache_flush(std::bind(after_write, naio));
}

int main(int argc, char **argv)
{
    zmain_argument_run(argc, argv);
    zinfo("USAGE %s -listen 0:8899 [ -read_wait_timeout 1d ] [ -write_wait_timeout 1d ]", zvar_progname);
    
    read_wait_timeout = zconfig_get_second(zvar_default_config, "read_wait_timeout", 3600*24);
    write_wait_timeout = zconfig_get_second(zvar_default_config, "write_wait_timeout", 3600*24);

    const char *listen = zconfig_get_str(zvar_default_config, "listen", "0:8899");

    int sock = zlisten(listen, 0, 5);
    if (sock < 0) {
        zinfo("ERR listen on %s(%m)", listen);
        return 1;
    }
    znonblocking(sock, 1);

    zinfo("### echo server start");

    zcc::aio_base ab;

    listen_aio = new zcc::aio(sock, &ab);
    listen_aio->readable(std::bind(before_accept, listen_aio));

    tm = new zcc::aio(-1, &ab);
    tm->sleep(std::bind(timer_cb, tm), 1);

    ab.run();

    return 0;
}

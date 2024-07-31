/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-11-24
 * ================================
 */

#include "zcc/zcc_aio.h"

static int current_client = 0;
static int all_client = 0;

static int wait_timeout = -1;
static const char *server;

static void after_read(zcc::aio *aio);

static void connection_quit(zcc::aio *aio, const char *msg)
{
    if (msg)
    {
        zcc_info("%s", msg);
    }
    delete aio;
    current_client--;
}

static int timer_count = 0;
static void timer_cb(zcc::aio_timer *tm)
{
    const char title[] = "LIB-ZC";
    static int s = 0;
    zcc_info("%c all:%d, current:%d", title[s++ % (sizeof(title) - 1)], all_client, current_client);
    tm->after(std::bind(timer_cb, tm), 1);
}

static void after_write(zcc::aio *aio)
{
    if (aio->get_result() < 1)
    {
        connection_quit(aio, "write error");
        return;
    }
    aio->gets(10240, std::bind(after_read, aio));
}

static void after_write_and_exit(zcc::aio *aio)
{
    if (aio->get_result() < 1)
    {
        connection_quit(aio, "write error");
    }
    else
    {
        connection_quit(aio, 0);
    }
}

static void after_write_and_EXIT(zcc::aio *aio)
{
    auto eb = aio->get_aio_base();
    connection_quit(aio, 0);
    eb->stop_notify();
}

static void after_read(zcc::aio *a)
{
    std::string bf;
    int ret = a->get_result();
    if (ret < 1)
    {
        connection_quit(a, "read error");
        return;
    }
    a->get_read_cache(bf, ret);

    zcc::trim_right(bf, "\r\n");

    a->cache_write(bf);
    a->cache_write("\n", 1);
    if (bf == "exit")
    {
        a->cache_flush(std::bind(after_write_and_exit, a));
        return;
    }
    if (bf == "EXIT")
    {
        a->cache_flush(std::bind(after_write_and_EXIT, a));
        return;
    }
    a->cache_flush(std::bind(after_write, a));
}

static void before_accept(zcc::aio *aio)
{
    int sock = aio->get_fd();
    int fd = zcc::inet_accept(sock);
    if (fd < -1)
    {
        return;
    }

    current_client++;
    all_client++;

    zcc::nonblocking(fd);
    zcc::aio *naio = new zcc::aio(fd, aio->get_aio_base());
    naio->set_timeout(wait_timeout);
    naio->set_timeout(wait_timeout);

    naio->cache_puts("echo server, support command: exit\n");
    naio->cache_flush(std::bind(after_write, naio));
}

int main(int argc, char **argv)
{
    zcc::main_argument::run(argc, argv);
    zcc_info("USAGE %s -listen 0:8899 [ -wait_timeout 1d ] [ -wait_timeout 1d ]", zcc::progname);

    wait_timeout = zcc::var_main_config.get_second("wait_timeout", 3600 * 24);
    wait_timeout = zcc::var_main_config.get_second("wait_timeout", 3600 * 24);

    server = zcc::var_main_config.get_cstring("server", "0:8899");

    int sock = zcc::netpath_listen(server, 5);
    if (sock < 0)
    {
        zcc_info("ERROR listen on %s(%m)", server);
        return 1;
    }
    zcc::nonblocking(sock);

    zcc_info("### echo server start");

    zcc::aio_base ab;

    zcc::aio *listen_aio = new zcc::aio(sock, &ab);
    listen_aio->readable(std::bind(before_accept, listen_aio));

    zcc::aio_timer *tm = new zcc::aio_timer(&ab);
    tm->after(std::bind(timer_cb, tm), 1);

    ab.run();

    delete listen_aio;
    delete tm;

    return 0;
}
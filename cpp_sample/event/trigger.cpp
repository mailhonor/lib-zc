/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2016-03-15
 * ================================
 */

#include "zcc/zcc_aio.h"
#include "zcc/zcc_stream.h"

static int current_client = 0;
static int all_client = 0;
static int flag_stop = 0;

static void connection_quit(zcc::aio *aio, const char *msg)
{
    if (msg)
    {
        zcc_info("%s", msg);
    }
    delete aio;
    current_client--;
}

static void timer_cb(zcc::aio_timer *tm)
{
    if (zcc::var_sigint_flag)
    {
        zcc_info("\nsigint, wait ...");
        flag_stop = 1;
    }
    if (flag_stop)
    {
        tm->get_aio_base()->stop_notify();
        delete tm;
        return;
    }
    const char title[] = "LIB-ZC";
    static int s = 0;
    zcc_info("%c all:%d, current:%d", title[s++ % (sizeof(title) - 1)], all_client, current_client);
    tm->after(std::bind(timer_cb, tm), 1);
}

static void server_read(zcc::aio *aio)
{
    if (aio->get_result() < 1)
    {
        connection_quit(aio, "readable error");
        return;
    }

    zcc::iostream *fp = new zcc::iostream();
    fp->open_socket(aio->get_fd());
    std::string str;
    if (fp->gets(str, 10240) < 0)
    {
        fp->close(false);
        delete fp;
        connection_quit(aio, "read error");
        return;
    }
    fp->append(str);
    if (fp->flush() < 1)
    {
        fp->close(false);
        delete fp;
        connection_quit(aio, "write error");
        return;
    }
    fp->close(false);
    delete fp;

    zcc::trim_line_end_rn(str);

    if (str == "exit")
    {
        connection_quit(aio, 0);
        return;
    }
    if (str == "EXIT")
    {
        flag_stop = 1;
        connection_quit(aio, 0);
        return;
    }

    aio->readable(std::bind(server_read, aio));
}

static void server_welcome(zcc::aio *aio)
{
    if (aio->get_result() < 0)
    {
        connection_quit(aio, "writeable error");
        return;
    }

    zcc::iostream fp;
    fp.open_socket(aio->get_fd());

    fp.append("echo server, support command: exit\n");
    if (fp.flush() < 0)
    {
        fp.close(false);
        connection_quit(aio, "write error");
        return;
    }
    fp.close(false);

    aio->readable(std::bind(server_read, aio));
}

static void before_accept(zcc::aio *aio)
{
    int sock = aio->get_fd();
    int fd = zcc::inet_accept(sock);
    if (fd < 0)
    {
        return;
    }
    all_client++;
    current_client++;
    zcc::aio *naio = new zcc::aio(fd, aio->get_aio_base());
    naio->writeable(std::bind(server_welcome, naio));
}

int main(int argc, char **argv)
{
    zcc::main_argument::run(argc, argv);
    zcc_info("USAGE %s -listen 0:8899", zcc::progname);

    const char *server = zcc::var_main_config.get_cstring("server", "0:8899");

    int sock = zcc::netpath_listen(server, 5);
    if (sock < 0)
    {
        zcc_info("ERROR listen on %s(%m)", server);
        return 1;
    }
    zcc::nonblocking(sock);

    zcc_info("### echo server start");

    zcc::aio_base ab;

    auto listen_aio = new zcc::aio(sock, &ab);
    listen_aio->readable(std::bind(before_accept, listen_aio));

    auto tm = new zcc::aio_timer(&ab);
    tm->after(std::bind(timer_cb, tm), 1);

    ab.run();
    delete listen_aio;

    return 0;
}

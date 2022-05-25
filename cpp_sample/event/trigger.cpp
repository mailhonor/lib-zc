/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2016-03-15
 * ================================
 */

#include "zc.h"

static int current_client = 0;
static int all_client = 0;
static zcc::aio *tm;
static zcc::aio *listen_aio;

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

static void server_read(zcc::aio *aio)
{
    if (aio->get_result()< 1) {
        connect_quit(aio, "readable error");
        return;
    }

    ZSTACK_BUF(bf, 10240 + 1);
    zstream_t *fp= zstream_open_fd(aio->get_fd());
    int ret = zstream_gets(fp, bf, 10240);
    if (ret < 1) {
        zstream_close(fp, 0);
        connect_quit(aio, "read error");
        return;
    }
    zbuf_trim_right_rn(bf);

    zstream_write(fp, zbuf_data(bf), zbuf_len(bf));
    zstream_puts(fp, "\n");
    ret = zstream_flush(fp);
    zstream_close(fp, 0);
    if (ret < 0) {
        connect_quit(aio, "write error");
        return;
    }

    if (!strcmp(zbuf_data(bf), "exit")) {
        connect_quit(aio, 0);
        return;
    }

    aio->readable(std::bind(server_read, aio));
}

static void server_welcome(zcc::aio *aio)
{
    if (aio->get_result() < 0) {
        connect_quit(aio, "writeable error");
        return;
    }

    zstream_t *fp = zstream_open_fd(aio->get_fd());
    zstream_puts(fp, "echo server, support command: exit\n");
    int ret = zstream_flush(fp);
    zstream_close(fp, 0);
    if (ret < 0) {
        connect_quit(aio, "write error");
        return;
    }

    aio->readable(std::bind(server_read, aio));
}

static void before_accept(zcc::aio *aio)
{
    int sock = aio->get_fd();
    int fd = zinet_accept(sock);
    if (fd < 0) {
        return;
    }
    all_client++;
    current_client++;
    zcc::aio *naio = new zcc::aio(fd, aio->get_aio_base());
    naio->writeable(std::bind(server_welcome, naio));
}

int main(int argc, char **argv)
{
    zmain_argument_run(argc, argv);
    zinfo("USAGE %s -listen 0:8899", zvar_progname);
    
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

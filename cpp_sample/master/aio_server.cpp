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

class my_aio_server: public zcc::aio_server {
public:
    void service_register(const char *service, int fd, int fd_type);
    void before_service(void);
    void before_softstop(void);
};

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
        return;
    }
    const char title[] = "LIB-ZC";
    static int s = 0;
    zinfo("%c all:%d, current:%d", title[s++%(sizeof(title)-1)], all_client, current_client);
    tm->sleep(std::bind(timer_cb, tm), 1);
}

static void after_read(zcc::aio *aio);

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
        return;
    }
    connect_quit(aio, 0);
}

static void after_write_and_DETACH(zcc::aio *aio)
{
    if (aio->get_result() < 1) {
        connect_quit(aio, "write error");
        return;
    }
    connect_quit(aio, 0);
    zcc::var_default_aio_server->detach_from_master();
}

static void after_read(zcc::aio *aio)
{
    std::string bf;

    int ret = aio->get_result();
    if (ret < 1) {
        connect_quit(aio, "read error");
        return;
    }
    aio->get_read_cache(bf, ret);
    zcc::trim_right(bf, "\r\n");

    aio->cache_write("your input:   ", 12);
    aio->cache_write(bf);
    aio->cache_puts("\n");

    if (bf == "exit") {
        aio->cache_flush(std::bind(after_write_and_exit, aio));
    } else if (bf == "DETACH") {
        aio->cache_flush(std::bind(after_write_and_DETACH, aio));
    } else {
        aio->cache_flush(std::bind(after_write, aio));
    }
}

static void simple_service(int fd)
{
    current_client++;
    all_client++;
    znonblocking(fd, 1);
    zcc::aio *aio = new zcc::aio(fd, zcc::var_default_aio_base);
    aio->cache_puts("echo server, support command: exit, DETACH\n");
    aio->cache_flush(std::bind(after_write, aio));
}

void my_aio_server::service_register(const char *service, int fd, int fd_type)
{
    general_service_register(fd, fd_type, simple_service);
}

void my_aio_server::before_service(void)
{
    zcc::aio_server::before_service();
    zcc::aio *tm = new zcc::aio(-1, zcc::var_default_aio_base);
    tm->sleep(std::bind(timer_cb, tm), 1);
}

void my_aio_server::before_softstop(void)
{
    zcc::aio_server::before_softstop();
    fprintf(stderr, "before_softstop, nothing to do\n");
}

int main(int argc, char **argv)
{
    my_aio_server mas;
    mas.run(argc, argv);
    return 0;
}

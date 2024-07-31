/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-11-24
 * ================================
 */

#include "zcc/zcc_server.h"
#include "zcc/zcc_openssl.h"

static bool stop_flag = false;
static int current_client = 0;
static int all_client = 0;
static bool info_flag = false;

class my_aio_worker_server : public zcc::aio_worker_server
{
public:
    void service_register(const char *service, int fd, int fd_type);
    void before_service(void);
    void before_softstop(void);
};

class my_aio : public zcc::aio
{
public:
    inline my_aio(int fd) : aio(fd, zcc::var_main_aio_base) {}
    std::string service_name_;
};

static void connection_quit(my_aio *aio, const char *msg)
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
        /* 这段代码是为了检查内存泄露 */
        zcc_info("\nsignal SIGINT, then EXIT");
        stop_flag = true;
    }
    if (stop_flag)
    {
        tm->get_aio_base()->stop_notify();
        delete tm;
        return;
    }
    const char title[] = "LIB-ZC";
    static int s = 0;
    if (info_flag)
    {
        zcc_info("%c all:%d, current:%d", title[s++ % (sizeof(title) - 1)], all_client, current_client);
    }
    tm->after(std::bind(timer_cb, tm), 1);
}

static void after_read(my_aio *aio);

static void after_write(my_aio *aio)
{
    if (aio->get_result() < 1)
    {
        connection_quit(aio, "write error 1");
        return;
    }
    aio->gets(10240, std::bind(after_read, aio));
}

static void after_write_and_exit(my_aio *aio)
{
    if (aio->get_result() < 1)
    {
        connection_quit(aio, "write error 2");
        return;
    }
    connection_quit(aio, 0);
}

static void after_write_and_EXIT(my_aio *aio)
{
    stop_flag = true;
    if (aio->get_result() < 1)
    {
        connection_quit(aio, "write error 3");
        return;
    }
    connection_quit(aio, 0);
}

static void after_write_and_DETACH(my_aio *aio)
{
    if (aio->get_result() < 1)
    {
        connection_quit(aio, "write error 4");
        return;
    }
    zcc::aio_worker_server::get_instance()->detach_from_master();
    aio->gets(10240, std::bind(after_read, aio));
}

static void after_read(my_aio *aio)
{
    std::string bf;

    int ret = aio->get_result();
    if (ret < 1)
    {
        connection_quit(aio, "read error");
        return;
    }
    aio->get_read_cache(bf, ret);
    zcc::trim_right(bf, "\r\n");

    if (!aio->service_name_.empty())
    {
        aio->cache_append("(");
        aio->cache_append(aio->service_name_);
        aio->cache_append(") ");
    }
    aio->cache_write(bf);
    aio->cache_puts("\n");

    if (bf == "exit")
    {
        aio->cache_flush(std::bind(after_write_and_exit, aio));
    }
    else if (bf == "EXIT")
    {
        aio->cache_flush(std::bind(after_write_and_EXIT, aio));
    }
    else if (bf == "DETACH")
    {
        aio->cache_flush(std::bind(after_write_and_DETACH, aio));
    }
    else
    {
        aio->cache_flush(std::bind(after_write, aio));
    }
}

static void simple_service(int fd, std::string service_name)
{
    current_client++;
    all_client++;
    zcc::nonblocking(fd);
    my_aio *aio = new my_aio(fd);
    aio->service_name_ = service_name;
    aio->cache_printf_1024("echo server(%s), support command: exit, DETACH\n", service_name.c_str());
    aio->cache_flush(std::bind(after_write, aio));
}

void my_aio_worker_server::service_register(const char *service, int fd, int fd_type)
{
    std::string service_name = service;
    zcc::aio_worker_server::simple_service_register(zcc::var_main_aio_base, fd, fd_type, std::bind(simple_service, std::placeholders::_1, service_name));
}

void my_aio_worker_server::before_service(void)
{
    info_flag = zcc::var_main_config.get_bool("info");
    zcc::aio_worker_server::before_service();
    zcc::aio_timer *tm = new zcc::aio_timer(zcc::var_main_aio_base);
    tm->after(std::bind(timer_cb, tm), 1);
}

void my_aio_worker_server::before_softstop(void)
{
    zcc::aio_worker_server::before_softstop();
    zcc_info("before_softstop, nothing to do");
}

int main(int argc, char **argv)
{
    my_aio_worker_server ms;
    ms.main_run(argc, argv);
    return 0;
}

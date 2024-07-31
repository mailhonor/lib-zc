/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2016-04-22
 * ================================
 */

#include "zcc/zcc_aio.h"
#include "zcc/zcc_thread.h"
#include <mutex>

static int flag_stop = 0;
static int wait_timeout = -1;
static const char *server = "";

static std::mutex count_locker;
static int current_client = 0;
static int all_client = 0;

static zcc::thread_pool *thread_pool = nullptr;

static void timer_cb(zcc::aio_timer *tm);

static void connection_quit(zcc::aio *aio, const char *msg)
{
    if (msg)
    {
        zcc_info("%s", msg);
    }
    delete aio;
    count_locker.lock();
    current_client--;
    count_locker.unlock();
}

static void foo_timer_cb(zcc::aio_timer *tm)
{
    // 此时已经在线程池了
    const char title[] = "LIB-ZC";
    static int s = 0;
    zcc_info("%c all:%d, current:%d", title[s++ % (sizeof(title) - 1)], all_client, current_client);
    tm->after(std::bind(timer_cb, tm), 1);
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

    // 仅仅为了测试, 我们把此功能也移动到线程池
    // 否则 // zaio_sleep(tm, timer_cb, 1);

    // 因为此 aio_timer 离开了aio_base 环境, 必须执行 disable
    tm->disable();
    thread_pool->enter_task(std::bind(foo_timer_cb, tm));
}

static void after_read(zcc::aio *aio);
static void after_write(zcc::aio *aio)
{
    // 当前线程是主线程
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
    if (aio->get_result() < 1)
    {
        connection_quit(aio, "write error");
    }
    else
    {
        connection_quit(aio, 0);
    }
    flag_stop = 1;
}

static void do_job(zcc::aio *aio)
{
    // 这个时候已经在线程池了
    std::string bf;
    int ret = aio->get_result();
    if (ret < 1)
    {
        connection_quit(aio, "read error");
        return;
    }
    aio->get_read_cache(bf, ret);

    zcc::trim_right(bf, "\r\n");

    aio->cache_write(bf);
    aio->cache_write("\n", 1);
    if (bf == "exit")
    {
        aio->cache_flush(std::bind(after_write_and_exit, aio));
        return;
    }
    if (bf == "EXIT")
    {
        aio->cache_flush(std::bind(after_write_and_EXIT, aio));
        return;
    }
    aio->cache_flush(std::bind(after_write, aio));
}

static void after_read(zcc::aio *aio)
{
    // 我们假设:
    // 在读取一行数据后, 要做一些复杂的业务,可能耗费较长时间,会阻塞...
    // 所以, 我们把业务处理交给线程池来处理

    // 因为此aio准备离开 aio_base 环境, 必须执行 disable
    aio->disable();
    thread_pool->enter_task(std::bind(do_job, aio));
}

static void before_accept(zcc::aio *aio)
{
    int sock = aio->get_fd();
    int fd = zcc::inet_accept(sock);
    if (fd < -1)
    {
        return;
    }

    count_locker.lock();
    current_client++;
    all_client++;
    count_locker.unlock();

    zcc::nonblocking(fd);

    // 仅仅为了演示 aio 的一些功能, 否则直接就将 fd 直接传给线程池了
    zcc::aio *naio = new zcc::aio(fd, aio->get_aio_base());
    naio->set_timeout(wait_timeout);
    naio->set_timeout(wait_timeout);

    naio->cache_puts("echo server, support command: exit/EXIT\n");
    naio->cache_flush(std::bind(after_write, naio));
}

int main(int argc, char **argv)
{
    zcc::main_argument::run(argc, argv);
    zcc_info("USAGE %s -listen 0:8899 [ -wait_timeout 1d ] [ -wait_timeout 1d ]", zcc::progname);

    wait_timeout = zcc::var_main_config.get_second("wait_timeout", 3600 * 24);
    wait_timeout = zcc::var_main_config.get_second("wait_timeout", 3600 * 24);
    server = zcc::var_main_config.get_cstring("server", "0:8899");

    thread_pool = new zcc::thread_pool();
    thread_pool->create_thread(5);

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
    tm->sleep(std::bind(timer_cb, tm), 1);

    ab.run();
    thread_pool->softstop();
    thread_pool->wait_all_stopped(9);
    delete thread_pool;

    delete listen_aio;

    return 0;
}

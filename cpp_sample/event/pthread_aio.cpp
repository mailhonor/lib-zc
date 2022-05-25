/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2016-04-22
 * ================================
 */

#include "zc.h"
#include <pthread.h>
#include <list>

std::list<std::function<void()> > job_list;

static int flag_stop = 0;
static pthread_mutex_t locker = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

static int read_wait_timeout = -1;
static int write_wait_timeout = -1;

static void *pthread_worker(void *arg)
{
    pthread_detach(pthread_self());
    while (1) {
        if (flag_stop) {
            break;
        }
        std::function<void()> job;
        pthread_mutex_lock(&locker);
        while (job_list.empty()) {
            if (flag_stop) {
                break;
            }
            struct timespec ts;
            ts.tv_sec = time(0) + 1;
            ts.tv_nsec = 0;
            pthread_cond_timedwait(&cond, &locker, &ts);
        }
        if (!job_list.empty()) {
            job = job_list.front();
            job_list.pop_front();
        }
        pthread_mutex_unlock(&locker);
        job();
    }
    return 0;
}


static pthread_mutex_t count_locker = PTHREAD_MUTEX_INITIALIZER;
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


static void timer_cb(zcc::aio *tm);

static void foo_timer_cb(zcc::aio *aio)
{
    /* 此时已经在线程池了 */
    const char title[] = "LIB-ZC";
    static int s = 0;
    zinfo("%c all:%d, current:%d", title[s++%(sizeof(title)-1)], all_client, current_client);
    tm->sleep(std::bind(timer_cb, tm), 1);
}

static void timer_cb(zcc::aio *tm)
{
    if (zvar_sigint_flag == 1) {
        /* 这段代码是为了检查内存泄露 */
        fprintf(stderr, "\r                          \n");
        zinfo("signal SIGINT, then sleep 3, then EXIT");
        flag_stop = 1;
        tm->get_aio_base()->stop_notify();
        /* 等线程池退出 */
        zsleep(3);
        delete tm;
        delete listen_aio;
        return;
    }

#if 0
    char tbuf[128];
    zinfo("%s client count: %d", get_current_time(tbuf), current_client);
    zaio_sleep(tm, timer_cb, 1);
#else 
    /* 仅仅为了测试, 我们把此功能也移动到线程池 */
    /* 因为此aio离开了aio_base 环境, 必须执行 disable */
    tm->disable();

    pthread_mutex_lock(&locker);
    job_list.push_back(std::bind(foo_timer_cb, tm));
    pthread_mutex_unlock(&locker);
    pthread_cond_signal(&cond);
#endif
}

static void after_read(zcc::aio *aio);
static void after_write(zcc::aio *aio)
{
    /* 当前线程是主线程 */
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

static void foo(zcc::aio *aio)
{
    /* 这个时候已经在线程池了 */
    std::string bf;
    int ret = aio->get_result();
    if (ret < 1) {
        connect_quit(aio, "read error");
        return;
    }
    aio->get_read_cache(bf, ret);

    zcc::trim_right(bf, "\r\n");

    aio->cache_write(bf);
    aio->cache_write("\n", 1);
    if (bf == "exit") {
        aio->cache_flush(std::bind(after_write_and_exit, aio));
        return;
    }
    aio->cache_flush(std::bind(after_write, aio));
}

static void after_read(zcc::aio *aio)
{
    /* 我们假设:
     * 在读取一行数据后, 要做一些复杂的业务,可能耗费较长时间,会阻塞...
     * 所以, 我们把业务处理交给线程池来处理 */

    /* 因为此aio准备离开 aio_base 环境, 必须执行 disable */
    aio->disable();

    pthread_mutex_lock(&locker);
    job_list.push_back(std::bind(foo, aio));
    pthread_mutex_unlock(&locker);
    pthread_cond_signal(&cond);
}

static void before_accept(zcc::aio *aio)
{
    int sock = aio->get_fd();
    int fd = zinet_accept(sock);
    if (fd < -1) {
        return;
    }

    pthread_mutex_lock(&count_locker);
    current_client++;
    all_client++;
    pthread_mutex_unlock(&count_locker);

    znonblocking(fd, 1);
    /* 仅仅为了演示 aio 的一些功能, 否则直接就将 fd 直接传给线程池了 */
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

    for (int pi = 0; pi < 2; pi++) {
        pthread_t pth_id;
        pthread_create(&pth_id, 0, pthread_worker, 0);
    }

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


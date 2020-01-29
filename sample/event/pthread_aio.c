/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2016-04-22
 * ================================
 */

#include "zc.h"
#include <pthread.h>

typedef struct info_t info_t;
struct info_t {
    zaio_t *aio;
    void *other_data;
    void (*foo)(info_t *);
};

static int flag_stop = 0;
static zlist_t *info_list = 0;
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
        pthread_mutex_lock(&locker);
        while (!zlist_head(info_list)) {
            if (flag_stop) {
                break;
            }
            struct timespec ts;
            ts.tv_sec = time(0) + 1;
            ts.tv_nsec = 0;
            pthread_cond_timedwait(&cond, &locker, &ts);
        }
        info_t *info = 0;
        zlist_shift(info_list, (void **)&info);
        pthread_mutex_unlock(&locker);
        if (!info) {
            continue;
        }
        info->foo(info);
        zfree(info);
    }
    return 0;
}


static pthread_mutex_t count_locker = PTHREAD_MUTEX_INITIALIZER;
static int current_client = 0;
static int all_client = 0;
static zaio_t *tm;
static zaio_t *listen_aio;

#define connect_quit(aio, msg)  { \
    if (msg) { \
        zinfo("%s", msg?msg:""); \
    } \
    zaio_free(aio, 1); \
    current_client--; \
}


static void timer_cb(zaio_t *tm);

static void foo_timer_cb(info_t *info)
{
    /* 此时已经在线程池了 */
    const char title[] = "LIB-ZC";
    static int s = 0;
    zinfo("%c all:%d, current:%d", title[s++%(sizeof(title)-1)], all_client, current_client);
    zaio_sleep(tm, timer_cb, 1);
}

static void timer_cb(zaio_t *tm)
{
    if (zvar_sigint_flag == 1) {
        /* 这段代码是为了检查内存泄露 */
        fprintf(stderr, "\r                          \n");
        zinfo("signal SIGINT, then sleep 3, then EXIT");
        flag_stop = 1;
        zaio_base_stop_notify(zaio_get_aio_base(tm));
        /* 等线程池退出 */
        zsleep(3);
        zaio_free(tm, 1);
        zaio_free(listen_aio, 1);
        zlist_free(info_list);
        return;
    }

#if 0
    char tbuf[128];
    zinfo("%s client count: %d", get_current_time(tbuf), current_client);
    zaio_sleep(tm, timer_cb, 1);
#else 
    /* 仅仅为了测试, 我们把此功能也移动到线程池 */
    /* 因为此aio离开了aio_base 环境, 必须执行 disable */
    zaio_disable(tm);

    info_t *info = (info_t *)zcalloc(1, sizeof(info_t));
    info->aio = tm;
    info->other_data = 0;
    info->foo = foo_timer_cb;

    pthread_mutex_lock(&locker);
    zlist_push(info_list, info);
    pthread_mutex_unlock(&locker);
    pthread_cond_signal(&cond);
#endif
}

static void after_read(zaio_t *aio);
static void after_write(zaio_t *aio)
{
    /* 当前线程是主线程 */
    if (zaio_get_result(aio) < 1) {
        connect_quit(aio, "write error");
        return;
    }
    zaio_gets(aio, 10240, after_read);
}

static void after_write_and_exit(zaio_t *aio)
{
    if (zaio_get_result(aio) < 1) {
        connect_quit(aio, "write error");
    } else {
        connect_quit(aio, 0);
    }
}

static void foo(info_t *info)
{
    /* 这个时候已经在线程池了 */
    zaio_t *aio = info->aio;

    ZSTACK_BUF(bf, 10240);
    int ret = zaio_get_result(aio);
    if (ret < 1) {
        connect_quit(aio, "read error");
        return;
    }
    zaio_get_read_cache(aio, bf, ret);

    zbuf_trim_right_rn(bf);

    zaio_cache_write(aio, zbuf_data(bf), zbuf_len(bf));
    zaio_cache_write(aio, "\n", 1);
    if (!strcmp(zbuf_data(bf), "exit")) {
        zaio_cache_flush(aio, after_write_and_exit);
        return;
    }
    zaio_cache_flush(aio, after_write);
}

static void after_read(zaio_t *aio)
{
    /* 我们假设:
     * 在读取一行数据后, 要做一些复杂的业务,可能耗费较长时间,会阻塞...
     * 所以, 我们把业务处理交给线程池来处理 */

    /* 因为此aio准备离开 aio_base 环境, 必须执行 disable */
    zaio_disable(aio);

    info_t *info = (info_t *)zcalloc(1, sizeof(info_t));
    info->aio = aio;
    info->other_data = 0;
    info->foo = foo;

    pthread_mutex_lock(&locker);
    zlist_push(info_list, info);
    pthread_mutex_unlock(&locker);
    pthread_cond_signal(&cond);
}
static void before_accept(zaio_t *aio)
{
    int sock = zaio_get_fd(aio);
    int fd = zinet_accept(sock);
    if (fd < -1) {
        return;
    }

    pthread_mutex_lock(&count_locker);
    current_client++;
    all_client++;
    pthread_mutex_unlock(&count_locker);

    znonblocking(fd, 1);
    zaio_t *naio = zaio_create(fd, zaio_get_aio_base(aio));
    zaio_set_read_wait_timeout(naio, read_wait_timeout);
    zaio_set_write_wait_timeout(naio, write_wait_timeout);

    zaio_cache_puts(naio, "echo server, support command: exit\n");
    zaio_cache_flush(naio, after_write);
}

int main(int argc, char **argv)
{
    zmain_argument_run(argc, argv, 0);
    
    read_wait_timeout = zconfig_get_second(zvar_default_config, "read_wait_timeout", 3600*24, -1, 3600*24);
    write_wait_timeout = zconfig_get_second(zvar_default_config, "write_wait_timeout", 3600*24, -1, 3600*24);

    char *listen = zconfig_get_str(zvar_default_config, "listen", 0);
    if (zempty(listen)) {
        zinfo("default listen on 0:8899");
        zinfo("USAGE %s -listen 0:8899 [ -read_wait_timeout 1d ] [ -write_wait_timeout 1d ]    #", argv[0]);
        listen = "0:8899";
    }

    info_list = zlist_create();
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

    zaio_base_t *aiobase = zaio_base_create();

    listen_aio = zaio_create(sock, aiobase);
    zaio_readable(listen_aio, before_accept);

    tm = zaio_create(-1, aiobase);
    zaio_sleep(tm, timer_cb, 1);

    zaio_base_run(aiobase, 0);

    zaio_base_free(aiobase);

    return 0;
}

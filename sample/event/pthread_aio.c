/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2016-04-22
 * ================================
 */

#include "zc.h"
#include <pthread.h>
#include <time.h>

typedef int (*send_cb_t) (zaio_t *);
typedef struct {
    zaio_t *aio;
    send_cb_t callback;
} send_obj_t;

static zlist_t *aio_list;
static pthread_mutex_t locker = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static zevent_base_t *evbase;

static void after_write(zaio_t * aio);
static void send_aio_to_another_pthread(zaio_t * aio, send_cb_t callback);

static int service_error(zaio_t * aio)
{
    zinfo("service_error %d: error or idle too long, ret %d", zaio_get_fd(aio), zaio_get_result(aio));
    zaio_free(aio, 1);
    return -1;
}

static int another_after_read(zaio_t * aio)
{
    zinfo("after_read");
    int ret, len;
    char *rbuf;

    ret = zaio_get_result(aio);
    if (ret < 1) {
        return service_error(aio);
    }

    zbuf_t *bf = zbuf_create(1024);
    zaio_fetch_rbuf(aio, bf, ret);
    zbuf_trim_right_rn(bf);
    rbuf = zbuf_data(bf);
    len = zbuf_len(bf);

    if (ret > 3 && !strncmp(rbuf, "exit", 4)) {
        zaio_free(aio, 1);
        return 0;
    }

    rbuf[ret] = 0;

    zaio_cache_puts(aio, "your input:   ");
    zaio_cache_write(aio, rbuf, len);
    zaio_cache_puts(aio, "\n");
    zaio_cache_flush(aio, after_write, 10);

    return 0;
}

static void after_read(zaio_t * aio)
{
    send_aio_to_another_pthread(aio, another_after_read);
}

static int another_after_write(zaio_t * aio)
{
    int ret;
    zinfo("before_write");

    ret = zaio_get_result(aio);

    if (ret < 1) {
        return service_error(aio);
    }

    zaio_gets(aio, 1024, after_read, 10 * 1000);

    return 0;
}

static void after_write(zaio_t * aio)
{
    send_aio_to_another_pthread(aio, another_after_write);
}

static void another_welcome(zaio_t * aio)
{
    time_t t = time(0);

    zaio_cache_printf_1024(aio, "welcome aio: %s\n", ctime(&t));
    zaio_cache_flush(aio, after_write, 10);
}

static void welcome(zaio_t * aio)
{
    send_aio_to_another_pthread(aio, (send_cb_t) another_welcome);
}

static void before_accept(zeio_t * ev)
{
    zinfo("before_accept");
    int sock;
    int fd;
    zaio_t *aio;

    sock = zeio_get_fd(ev);
    fd = zinet_accept(sock);
    if (fd < -1) {
        zinfo("accept fail");
        return;
    }
    znonblocking(fd, 1);
    aio = zaio_create(fd, evbase);
    welcome(aio);
}

static void timer_cb(zetimer_t * zt)
{
    zinfo("now exit!");
    exit(1);
}

void *another_pthread_deal_aio(void *arg)
{
    send_obj_t *obj;

    while (1) {
        pthread_mutex_lock(&locker);
        while (!zlist_head(aio_list)) {
            pthread_cond_wait(&cond, &locker);
        }
        obj = 0;
        zlist_shift(aio_list, (void **)&obj);
        pthread_mutex_unlock(&locker);

        if (obj) {
            (obj->callback) (obj->aio);
            zfree(obj);
        }
    }

    return arg;
}

static void send_aio_to_another_pthread(zaio_t * aio, send_cb_t callback)
{
    long r;
    send_obj_t *obj;

    r = ztimeout_set(0);
    if (r % 3 == 0) {
        callback(aio);
        return;
    }

    obj = (send_obj_t *) zcalloc(1, sizeof(send_obj_t));
    obj->aio = aio;
    obj->callback = callback;

    pthread_mutex_lock(&locker);
    zlist_push(aio_list, obj);
    pthread_mutex_unlock(&locker);
    pthread_cond_signal(&cond);
}

pthread_t m_pth;
static void ___log(const char *fn, size_t line, const char *fmt, va_list ap)
{
    pthread_t c_pth = pthread_self();

    fprintf(stderr, "%s[%lu]: ", (pthread_equal(m_pth, c_pth) ? "pth1" : "pth2"), pthread_self());
    vfprintf(stderr, fmt, ap);
    printf("\n");
}

int main(int argc, char **argv)
{
    int port;
    int sock;
    zeio_t *ev;
    zetimer_t *tm;
    pthread_t pth;

    m_pth = pthread_self();

    zlog_vprintf = ___log;

    aio_list = zlist_create();

    pthread_create(&pth, 0, another_pthread_deal_aio, 0);

    port = 8899;
    evbase = zevent_base_create();

    sock = zinet_listen(0, port, 5, 1);
    ev = zeio_create(sock, evbase);
    zeio_enable_read(ev, before_accept);

    tm = zetimer_create(evbase);
    zetimer_start(tm, timer_cb, 200);

    while(zevent_base_dispatch(evbase)) {
    }

    return 0;
}

/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-04-22
 * ================================
 */

#include "libzc.h"
#include <pthread.h>
#include <time.h>

typedef int (*send_cb_t) (zaio_t *);
typedef struct {
    zaio_t *aio;
    send_cb_t callback;
} send_obj_t;

static zchain_t *aio_chain;
static pthread_mutex_t locker = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

static int after_write(zaio_t * aio);
static void send_aio_to_another_pthread(zaio_t * aio, send_cb_t callback);

static int service_error(zaio_t * aio)
{
    zinfo("service_error");
    int fd;

    fd = zaio_get_fd(aio);

    zinfo("%d: error or idle too long, ret %d", fd, zaio_get_ret(aio));
    zaio_fini(aio);
    zaio_free(aio);
    close(fd);

    return -1;
}

static int another_after_read(zaio_t * aio)
{
    zinfo("after_read");
    int ret, fd, len;
    char rbuf[102400];
    char *p;

    ret = zaio_get_ret(aio);
    fd = zaio_get_fd(aio);
    if (ret < 1) {
        return service_error(aio);
    }
    zaio_fetch_rbuf(aio, rbuf, ret);

    if (ret > 3 && !strncmp(rbuf, "exit", 4)) {
        zaio_fini(aio);
        zaio_free(aio);
        close(fd);
        return 0;
    }

    rbuf[ret] = 0;
    p = strchr(rbuf, '\r');
    if (p) {
        *p = 0;
    }
    p = strchr(rbuf, '\n');
    if (p) {
        *p = 0;
    }
    len = strlen(rbuf);

    zaio_write_cache_append(aio, "your input:   ", 12);
    zaio_write_cache_append(aio, rbuf, len);
    zaio_write_cache_append(aio, "\n", 1);
    zaio_write_cache_flush(aio, after_write, 10 * 1000);

    return 0;
}

static int after_read(zaio_t * aio)
{
    send_aio_to_another_pthread(aio, another_after_read);

    return 0;
}

static int another_after_write(zaio_t * aio)
{
    int ret;
    zinfo("before_write");

    ret = zaio_get_ret(aio);

    if (ret < 1) {
        return service_error(aio);
    }

    zaio_read_delimiter(aio, '\n', 1024, after_read, 10 * 1000);

    return 0;
}

static int after_write(zaio_t * aio)
{
    send_aio_to_another_pthread(aio, another_after_write);

    return 0;
}

static void another_welcome(zaio_t * aio)
{
    time_t t = time(0);

    zaio_printf_1024(aio, "welcome aio: %s\n", ctime(&t));
    zaio_write_cache_flush(aio, after_write, 10 * 1000);
}

static void welcome(zaio_t * aio)
{
    send_aio_to_another_pthread(aio, (send_cb_t) another_welcome);
}

static int before_accept(zev_t * ev)
{
    zinfo("before_accept");
    int sock;
    int fd;
    zaio_t *aio;

    sock = zev_get_fd(ev);
    fd = zinet_accept(sock);
    if (fd < -1) {
        zinfo("accept fail");
        return 0;
    }
    znonblocking(fd, 1);
    aio = zaio_create();
    zaio_init(aio, zvar_evbase, fd);

    welcome(aio);

    return 0;
}

static int timer_cb(zevtimer_t * zt)
{
    zinfo("now exit!");
    exit(1);

    return 0;
}

void *another_pthread_deal_aio(void *arg)
{
    send_obj_t *obj;

    while (1) {
        pthread_mutex_lock(&locker);
        while (!ZCHAIN_HEAD(aio_chain)) {
            pthread_cond_wait(&cond, &locker);
        }
        obj = 0;
        zchain_shift(aio_chain, (char **)&obj);
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
    zchain_push(aio_chain, obj);
    pthread_mutex_unlock(&locker);
    pthread_cond_signal(&cond);
}

pthread_t m_pth;
static int ___log(int level, char *fmt, va_list ap)
{
    char log_buf[102400];
    int len;
    pthread_t c_pth = pthread_self();

    len = sprintf(log_buf, "%s[%d][%lu]: ", (pthread_equal(m_pth, c_pth) ? "pth1" : "pth2"), getpid(), pthread_self());
    len += zvsnprintf(log_buf + len, 102000 - len - 2, fmt, ap);
    log_buf[len] = '\n';
    log_buf[len + 1] = 0;
    fputs(log_buf, stderr);

    return 0;
}

int main(int argc, char **argv)
{
    int port;
    int sock;
    zev_t *ev;
    zevtimer_t tm;
    pthread_t pth;

    m_pth = pthread_self();

    zlog_set_voutput(___log);

    aio_chain = zchain_create();

    pthread_create(&pth, 0, another_pthread_deal_aio, 0);

    port = 8899;
    zvar_evbase = zevbase_create();

    sock = zinet_listen(0, port, 5);
    znonblocking(sock, 1);
    ev = zev_create();
    zev_init(ev, zvar_evbase, sock);
    zev_read(ev, before_accept);

    zevtimer_init(&tm, zvar_evbase);
    zevtimer_start(&tm, timer_cb, 200 * 1000);

    while (1) {
        zevbase_dispatch(zvar_evbase, 0);
    }

    return 0;
}

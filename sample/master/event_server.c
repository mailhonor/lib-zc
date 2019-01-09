/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2015-11-24
 * ================================
 */

#include "zc.h"
#include <time.h>

static void after_write(zaio_t *aio);
static void service_error(zaio_t *aio)
{
    zinfo("%d: error or idle too long", zaio_get_fd(aio));
    zaio_free(aio, 1);
}

static void after_read(zaio_t *aio)
{
    int ret, len;
    ZSTACK_BUF(rbuf_buf, 102400);
    char *rbuf = zbuf_data(rbuf_buf);

    ret = zaio_get_result(aio);
    if (ret < 1) {
        return service_error(aio);
    }
    zaio_fetch_rbuf(aio, rbuf_buf, ret);

    if (ret > 3 && !strncasecmp(rbuf, "exit", 4)) {
        zaio_free(aio, 1);
        if (!strncmp(rbuf, "EXIT", 4)) {
            zevent_server_stop_notify();
        }
        return;
    }

    zbuf_trim_right_rn(rbuf_buf);
    len = zbuf_len(rbuf_buf);
    zaio_cache_write(aio, "your input:   ", 12);
    zaio_cache_write(aio, rbuf, len);
    zaio_cache_puts(aio, "\n");
    zaio_cache_flush(aio, after_write, 1);
}

static void after_write(zaio_t *aio)
{
    int ret;
    zinfo("before_write");

    ret = zaio_get_result(aio);

    if (ret < 1) {
        service_error(aio);
        return;
    }
    zaio_gets(aio, 1024, after_read, 10);
}

static int count = 0;
static void simple_service(int fd)
{
    znonblocking(fd, 1);
    zaio_t *aio = zaio_create(fd, zvar_default_event_base);
    count++;
    zaio_cache_printf_1024(aio, "welcome No.%d\n", count);
    zaio_cache_flush(aio, after_write, 1);
}

static void service_register (const char *service, int fd, int fd_type)
{
    zevent_server_general_aio_register(zvar_default_event_base, fd, fd_type, simple_service);
}

static void before_reload()
{
}

int main(int argc, char **argv)
{
    zevent_server_service_register = service_register;
    zevent_server_before_reload = before_reload;
    zevent_server_main(argc, argv);
    return 0;
}

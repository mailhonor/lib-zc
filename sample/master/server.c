/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-11-24
 * ================================
 */

#include "libzc.h"
#include <time.h>

static int var_sleep;
static void fn_before(void)
{
    var_sleep = zconfig_get_second(zvar_config, "sleep", 10);
    zinfo("before: %d", var_sleep);
}

static void exit_sleep(void)
{
    if(var_sleep > 0)
    {
        zinfo("sleep %ds before exit", var_sleep);
        zsleep(var_sleep);
    }
}

static int after_write(zaio_t * aio, char *wbuf);
static int service_error(zaio_t * aio)
{
    int fd;

    fd = zaio_get_fd(aio);

    zinfo("%d: error or idle too long", fd);
    zaio_fini(aio);
    zfree(aio);
    close(fd);

    return -1;
}

static int after_read(zaio_t * aio, char *rbuf)
{
    int ret, fd, len;
    char *p;

    ret = zaio_get_ret(aio);
    fd = zaio_get_fd(aio);
    if (ret < 1)
    {
        return service_error(aio);
    }

    if (ret > 3 && !strncmp(rbuf, "exit", 4))
    {
        zaio_fini(aio);
        zfree(aio);
        close(fd);
        return 0;
    }

    if (ret > 3 && !strncmp(rbuf, "EXIT", 4))
    {
        zaio_fini(aio);
        zfree(aio);
        close(fd);
        zmaster_server_stop_notify();
        return 0;
    }

    rbuf[ret] = 0;
    p = strchr(rbuf, '\r');
    if (p)
    {
        *p = 0;
    }
    p = strchr(rbuf, '\n');
    if (p)
    {
        *p = 0;
    }
    len = strlen(rbuf);

    zaio_write_cache_append(aio, "your input:   ", 12);
    zaio_write_cache_append(aio, rbuf, len);
    zaio_write_cache_append(aio, "\n", 1);
    zaio_write_cache_flush(aio, after_write);

    return 0;
}

static int after_write(zaio_t * aio, char *wbuf)
{
    int ret;

    ret = zaio_get_ret(aio);

    if (ret < 1)
    {
        return service_error(aio);
    }

    zaio_read_delimiter(aio, '\n', 1024, after_read);

    return 0;
}

static int attach_to_evbase(zaio_t * aio, char *unused)
{
    char buf[1024];
    time_t t = time(0);

    sprintf(buf, "welcome aio: %s\n", ctime(&t));
    zaio_write_cache_append(aio, buf, strlen(buf));
    zaio_write_cache_flush(aio, after_write);
    return 0;
}

static void service(int fd)
{
    /*
     * Here, we get a fd. So we can choice any framework.
     * I choice the aio model in this case.
     * The reference example ../event/abc.c
     */
    zaio_t *aio = zaio_create();

    znonblocking(fd, 1);
    zaio_init(aio, zvar_evbase, fd);
    zaio_attach(aio, attach_to_evbase);
}

int main(int argc, char **argv)
{
    zmaster_server_service = service;
    zmaster_server_before_service = fn_before;
    zmaster_server_loop = 0;
    zmaster_server_before_exit = exit_sleep;

    zmaster_server_main(argc, argv);

    return 0;
}

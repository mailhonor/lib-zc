/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-11-20
 * ================================
 */

#include "zc.h"
#include <signal.h>

void (*zmaster_server_simple_service) (int fd) = 0;
void (*zmaster_server_service_register) (const char *service, int fd, int fd_type) = 0;

void (*zmaster_server_before_service) (void) = 0;
void (*zmaster_server_event_loop) (void) = 0;
void (*zmaster_server_before_reload) (void) = 0;
void (*zmaster_server_before_exit) (void) = 0;

zevbase_t *zvar_master_server_evbase = 0;
int zvar_master_server_reloading = 0;
int zvar_master_server_stopping = 0;

static int test_mode = 0;
static pid_t parent_pid;
static zev_t *ev_listen = 0;
static zev_t *ev_status = 0;
static zevtimer_t reload_timer;

static void reloading_to_stopping(zevtimer_t *tm)
{
    zvar_master_server_stopping = 1;
}

static void on_master_reload(zev_t * zev)
{
    zvar_master_server_reloading = 1;
    if (getppid() == parent_pid) {
        if (zmaster_server_before_reload) {
            zmaster_server_before_reload();
        }
        zevtimer_start(&reload_timer, reloading_to_stopping, 10 * 1000);
    } else {
        zmaster_server_stop_notify();
    }
}

#define local_ev_close()    if(local_ev_close_do_once){local_ev_close_do_once=0;local_ev_close_do();}
static int local_ev_close_do_once;
static void local_ev_close_do()
{
    if (zmaster_server_simple_service  && ev_listen) {
        int fd = zev_get_fd(ev_listen);
        zev_unset(ev_listen);
        zev_fini(ev_listen);
        zfree(ev_listen);
        ev_listen = 0;
        close(fd);
    }
    if (ev_status) {
        /* The master would receive the signal of closing ZMASTER_SERVER_STATUS_FD. */
        zev_unset(ev_status);
        zev_fini(ev_status);
        zfree(ev_status);
        ev_status = 0;
        close(ZMASTER_SERVER_STATUS_FD);
        close(ZMASTER_MASTER_STATUS_FD);
    }
}

static void service_register(char *s, char *optval)
{
    int t = optval[0];
    switch(t) {
        case ZMASTER_SERVER_LISTEN_INET:
        case ZMASTER_SERVER_LISTEN_UNIX:
        case ZMASTER_SERVER_LISTEN_FIFO:
            break;
        default:
            zfatal("zmaster_server: unknown service type %c", t);
            break;
    }
    int fd = atoi(optval+1);
    if (fd < 1) {
        zfatal("zmaster_server: fd is invalid", optval+1);
    }
    zclose_on_exec(fd, 1);
    znonblocking(fd, 1);
    if (zmaster_server_simple_service) {
        ev_listen = zmaster_server_general_aio_register(zvar_master_server_evbase, fd, t, zmaster_server_simple_service);
    } else {
        zmaster_server_service_register(s, t, fd);
    }
}

static void test_register(char *test_url)
{
    char service_buf[1024];
    char *service, *url, *p;

    strcpy(service_buf, test_url);
    p = strstr(service_buf, "://");
    if (p) {
        *p=0;
        service = service_buf;
        url = p+1;
    } else {
        service = zblank_buffer;
        url = service_buf;
    }

    int fd_type;
    int fd = zlisten(url, 5,  &fd_type);
    if (fd < 0) {
        zfatal("zmaster_server_test: open %s(%m)", test_url);
    }
    zclose_on_exec(fd, 1);
    znonblocking(fd, 1);
    if (zmaster_server_simple_service) {
        ev_listen = zmaster_server_general_aio_register(zvar_master_server_evbase, fd, fd_type, zmaster_server_simple_service);
    } else {
        zmaster_server_service_register(service, fd, fd_type);
    }
}

static void ___usage(char *parameter)
{
    printf("do not run this command by hand\n");
    exit(1);
}

static void  main_init(int argc, char **argv)
{
    zvar_progname = argv[0];
    signal(SIGPIPE, SIG_IGN);
    parent_pid = getppid();

    test_mode = 1;
    ev_status = 0;
    ev_listen = 0;
    local_ev_close_do_once = 1;

    ZPARAMETER_BEGIN() {
        if (!strcmp(optname, "-M")) {
            test_mode = 0;
            opti += 1;
            continue;
        }
        if (optval == 0) {
            ___usage(0);
        }
        if (!strncmp(optname, "-s", 2)) {
            service_register(optname+2, optval);
            opti += 2;
            continue;
        }
        if (!strncmp(optname, "-l", 2)) {
            test_register(optval);
            opti += 2;
            continue;
        }
    } ZPARAMETER_END;

    if (!test_mode) {
        ev_status = (zev_t *) zcalloc(1, sizeof(zev_t));
        zclose_on_exec(ZMASTER_MASTER_STATUS_FD, 1);
        znonblocking(ZMASTER_MASTER_STATUS_FD, 1);

        zev_init(ev_status, zvar_master_server_evbase, ZMASTER_MASTER_STATUS_FD);
        zev_read(ev_status, on_master_reload);
    }

    if (zmaster_server_before_service) {
        zmaster_server_before_service();
    }

}

int zmaster_server_main(int argc, char **argv)
{
    main_init(argc, argv);
    while (1) {
        zevbase_dispatch(zvar_master_server_evbase, 0);
        if (zmaster_server_event_loop) {
            zmaster_server_event_loop();
        }
        if (zvar_master_server_reloading) {
            local_ev_close();
            break;
        }
        if (zvar_master_server_stopping) {
            local_ev_close();
            break;
        }
    }

    local_ev_close();

    if (zmaster_server_before_exit) {
        zmaster_server_before_exit();
    }

    zevtimer_fini(&reload_timer);

    zevbase_free(zvar_master_server_evbase);

    return 0;
}

void zmaster_server_stop_notify(void)
{
    zvar_master_server_stopping = 1;
    zevbase_notify(zvar_master_server_evbase);
}


/* **************************************** */
static void ___inet_server_accept(zev_t * zev)
{
    int fd, listen_fd;
    void (*cb)(int) = (void(*)(int))zev_get_context(zev);

    listen_fd = zev_get_fd(zev);

    fd = zinet_accept(listen_fd);
    if (fd < 0) {
        if (errno != EAGAIN) {
            zfatal("inet_server_accept: %m");
        }
        return;
    }
    znonblocking(fd, 1);
    zclose_on_exec(fd, 1);
    cb(fd);
}

static void ___unix_server_accept(zev_t * zev)
{
    int fd, listen_fd;
    void (*cb)(int) = (void(*)(int))zev_get_context(zev);

    listen_fd = zev_get_fd(zev);

    fd = zunix_accept(listen_fd);
    if (fd < 0) {
        if (errno != EAGAIN) {
            zfatal("unix_server_accept: %m");
        }
        return;
    }
    znonblocking(fd, 1);
    zclose_on_exec(fd, 1);
    cb(fd);
}

zev_t *zmaster_server_general_aio_register(zevbase_t *eb, int fd, int fd_type, void (*callback) (int))
{
    zev_t *ev = (zev_t *) zcalloc(1, sizeof(zev_t));
    zev_init(ev, eb, fd);
    zev_set_context(ev, callback);
    if (fd_type == ZMASTER_SERVER_LISTEN_INET) {
        zev_read(ev, ___inet_server_accept);
    } else if (fd_type == ZMASTER_SERVER_LISTEN_UNIX) {
        zev_read(ev, ___unix_server_accept);
    } else if (fd_type == ZMASTER_SERVER_LISTEN_FIFO) {
    }
    return ev;
}


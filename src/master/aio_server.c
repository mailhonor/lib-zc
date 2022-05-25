/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2015-11-20
 * ================================
 */

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include "zc.h"
#include "errno.h"
#include "master.h"
#include <signal.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/prctl.h>

void (*zaio_server_service_register) (const char *service, int fd, int fd_type) = 0;
void (*zaio_server_before_service) (void) = 0;
void (*zaio_server_before_softstop) (void) = 0;

static zbool_t flag_run = 0;
static zbool_t flag_softstop = 0;
static zbool_t flag_stop = 0;
static zbool_t master_mode = 0;
static zbool_t flag_detach_from_master = 0;
static pid_t parent_pid = 0;
static zaio_t *ev_status = 0;
static zvector_t *event_ios; /* zeiot_t* */
static zaio_t *stop_timer = 0;
static zaio_t *detach_from_master_timer = 0;
static zbool_t flag_zvar_aio_server_status_fd_closed = 0;

static void stop_now(zaio_t *tm)
{
    flag_stop = 1;
}

static void stop_now_and_release_self(zaio_t *tm)
{
    flag_stop = 1;
    zaio_free(tm, 1);
}

static void detach_from_master_check(zaio_t *tm)
{
}

static void on_master_softstop(zaio_t *ev)
{
    flag_softstop = 1;

    if(flag_detach_from_master) {
        zaio_free(ev_status, 1);
        ev_status = 0;
        return;
    }

    int need_stop_timer = 0;
    int exit_after = (int)zconfig_get_second(zvar_default_config, "server-stop-on-softstop-after", 0);

    if (zaio_server_before_softstop) {
        need_stop_timer = 1;
        if (exit_after < 1) {
            exit_after = 3600;
        }
        zaio_server_before_softstop();
    } else {
        if (exit_after > 0) {
            need_stop_timer = 1;
        } else {
            flag_stop = 1;
        }
    }
    if (need_stop_timer) {
        if (ev_status) {
            stop_timer = zaio_create(-1, zvar_default_aio_base);
            zaio_sleep(stop_timer, stop_now, exit_after);
            zaio_free(ev, 1);
            ev_status = 0;
        }
    }
}

static void ___inet_server_accept(zaio_t *ev)
{
    int fd, listen_fd = zaio_get_fd(ev);
    void (*cb)(int) = (void(*)(int))zaio_get_context(ev);

    if (flag_softstop) {
        zaio_free(ev, 0);
        return;
    }

    fd = zinet_accept(listen_fd);
    if (fd < 0) {
        int ec = errno;
        if (ec == EINTR) {
            return;
        }
        if (ec == EAGAIN) {
            return;
        }
        zfatal("FATAL inet_server_accept: %m");
        return;
    }
    zclose_on_exec(fd, 1);
    cb(fd);
}

static void ___unix_server_accept(zaio_t *ev)
{
    int fd, listen_fd = zaio_get_fd(ev);
    void (*cb)(int) = (void(*)(int))zaio_get_context(ev);

    if (flag_softstop) {
        zaio_free(ev, 0);
        return;
    }

    fd = zunix_accept(listen_fd);
    if (fd < 0) {
        int ec = errno;
        if (ec == EINTR) {
            return;
        }
        if (ec == EAGAIN) {
            return;
        }
        zfatal("FATAL inet_server_accept: %m");
        return;
    }
    zclose_on_exec(fd, 1);
    cb(fd);
}

static void ___fifo_server_accept(zaio_t *ev)
{
    int listen_fd = zaio_get_fd(ev);
    void (*cb)(int) = (void(*)(int))zaio_get_context(ev);

    if (flag_softstop) {
        zaio_free(ev, 0);
        return;
    }

    cb(listen_fd);
}

static void alone_register(char *alone_url)
{
    char service_buf[1024];
    char *service, *url, *p;
    zstrtok_t splitor;
    zstrtok_init(&splitor, alone_url);
    while(zstrtok(&splitor, " \t,;\r\n")) {
        if (splitor.len > 1000) {
            zfatal("FATAL alone_register: url too long");
        }
        memcpy(service_buf, splitor.str, splitor.len);
        service_buf[splitor.len] = 0;
        p = strstr(service_buf, "://");
        if (p) {
            *p=0;
            service = service_buf;
            url = p+3;
        } else {
            service = zblank_buffer;
            url = service_buf;
        }

        int fd_type;
        int fd = zlisten(url,  &fd_type, 5);
        if (fd < 0) {
            zfatal("FATAL alone_register: open %s(%m)", alone_url);
        }
        zclose_on_exec(fd, 1);
        zaio_server_service_register(service, fd, fd_type);
    }
}

static void master_register(char *master_url)
{
    zargv_t *service_argv = zargv_create(-1);
    zargv_split_append(service_argv, master_url, ",");
    for (int i = 0; i < zargv_len(service_argv); i++) {
        char *service_name, *typefd;
        zargv_t *stfd = zargv_create(5);
        zargv_split_append(stfd, zargv_data(service_argv)[i], ":");
        if (zargv_len(stfd) == 1) {
            service_name = zblank_buffer;
            typefd = zargv_data(stfd)[0];
        } else {
            service_name = zargv_data(stfd)[0];
            typefd = zargv_data(stfd)[1];
        }
        int fdtype = typefd[0];
        switch(fdtype) {
            case zvar_tcp_listen_type_inet:
            case zvar_tcp_listen_type_unix:
            case zvar_tcp_listen_type_fifo:
                break;
            default:
                zfatal("FATAL master_aio_server: unknown service type %c", fdtype);
                break;
        }
        int fd = atoi(typefd+1);
        if (fd < zvar_aio_server_listen_fd) {
            zfatal("FATAL master_aio_server: fd(%s) is invalid", typefd+1);
        }
        znonblocking(fd, 0);
        zclose_on_exec(fd, 1);
        zaio_server_service_register(service_name, fd, fdtype);
        zargv_free(stfd);
    }
    zargv_free(service_argv);
}

zaio_t *zaio_server_general_aio_register(zaio_base_t *eb, int fd, int fd_type, void (*callback) (int))
{
    zclose_on_exec(fd, 1);
    znonblocking(fd, 1);

    zaio_t *ev = zaio_create(fd, eb);
    zaio_set_context(ev, callback);
    if (fd_type == zvar_tcp_listen_type_inet) {
        zaio_readable(ev, ___inet_server_accept);
    } else if (fd_type == zvar_tcp_listen_type_unix) {
        zaio_readable(ev, ___unix_server_accept);
    } else if (fd_type == zvar_tcp_listen_type_fifo) {
        zaio_readable(ev, ___fifo_server_accept);
    }
    if (!event_ios) {
        event_ios = zvector_create(-1);
    }
    zvector_push(event_ios, ev);
    return ev;
}

static void deal_argument()
{
    char *s = zvar_main_redundant_argv[0];
    if (zempty(s)) {
        s = zblank_buffer;
    }
    if (!strcmp(s, "MASTER")) {
        master_mode = 1;
    } else if (!strcmp(s, "alone")) {
        master_mode = 0;
    } else {
        printf("FATAL USAGE: %s alone [ ... ] -server-service 0:8899 [ ... ]\n", zvar_progname);
        zfatal("FATAL USAGE: %s alone [ ... ] -server-service 0:8899 [ ... ]", zvar_progname);
    }
}

static void zaio_server_main_loop(zaio_base_t *eb)
{
    if (flag_stop || zvar_sigint_flag) {
        zaio_base_stop_notify(zvar_default_aio_base);
        return;
    }

    static int loop_times = 0;
    if (loop_times++ > 10) {
        if (getppid() != parent_pid) {
            zaio_base_stop_notify(zvar_default_aio_base);
        }
        loop_times = 0;
    }

    if (master_mode) {
        static int detach_dealed = 0;
        if (flag_detach_from_master && (!detach_dealed)) {
            detach_dealed = 1;
            if (!flag_zvar_aio_server_status_fd_closed) {
                flag_zvar_aio_server_status_fd_closed = 1;
                zclose(zvar_aio_server_status_fd);
            }
            int exit_after = (int)zconfig_get_second(zvar_default_config, "server-stop-on-softstop-after", 0);
            if (exit_after < 1) {
                exit_after = 3600;
            }
            zaio_sleep(zaio_create(-1, zvar_default_aio_base), stop_now_and_release_self, exit_after);
            if (event_ios) {
                ZVECTOR_WALK_BEGIN(event_ios, zaio_t *, eio) {
                    zaio_disable(eio);
                } ZVECTOR_WALK_END;
            }
        }
    }
}

static void zaio_server_init(int argc, char **argv)
{
    if (flag_run) {
        zfatal("FATAL zaio_server_main: only once");
    }
    flag_run = 1;

    zmain_argument_run(argc, argv);
    deal_argument();

    zsignal_ignore(SIGPIPE);
    prctl(PR_SET_PDEATHSIG, SIGHUP);

    parent_pid = getppid();
    if (parent_pid == 1) {
        exit(1);
    }

    if (!zaio_server_service_register) {
        zfatal("FATAL zaio_server_service_register is null");
    }
    char *attr;

    zvar_default_aio_base = zaio_base_create();
    zaio_base_set_loop_fn(zvar_default_aio_base, zaio_server_main_loop);

    attr = zconfig_get_str(zvar_default_config, "server-config-path", "");
    if (!zempty(attr)) {
        zconfig_t *cf = zconfig_create();
        zmaster_load_global_config_from_dir_inner(cf, attr);
        zconfig_load_annother(cf, zvar_default_config);
        zconfig_load_annother(zvar_default_config, cf);
        zconfig_free(cf);
    }

    zmaster_log_use_inner(argv[0], zconfig_get_str(zvar_default_config, "server-log", ""));

    if (master_mode) {
        zclose_on_exec(zvar_master_server_status_fd, 1);
        znonblocking(zvar_master_server_status_fd, 1);
        ev_status = zaio_create(zvar_master_server_status_fd, zvar_default_aio_base);
        zaio_readable(ev_status, on_master_softstop);
    }

    if (zaio_server_before_service) {
        zaio_server_before_service();
    }

    if (master_mode == 0) {
        alone_register(zconfig_get_str(zvar_default_config, "server-service", ""));
    } else {
        master_register(zconfig_get_str(zvar_default_config, "server-service", ""));
    }

    long ea = zconfig_get_second(zvar_default_config, "exit-after", 0);
    if (ea > 0) {
        alarm(0);
        zaio_sleep(zaio_create(-1, zvar_default_aio_base), stop_now_and_release_self, (int)ea);
    }
    if (master_mode) {
        detach_from_master_timer = zaio_create(-1, zvar_default_aio_base);
        zaio_sleep(detach_from_master_timer, detach_from_master_check, 10);
    }
}

static void zaio_server_fini()
{
    if (zvar_memleak_check == 0) {
        return;
    }
    if (ev_status) {
        zaio_free(ev_status, 0);
        ev_status = 0;
    }
    if (detach_from_master_timer) {
        zaio_free(detach_from_master_timer, 1);
        detach_from_master_timer = 0;

        /* The master would receive the signal of closing zvar_aio_server_status_fd */
        if (!flag_zvar_aio_server_status_fd_closed) {
            flag_zvar_aio_server_status_fd_closed = 1;
            zclose(zvar_aio_server_status_fd);
        }
    }

    if (event_ios) {
        ZVECTOR_WALK_BEGIN(event_ios, zaio_t *, eio) {
            zaio_free(eio, 1);
        } ZVECTOR_WALK_END;
        zvector_free(event_ios);
        event_ios = 0;
    }
    if (stop_timer) {
        zaio_free(stop_timer, 1);
        stop_timer = 0;
    }

    zaio_base_free(zvar_default_aio_base);

    zsleep(1);
}

int zaio_server_main(int argc, char **argv)
{
    zaio_server_init(argc, argv);
    zaio_base_run(zvar_default_aio_base);
    zaio_server_fini();
    return 0;
}

void zaio_server_stop_notify(int stop_after_second)
{
    if (stop_after_second < 1) {
        flag_stop = 1;
        return;
    }
    zaio_sleep(zaio_create(-1, zvar_default_aio_base), stop_now_and_release_self, stop_after_second);
}

void zaio_server_detach_from_master()
{
    flag_detach_from_master = 1;
}


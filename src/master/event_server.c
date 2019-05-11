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

void (*zevent_server_service_register) (const char *service, int fd, int fd_type) = 0;
void (*zevent_server_before_service) (void) = 0;
void (*zevent_server_event_loop) (void ) = 0;
void (*zevent_server_before_reload) (void) = 0;
void (*zevent_server_before_exit) (void) = 0;

static zbool_t flag_run = 0;
static zbool_t flag_stop = 0;
static zbool_t flag_reload = 0;
static zbool_t master_mode = 0;
static char *stop_file = 0;
static pid_t parent_pid = 0;
static zeio_t *ev_status = 0;
static zvector_t *event_ios; /* zeiot_t* */
static zetimer_t *stop_file_timer = 0;
static zetimer_t *stop_timer = 0;
static zetimer_t *exit_after_timer = 0;

static void sigterm_handler(int sig)
{
    zvar_proc_stop = 1;
}

static void sighup_handler(int sig)
{
    zvar_proc_stop = 1;
}

static void stop_file_check(zetimer_t *tm)
{
    if (zfile_get_size(stop_file) < 0) {
        zvar_proc_stop = 1;
    } else {
        zetimer_start(tm, stop_file_check, 1);
    }
}

static void stop_now(zetimer_t *tm)
{
    zvar_proc_stop = 1;
}

static void exit_after_check(zetimer_t *tm)
{
    zvar_proc_stop = 1;
    zetimer_free(exit_after_timer);
    exit_after_timer = 0;
}

static void on_master_reload(zeio_t *ev)
{
    flag_reload = 1;
    if (zevent_server_before_reload) {
        stop_timer = zetimer_create(zvar_default_event_base);
        zetimer_start(stop_timer, stop_now, 100);
        zeio_free(ev, 1);
        ev_status = 0;
        zevent_server_before_reload();
    } else {
        zvar_proc_stop = 1;
    }
}

static void ___inet_server_accept(zeio_t *ev)
{
    int fd, listen_fd = zeio_get_fd(ev);
    void (*cb)(int) = (void(*)(int))zeio_get_context(ev);

    if (flag_reload) {
        zeio_free(ev, 0);
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
        zfatal("inet_server_accept: %m");
        return;
    }
    zclose_on_exec(fd, 1);
    cb(fd);
}

static void ___unix_server_accept(zeio_t *ev)
{
    int fd, listen_fd = zeio_get_fd(ev);
    void (*cb)(int) = (void(*)(int))zeio_get_context(ev);

    if (flag_reload) {
        zeio_free(ev, 0);
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
        zfatal("inet_server_accept: %m");
        return;
    }
    zclose_on_exec(fd, 1);
    cb(fd);
}

static void ___fifo_server_accept(zeio_t *ev)
{
    int listen_fd = zeio_get_fd(ev);
    void (*cb)(int) = (void(*)(int))zeio_get_context(ev);

    if (flag_reload) {
        zeio_free(ev, 0);
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
            zfatal("alone_register: url too long");
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
        int fd = zlisten(url,  &fd_type, 5, 1);
        if (fd < 0) {
            zfatal("alone_register: open %s(%m)", alone_url);
        }
        znonblocking(fd, 0);
        zclose_on_exec(fd, 1);
        zevent_server_service_register(service, fd, fd_type);
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
                zfatal("master_event_server: unknown service type %c", fdtype);
                break;
        }
        int fd = atoi(typefd+1);
        if (fd < zvar_event_server_listen_fd) {
            zfatal("master_event_server: fd(%s) is invalid", typefd+1);
        }
        znonblocking(fd, 0);
        zclose_on_exec(fd, 1);
        zevent_server_service_register(service_name, fd, fdtype);
        zargv_free(stfd);
    }
    zargv_free(service_argv);
}

zeio_t *zevent_server_general_aio_register(zevent_base_t *eb, int fd, int fd_type, void (*callback) (int))
{
    znonblocking(fd, 1);

    zeio_t *ev = zeio_create(fd, eb);
    zeio_set_context(ev, callback);
    if (fd_type == zvar_tcp_listen_type_inet) {
        zeio_enable_read(ev, ___inet_server_accept);
    } else if (fd_type == zvar_tcp_listen_type_unix) {
        zeio_enable_read(ev, ___unix_server_accept);
    } else if (fd_type == zvar_tcp_listen_type_fifo) {
        zeio_enable_read(ev, ___fifo_server_accept);
    }
    if (!event_ios) {
        event_ios = zvector_create(-1);
    }
    zvector_push(event_ios, ev);
    return ev;
}

static unsigned int deal_argument(int argc, char **argv, int offset)
{
    if (offset == 1) {
        char *s = argv[1];
        if (!strcmp(s, "MASTER")) {
            master_mode = 1;
        } else if (!strcmp(s, "alone")) {
            master_mode = 0;
        } else {
            printf("ERR USAGE: %s alone ...\n", argv[0]);
            zfatal("ERR USAGE: %s alone ...\n", argv[0]);
        }
        return 1;
    }
    return 0;
}

static void zevent_server_init(int argc, char **argv)
{
    if (flag_run) {
        zfatal("zevent_server_main: only once");
    }
    flag_run = 1;

    signal(SIGPIPE, SIG_IGN);
    signal(SIGTERM, sigterm_handler);
    signal(SIGHUP, sighup_handler);
    prctl(PR_SET_PDEATHSIG, SIGHUP);
    parent_pid = getppid();

    if (parent_pid == 1) {
        exit(1);
    }

    if (!zevent_server_service_register) {
        zfatal("zevent_server_service_register is null");
    }
    char *attr;

    zmain_argument_run(argc, argv, deal_argument);

    zvar_default_event_base = zevent_base_create();

    attr = zconfig_get_str(zvar_default_config, "server-config-path", "");
    if (!zempty(attr)) {
        zconfig_t *cf = zconfig_create();
        zmaster_load_global_config_from_dir_inner(cf, attr);
        zconfig_load_annother(cf, zvar_default_config);
        zconfig_load_annother(zvar_default_config, cf);
        zconfig_free(cf);
    }

    stop_file = zconfig_get_str(zvar_default_config, "stop-file", "");

    if (zconfig_get_bool(zvar_default_config, "dev-mode", 0) == 0) {
        zmaster_log_use_inner(argv[0], zconfig_get_str(zvar_default_config, "server-log", ""));
    }

    if (master_mode) {
        zclose_on_exec(zvar_master_server_status_fd, 1);
        znonblocking(zvar_master_server_status_fd, 1);
        ev_status = zeio_create(zvar_master_server_status_fd, zvar_default_event_base);
        zeio_enable_read(ev_status, on_master_reload);
        if (!zempty(stop_file)) {
            stop_file_timer = zetimer_create(zvar_default_event_base);
            zetimer_start(stop_file_timer, stop_file_check, 1);
        }
    }

    if (zevent_server_before_service) {
        zevent_server_before_service();
    }

    if (master_mode == 0) {
        alone_register(zconfig_get_str(zvar_default_config, "server-service", ""));
    } else {
        master_register(zconfig_get_str(zvar_default_config, "server-service", ""));
    }

    attr = zconfig_get_str(zvar_default_config, "server-user", 0);
    if (!zempty(attr)) {
        if(!zchroot_user(0, attr)) {
            zfatal("ERR chroot_user %s", attr);
        }
    }

    long ea = zconfig_get_second(zvar_default_config, "exit-after", 0, 3600L * 24 * 365 * 10, 0);
    if (ea > 0) {
        alarm(0);
        exit_after_timer = zetimer_create(zvar_default_event_base);
        zetimer_start(exit_after_timer, exit_after_check, (int)ea);
    }
}

static void zevent_server_fini()
{
    if (ev_status) {
        zeio_free(ev_status, 1);
        ev_status = 0;
        zclose(zvar_master_server_status_fd);
        /* The master would receive the signal of closing zvar_event_server_status_fd */
        zclose(zvar_event_server_status_fd);
    }
    if (event_ios) {
        ZVECTOR_WALK_BEGIN(event_ios, zeio_t *, eio) {
            zeio_free(eio, 1);
        } ZVECTOR_WALK_END;
        zvector_free(event_ios);
        event_ios = 0;
    }
    if (stop_file_timer) {
        zetimer_free(stop_file_timer);
        stop_file_timer = 0;
    }
    if (stop_timer) {
        zetimer_free(stop_timer);
        stop_timer = 0;
    }
    if (zevent_server_before_exit) {
        zevent_server_before_exit();
    }

    zevent_base_free(zvar_default_event_base);
}

int zevent_server_main(int argc, char **argv)
{
    zevent_server_init(argc, argv);
    int loop = 0;
    while (1) {
        if (zvar_proc_stop || flag_stop) {
            break;
        }
        if (!zevent_base_dispatch(zvar_default_event_base)) {
            break;
        }
        if (zvar_proc_stop || flag_stop) {
            break;
        }
        if (loop++ == 10) {
            if (getppid() != parent_pid) {
                break;
            }
            loop = 0;
        }
    }
    zevent_server_fini();
    return 0;
}

void zevent_server_stop_notify(void)
{
    flag_stop = 1;
    zevent_base_notify(zvar_default_event_base);
}

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

void (*zcoroutine_server_service_register) (const char *service, int fd, int fd_type) = 0;
void (*zcoroutine_server_before_service) (void) = 0;
void (*zcoroutine_server_before_reload) (void) = 0;
void (*zcoroutine_server_before_exit) (void) = 0;

static zbool_t flag_run = 0;
static zbool_t flag_reload = 0;
static zbool_t master_mode = 0;
static char *stop_file = 0;
static pid_t parent_pid = 0;

static void *after_monitor_reload_signal(void *arg)
{
    zsleep(100);
    zvar_proc_stop = 1;
    return 0;
}

static void *monitor_reload_signal(void *arg)
{
    while(ztimed_read_wait(zvar_master_server_status_fd, 10) == 0) {
    }

    flag_reload = 1;
    if (zcoroutine_server_before_reload) {
        zcoroutine_go(after_monitor_reload_signal, 0, -1);
        zcoroutine_server_before_reload();
    } else {
        zvar_proc_stop = 1;
    }
    return 0;
}

static void *stop_file_check(void *arg)
{
    while(!zvar_proc_stop) {
        zsleep(1);
        if (zfile_get_size(stop_file) < 0) {
            break;
        }
    }
    zvar_proc_stop = 1;
    return 0;
}

static void *do_exit_after(void *arg)
{
    zsleep((int)(long)(arg));
    zvar_proc_stop = 1;
    return arg;
}

static void *ppid_check(void *arg)
{
    while(1) {
        zsleep(10);
        if(getppid() != parent_pid) {
            break;
        }
    }
    zvar_proc_stop = 1;
    return 0;
}

static void sigterm_handler(int sig)
{
    zvar_proc_stop = 1;
}

static void sighup_handler(int sig)
{
    zvar_proc_stop = 1;
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
        zcoroutine_server_service_register(service, fd, fd_type);
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
        zcoroutine_enable_fd(fd);
        znonblocking(fd, 0);
        zclose_on_exec(fd, 1);
        zcoroutine_server_service_register(service_name, fd, fdtype);
        zargv_free(stfd);
    }
    zargv_free(service_argv);
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
static void zcoroutine_server_init(int argc, char ** argv)
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

    if (!zcoroutine_server_service_register) {
        zfatal("zcoroutine_server_service_register is null");
    }

    char *attr;
    zmain_argument_run(argc, argv, deal_argument);

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

    zcoroutine_base_init();

    if (master_mode) {
        zclose_on_exec(zvar_master_server_status_fd, 1);
        znonblocking(zvar_master_server_status_fd, 1);
        zcoroutine_enable_fd(zvar_master_server_status_fd);
        zcoroutine_go(monitor_reload_signal, 0, -1);
        if (!zempty(stop_file)) {
            zcoroutine_go(stop_file_check, 0, -1);
        }
    }

    zcoroutine_go(ppid_check, 0, -1);

    if (zcoroutine_server_before_service) {
        zcoroutine_server_before_service();
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
        zcoroutine_go(do_exit_after, (void *)ea, -1);
    }
}

int zcoroutine_server_main(int argc, char **argv)
{
    zcoroutine_server_init(argc, argv);
    zcoroutine_base_run();
    zcoroutine_base_fini();
    if (zcoroutine_server_before_exit) {
        zcoroutine_server_before_exit();
    }
    return 0;
}

void zcoroutine_server_stop_notify(void)
{
    zvar_proc_stop = 1;
    zcoroutine_base_stop_notify();
    zevent_base_notify(zvar_default_event_base);
}


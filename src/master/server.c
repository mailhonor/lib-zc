/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-11-20
 * ================================
 */


#include "libzc.h"
#include <signal.h>
#include <pthread.h>

int zvar_master_server_listen_fd;
int zvar_master_server_listen_type;
zmaster_server_service_t zmaster_server_service = 0;
zmaster_server_cb_t zmaster_server_before_service = 0;
zmaster_server_cb_t zmaster_server_reload = 0;
zmaster_server_cb_t zmaster_server_loop = 0;
zmaster_server_cb_t zmaster_server_before_exit = 0;

static int test_mode;
static int softstopping;
static int reloading;
static int client_count;
static pid_t parent_pid;
static zev_t *ev_status;
static zev_t *ev_listen;
static pthread_mutex_t *server_mutex;

static int on_master_reload(zev_t * zev)
{
    reloading = 1;
    if (zmaster_server_reload)
    {
        zmaster_server_reload();
    }
    return 0;
}

#define client_count_plus()     client_recount(1)
#define client_count_minus()    client_recount(-1)
static void client_recount(int pm)
{
    if (pthread_mutex_lock(server_mutex))
    {
        zfatal("mutex: %m");
    }
    client_count += pm;
    if (pthread_mutex_unlock(server_mutex))
    {
        zfatal("mutex: %m");
    }
}

#define local_ev_close()    if(local_ev_close_do_once){local_ev_close_do_once=0;local_ev_close_do();}
static int local_ev_close_do_once;
static void local_ev_close_do()
{
    if (ev_listen)
    {
        zev_stop(ev_listen);
        zev_fini(ev_listen);
        zfree(ev_listen);
        ev_listen = 0;
        close(zvar_master_server_listen_fd);
        zvar_master_server_listen_fd = -1;
    }

    if (ev_status)
    {
        /* The master would receive the signal of closing ZMASTER_SERVER_STATUS_FD. */
        zev_stop(ev_status);
        zev_fini(ev_status);
        zfree(ev_status);
        ev_status = 0;
        close(ZMASTER_SERVER_STATUS_FD);
        close(ZMASTER_MASTER_STATUS_FD);
    }
}
static int inet_server_accept(zev_t * zev)
{
    int fd;
    int listen_fd;

    listen_fd = zev_get_fd(zev);

    fd = zinet_accept(listen_fd);
    if (fd < 0)
    {
        if (errno != EAGAIN)
        {
            zfatal("inet_server_accept: %m");
        }
        return -1;
    }
    znonblocking(fd, 1);

    client_count_plus();
    zmaster_server_service(fd);

    return 0;
}

static int unix_server_accept(zev_t * zev)
{
    int fd;
    int listen_fd;

    listen_fd = zev_get_fd(zev);

    fd = zunix_accept(listen_fd);
    if (fd < 0)
    {
        if (errno != EAGAIN)
        {
            zfatal("unix_server_accept: %m");
        }
        return -1;
    }
    znonblocking(fd, 1);

    client_count++;
    zmaster_server_service(fd);

    return 0;
}

void register_server(char *test_listen)
{
    if (!test_mode)
    {
        ev_status = (zev_t *)zcalloc(1, sizeof(zev_t));
        zclose_on_exec(ZMASTER_MASTER_STATUS_FD, 1);
        znonblocking(ZMASTER_MASTER_STATUS_FD, 1);

        zev_init(ev_status, zvar_evbase, ZMASTER_MASTER_STATUS_FD);
        zev_read(ev_status, on_master_reload);

        zvar_master_server_listen_fd = ZMASTER_LISTEN_FD;
    }
    else
    {
        if (test_listen == 0)
        {
            test_listen = zconfig_get_str(zvar_config, "zlisten", 0);
        }
        if (ZEMPTY(test_listen))
        {
            zfatal("no listen");
        }
        if (strchr(test_listen, ':'))
        {
            zvar_master_server_listen_type = ZMASTER_LISTEN_INET;
        }
        else
        {
            zvar_master_server_listen_type = ZMASTER_LISTEN_UNIX;
        }
        zvar_master_server_listen_fd = zlisten(test_listen, 10);

        if (zvar_master_server_listen_fd < 0)
        {
            zfatal("open %s(%m)", test_listen);
        }
    }
    zclose_on_exec(zvar_master_server_listen_fd, 1);

    znonblocking(zvar_master_server_listen_fd, 1);

    if (!zmaster_server_service)
    {
        return;
    }

    ev_listen = (zev_t *) zcalloc(1, sizeof(zev_t));
    zev_init(ev_listen, zvar_evbase, zvar_master_server_listen_fd);

    if (zvar_master_server_listen_type == ZMASTER_LISTEN_INET)
    {
        zev_read(ev_listen, inet_server_accept);
    }
    else if (zvar_master_server_listen_type == ZMASTER_LISTEN_INET)
    {
        zev_read(ev_listen, unix_server_accept);
    }
}

int zmaster_server_main(int argc, char **argv)
{
    int op;
    char *test_listen = 0;

    parent_pid = getppid();
    test_mode = 1;
    client_count = 0;
    reloading = 0;
    softstopping = 0;
    ev_status = 0;
    ev_listen = 0;
    local_ev_close_do_once = 1;
    server_mutex = (pthread_mutex_t *)zcalloc(1, sizeof(pthread_mutex_t));
    pthread_mutex_init(server_mutex, 0);
    signal(SIGPIPE, SIG_IGN);

    if (!zvar_progname)
    {
        zvar_progname = argv[0];
    }

    zvar_config_init();
    zvar_evbase_init();

    while ((op = getopt(argc, argv, "Ml:c:o:t:dv")) > 0)
    {
        switch (op)
        {
        case 'M':
            test_mode = 0;
            break;
        case 'l':
            test_listen = optarg;
            break;
        case 'c':
            zconfig_load(zvar_config, optarg);
            break;
        case 'o':
            {
                char *key, *value;
                key = zstrdup(optarg);
                value = strchr(key, '=');
                if (value)
                {
                    *value++ = 0;
                }
                zconfig_add(zvar_config, key, value);
                zfree(key);
            }
            break;
        case 't':
            zvar_master_server_listen_type = optarg[0];
            break;
        case 'd':
            zlog_set_level_from_console(ZLOG_DEBUG);
            break;
        case 'v':
            zlog_set_level_from_console(ZLOG_VERBOSE);
            break;
        default:
            zfatal("parameters error");
        }
    }

    register_server(test_listen);

    if (zmaster_server_before_service)
    {
        zmaster_server_before_service();
    }

    while (1)
    {
        zevbase_dispatch(zvar_evbase, 0);
        if (zmaster_server_loop)
        {
            zmaster_server_loop();
        }
        if (reloading)
        {
            local_ev_close();
            if(client_count < 1)
            {
                break;
            }
            if (getppid() != parent_pid)
            {
                break;
            }
        }
        if (softstopping)
        {
            local_ev_close();
            break;
        }
    }

    local_ev_close();

    if (zmaster_server_before_exit)
    {
        zmaster_server_before_exit();
    }

    zevbase_free(zvar_evbase);

    return 0;
}

void zmaster_server_stop_notify(void)
{
    softstopping = 1;
    zevbase_notify(zvar_evbase);
}

void zmaster_server_disconnect(int fd)
{
    client_count_minus();
    close(fd);
}

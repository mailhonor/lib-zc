/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-11-10
 * ================================
 */


#include "libzc.h"
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>

/* FIXME FREE entry */

#define zmaster_entry_info(m) zverbose("master entry: %s, limit:%d,count:%d, line:%d", \
        m->cmd, m->proc_limit, m->proc_count, __LINE__)

typedef struct zmaster_entry_t zmaster_entry_t;
typedef struct zmaster_status_fd_t zmaster_status_fd_t;
struct zmaster_entry_t
{
    int stop;
    char *config_path;
    char *cmd;
    int proc_limit;
    int proc_count;
    int wakeup;
    int wakeup_on;
    int wakeup_now;
    ztimer_t wakeup_timer;
    int listen_fd;
    int listen_type;
    int listen_on;
    zev_t listen_ev;
    int child_error;
    ztimer_t child_error_timer;
};

struct zmaster_status_fd_t
{
    long stamp;
    zmaster_entry_t *men;
    zev_t *ev;
};

int zvar_master_child_exception_check = 0;
static int try_lock;
static char *config_dir;
static char *lock_file;

static int self_init;
static int sighup_on;
static int lock_fd;
static int master_status_fd[2];
static zgrid_t *server_entry_list;
static zigrid_t *status_fd_list;

static void zmaster_listen_set(zmaster_entry_t * men);
static int zmaster_start_child(zev_t *zev);

static void zmaster_sighup_handler(int sig)
{
    zdebug("now reload");
    sighup_on = sig;
}

static void zmaster_server_init(void)
{
    if (self_init)
    {
        return;
    }
    self_init = 1;

    /* EVENT */
    if (!zvar_evbase)
    {
        zvar_evbase = zevbase_create();
    }

    /* VAR */
    server_entry_list = zgrid_create();
    status_fd_list = zigrid_create();

    /* SIG */
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);

    /* SIG RELOAD */
    struct sigaction sig;
    sigemptyset(&sig.sa_mask);
    sig.sa_flags = 0;
    sig.sa_handler = zmaster_sighup_handler;
    if (sigaction(SIGHUP, &sig, (struct sigaction *)0) < 0)
    {
        zfatal("%s: sigaction(%d) : %m", __FUNCTION__, SIGHUP);
    }

    /* MASTER STATUS */
    if (pipe(master_status_fd) == -1)
    {
        zfatal("zmaster: pipe : %m");
    }
    zclose_on_exec(master_status_fd[0], 1);
}

static void zmaster_remove_old_child(void)
{
    zigrid_node_t *idn;
    int status_fd;
    zev_t *ev;
    int fd;

    while((idn = zigrid_first(status_fd_list)))
    {
        status_fd = zigrid_key(idn);
        ev = (zev_t *)((char *)(zigrid_value(idn)) + sizeof(zmaster_status_fd_t));
        fd = zev_get_fd(ev);
        zev_fini(ev);
        close(fd);
        zfree(zigrid_value(idn));
        zigrid_delete_node(status_fd_list, idn);
        close(status_fd);
    }

    close(master_status_fd[0]);
    close(master_status_fd[1]);
    if (pipe(master_status_fd) == -1)
    {
        zfatal("zmaster: pipe : %m");
    }
    zclose_on_exec(master_status_fd[0], 1);
}

static void zmaster_set_stop(void)
{
    zgrid_node_t *n;
    zmaster_entry_t *men;

    ZGRID_WALK_BEGIN(server_entry_list, n)
    {
        men = (zmaster_entry_t *)(zgrid_value(n));
        men->stop = 1;
        zfree(men->config_path);
        zfree(men->cmd);
        ztimer_fini(&(men->wakeup_timer));
        zev_fini(&(men->listen_ev));
        men->child_error = 0;
        ztimer_fini(&(men->child_error_timer));
    }
    ZGRID_WALK_END;
}

static void zmaster_reload_one_config(char *pn)
{
    zconfig_t *cf;
    char *cmd, *listen;
    zmaster_entry_t *men;

    cf = zconfig_create();
    zconfig_load(cf, pn);

    cmd = zconfig_get_str(cf, "zcmd", "");
    if (!*cmd)
    {
        zconfig_free(cf);
        return;
    }

    listen = zconfig_get_str(cf, "zlisten", "");
    if (!*listen)
    {
        zconfig_free(cf);
        return;
    }

    if (!zgrid_lookup(server_entry_list, listen, (char **)&men))
    {
        men = zcalloc(1, sizeof(zmaster_entry_t));
        men->listen_fd = -1;
        zgrid_add(server_entry_list, listen, men, 0);
    }
    men->stop = 0;
    men->config_path = zstrdup(pn);
    men->cmd = zstrdup(cmd);
    men->proc_limit = zconfig_get_int(cf, "zproc_limit", 0);
    if (men->proc_limit < 1)
    {
        men->proc_limit = 1;
    }
    men->proc_count = 0;
    men->wakeup = zconfig_get_int(cf, "zwakeup", 0);
    if (men->wakeup < 0)
    {
        men->wakeup = 0;
    }
    ztimer_init(&(men->wakeup_timer), zvar_evbase);
    ztimer_set_context(&(men->wakeup_timer), men);
    men->child_error = 0;
    ztimer_init(&(men->child_error_timer), zvar_evbase);
    ztimer_set_context(&(men->child_error_timer), men);

    zconfig_free(cf);
}

static void zmaster_reload_config(void)
{
    DIR *dir;
    struct dirent ent, *ent_list;
    char pn[4096], *fn, *p;

    dir = opendir(config_dir);
    if (!dir)
    {
        zfatal("open %s(%m)", config_dir);
    }

    while ((!readdir_r(dir, &ent, &ent_list)) && (ent_list))
    {
        fn = ent.d_name;
        if (fn[0] == '.')
        {
            continue;
        }
        p = strrchr(fn, '.');
        if ((!p) || (strcasecmp(p+1, "cf")))
        {
            continue;
        }
        snprintf(pn, 4000, "%s/%s", config_dir, fn);
        zmaster_reload_one_config(pn);
    }
    closedir(dir);
}

static void zmaster_release_unused_entry(void)
{
    zarray_t *list;
    zgrid_node_t *n;
    zmaster_entry_t *men;

    list = zarray_create(1024);
    ZGRID_WALK_BEGIN(server_entry_list, n)
    {
        men = (zmaster_entry_t *)(zgrid_value(n));
        if (men->stop)
        {
            zarray_add(list, n);
        }
    }
    ZGRID_WALK_END;

    ZARRAY_WALK_BEGIN(list, n)
    {
        men = (zmaster_entry_t *)(zgrid_value(n));
        if (men->listen_fd!= -1)
        {
            close(men->listen_fd);
        }
        zfree(men);
        zgrid_delete_node(server_entry_list, n);
    }
    ZARRAY_WALK_END;

    zarray_free(list, 0, 0);
}

static void zmaster_active_one_server(char *name, zmaster_entry_t *men)
{
    if (men->listen_fd == -1)
    {
        int limit = men->proc_limit;
        limit = ((limit>100)?100:limit);
        limit = ((limit<5)?5:limit);
        men->listen_fd = zlisten(name, limit);
        if (men->listen_fd == -1)
        {
            printf("open %s(%m)", name);
            zfatal("open %s(%m)", name);
        }

        men->listen_type = ZMASTER_LISTEN_UNIX;
        if (strchr(name, ':'))
        {
            men->listen_type = ZMASTER_LISTEN_INET;
        }
    }
    zev_init(&(men->listen_ev), zvar_evbase, men->listen_fd);
    zev_set_context(&(men->listen_ev), men);
    zmaster_listen_set(men);
}

static void zmaster_active_server(void)
{
    zgrid_node_t *n;
    zmaster_entry_t *men;
    char *name;

    ZGRID_WALK_BEGIN(server_entry_list, n)
    {
        men = (zmaster_entry_t *)(zgrid_value(n));
        name = zgrid_key(n);
        zmaster_active_one_server(name, men);
    }
    ZGRID_WALK_END;
}

static int zmaster_wakeup_idle(ztimer_t * tm)
{
    zmaster_entry_t *men;

    men = (zmaster_entry_t *) (ztimer_get_context(tm));
    ztimer_stop(&(men->child_error_timer));
    men->child_error = 0;
    zmaster_listen_set(men);

    return 0;
}

static int zmaster_wakeup_child(ztimer_t * tm)
{
    zmaster_entry_t *men;

    men = (zmaster_entry_t *) (ztimer_get_context(tm));
    if (men->proc_count)
    {
        return 0;
    }
    men->wakeup_now = 1;
    zmaster_start_child(&(men->listen_ev));

    return 0;
}

static void zmaster_listen_set(zmaster_entry_t * men)
{
    zmaster_entry_info(men);
    if (men->child_error || (men->proc_count >= men->proc_limit))
    {
        if (men->listen_on == 1)
        {
            zev_stop(&(men->listen_ev));
            men->listen_on = 0;
        }
        if (men->child_error)
        {
            ztimer_start(&(men->child_error_timer), zmaster_wakeup_idle, 100);
        }
    }
    else if ((men->listen_on == 0) && (men->proc_count < men->proc_limit))
    {
        zev_read(&(men->listen_ev), zmaster_start_child);
        men->listen_on = 1;
    }

    if (men->wakeup > 0)
    {
        if (men->proc_count > 0)
        {
            if (men->wakeup_on)
            {
                ztimer_stop(&(men->wakeup_timer));
                men->wakeup_on = 0;
            }
        }
        else
        {
            if (!men->wakeup_on)
            {
                ztimer_start(&(men->wakeup_timer), zmaster_wakeup_child, men->wakeup * 1000);
                men->wakeup_on = 1;
            }
        }
    }
    zmaster_entry_info(men);
}

static int zmaster_child_strike(zev_t * zev)
{
    zmaster_status_fd_t *fd_info;
    zmaster_entry_t *men;
    int fd;

    fd = zev_get_fd(zev);
    fd_info = (zmaster_status_fd_t *)zev_get_context(zev);
    men = fd_info->men;

    if (ztimeout_set(0) - fd_info->stamp < 100)
    {
        if (zvar_master_child_exception_check)
        {
            men->child_error = 1;
        }
    }

    men->proc_count -- ;
    zmaster_listen_set(men);
    zmaster_entry_info(men);

    zev_fini(zev);
    zfree(fd_info);
    zigrid_delete(status_fd_list, fd, 0);
    close(fd);

    return 0;
}

static int zmaster_start_child(zev_t * zev)
{
    int pid;
    zmaster_entry_t *men;
    int status_fd[2];

    men = (zmaster_entry_t *) (zev_get_context(zev));

    if (men->wakeup_now == 0)
    {
        if (ZEV_EXCEPTION & zev_get_events(zev))
        {
            return 0;
        }
    }
    men->wakeup_now = 0;

    if (pipe(status_fd) == -1)
    {
        zfatal("zmaster: pipe : %m");
    }

    pid = fork();
    if (pid == -1)
    {
        close(status_fd[0]);
        close(status_fd[1]);
        return 0;
    }
    else if (pid)
    {
        zmaster_status_fd_t *fd_info;
        zev_t *ev;
        men->proc_count++;
        zmaster_listen_set(men);
        zmaster_entry_info(men);
        fd_info = (zmaster_status_fd_t *) zcalloc(1, sizeof(zmaster_status_fd_t) + sizeof(zev_t));
        fd_info->stamp = ztimeout_set(0);
        fd_info->men = men;
        close(status_fd[0]);
        zigrid_add(status_fd_list, status_fd[1], fd_info, 0);
        ev = (zev_t *)((char *)fd_info + sizeof(zmaster_status_fd_t));
        zev_init(ev, zvar_evbase, status_fd[1]);
        zev_set_context(ev, fd_info);
        zev_set(ev, ZEV_READ, zmaster_child_strike);
        return 0;
    }
    else
    {
        zargv_t *exec_argv;
        char buf[128];

        close(status_fd[1]);
        dup2(status_fd[0], ZMASTER_SERVER_STATUS_FD);
        close(status_fd[0]);

        close(master_status_fd[0]);
        dup2(master_status_fd[1], ZMASTER_MASTER_STATUS_FD);
        close(master_status_fd[1]);

        dup2(men->listen_fd, ZMASTER_LISTEN_FD);
        close(men->listen_fd);

        exec_argv = zargv_create(1);

        zargv_add(exec_argv, men->cmd);
        zargv_add(exec_argv, "-M");

        zargv_add(exec_argv, "-c");
        zargv_add(exec_argv, men->config_path);

        zargv_add(exec_argv, "-t");
        sprintf(buf, "%c", men->listen_type);
        zargv_add(exec_argv, buf);

        execvp(men->cmd, (char **)(zmemdup(ZARGV_DATA(exec_argv), (ZARGV_LEN(exec_argv) + 1) * sizeof(char *))));

        zfatal("zmaster_start_child error: %m");
    }

    return 0;
}

static void zmaster_reload_service(void)
{
    zmaster_remove_old_child();
    zmaster_set_stop();
    zmaster_reload_config();
    zmaster_release_unused_entry();
    zmaster_active_server();
}

static int zmaster_lock(void)
{
    char pid_buf[33];

    lock_fd = open(lock_file, O_RDWR|O_CREAT, 0666);
    if (lock_fd < 0)
    {
        return 0;
    }
    zclose_on_exec(lock_fd, 1);

    if (flock(lock_fd, LOCK_EX | LOCK_NB))
    {
        close(lock_fd);
        return 0;
    }
    else
    {
        sprintf(pid_buf, "%d          ", getpid());
        if (write(lock_fd, pid_buf, 10) != 10)
        {
            close(lock_fd);
            return 0;
        }
        return 1;
    }

    return 0;
}

int zmaster_main(int argc, char **argv)
{
    int op;

    try_lock = 0;
    config_dir = "config/";
    lock_file = "master.pid";

    while ((op = getopt(argc, argv, "c:p:tdv")) > 0)
    {
        switch (op)
        {
        case 'c':
            config_dir = optarg;
            break;
        case 't':
            try_lock = 1;
            break;
        case 'p':
            lock_file = optarg;
            break;
        case 'd':
            zlog_set_level_from_console(ZLOG_DEBUG);
            break;
        case 'v':
            zlog_set_level_from_console(ZLOG_VERBOSE);
            break;
        default:
            zfatal("args error");
        }
    }

    if (try_lock)
    {
        if (zmaster_lock())
        {
            exit(0);
        }
        else
        {
            exit(1);
        }
    }

    if (!zmaster_lock())
    {
        exit(1);
    }

    zmaster_server_init();
    zmaster_reload_service();

    sighup_on = 0;
    while (1)
    {
        zevbase_dispatch(zvar_evbase, 0);
        if (sighup_on)
        {
            zdebug("zmaster_reload");
            sighup_on = 0;
            zmaster_reload_service();
        }
    }

    return 0;
}


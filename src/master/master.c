/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-11-10
 * ================================
 */

#include "zc.h"
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>


zevbase_t *zvar_master_evbase = 0;

#define zdebug(fmt, args...) {if(debug_mode){zinfo(fmt, ##args);}}

typedef struct server_entry_t server_entry_t;
typedef struct listen_pair_t listen_pair_t;
typedef struct child_status_t child_status_t;

struct server_entry_t {
    char *config_fn;
    char *cmd;
    char *module;
    int proc_limit;
    int proc_count;
    zidict_t *listens;
    unsigned char listen_on;
    unsigned char wait_on;
    zevtimer_t wait_timer;
};

struct listen_pair_t {
    char *service_name;
    int fd;
    unsigned char iuf;
    unsigned char used;
    zev_t listen_ev;
    server_entry_t *server;
};

struct child_status_t {
    server_entry_t *server;
    int fd;
    zev_t status_ev;
    long stamp;
};

static int debug_mode = 0;
static int sighup_on = 0;
static char *config_path = 0;

static int master_status_fd[2];
static zdict_t *listen_pair_dict;
static zidict_t *child_status_dict;
static zvector_t *server_vector;


static void disable_server_listen(server_entry_t *server)
{
    zdebug("zmaster: disable_server_listen server:%s, listen_one:%d", server->cmd, server->listen_on);
    if (!server->listen_on) {
        return;
    }
    server->listen_on = 0;

    zidict_node_t *n;
    listen_pair_t *lp;
    ZIDICT_WALK_BEGIN(server->listens, n) {
        lp = (listen_pair_t *)(ZDICT_VALUE(n));
        zev_unset(&(lp->listen_ev));
    } ZIDICT_WALK_END;
}

static void start_one_child(zev_t *ev);
static void enable_server_listen(server_entry_t *server)
{
    zdebug("zmaster: enable_server_listen server:%s, listen_one:%d", server->cmd, server->listen_on);
    if (server->listen_on) {
        return;
    }
    server->listen_on = 1;

    zidict_node_t *n;
    listen_pair_t *lp;
    ZIDICT_WALK_BEGIN(server->listens, n) {
        lp = (listen_pair_t *)(ZDICT_VALUE(n));
        zev_set_context(&(lp->listen_ev), lp);
        zev_read(&(lp->listen_ev), start_one_child);
    } ZIDICT_WALK_END;
}

static void child_exception_over(zevtimer_t *tm)
{
    zdebug("zmaster: child_exception_over");
    server_entry_t *server = (server_entry_t *)zevtimer_get_context(tm);
    enable_server_listen(server);
}

static void child_strike(zev_t *ev)
{
    child_status_t *cs = (child_status_t *)zev_get_context(ev);
    server_entry_t *server = cs->server;
    zdebug("zmaster: child_strike, cmd:%s", server->cmd);
    if (ztimeout_set(0) - cs->stamp < 100) {
        if (!server->wait_on) {
            zdebug("zmaster: child_strike, cmd:%s, exception", server->cmd);
            zevtimer_set_context(&(server->wait_timer), server);
            zevtimer_start(&(server->wait_timer), child_exception_over, 100);
            server->wait_on = 1;
        }
    }

    zev_fini(ev);
    zfree(cs);
    server->proc_count--;
    if ((server->proc_count  < server->proc_limit) && (server->wait_on==0)) {
        enable_server_listen(server);
    }
}

static void start_one_child(zev_t *ev)
{
    listen_pair_t *lp = (listen_pair_t *)zev_get_context(ev);
    server_entry_t *server = lp->server;
    zdebug("zmaster: start_one_child, cmd:%s", server->cmd);

    if (zev_exception(ev)) {
        zmsleep(100);
        return;
    }
    printf("AAA\n");
    if (server->wait_on) {
        disable_server_listen(server);
        return;
    }
    printf("BBB\n");
    if (server->proc_count >= server->proc_limit) {
        disable_server_listen(server);
        return;
    }
    printf("ccc\n");

    int status_fd[2];
    if (pipe(status_fd) == -1) {
        zfatal("zmaster: pipe (%m)");
    }
    pid_t pid = fork();
    if (pid == -1) {
        close(status_fd[0]);
        close(status_fd[1]);
        zmsleep(100);
        return;
    } else if (pid) {
        server->proc_count++;
        if (server->proc_count >= server->proc_limit) {
            disable_server_listen(server);
        }
        close(status_fd[0]);
        child_status_t *cs = (child_status_t *)zcalloc(1, sizeof(child_status_t));
        zidict_update(child_status_dict, status_fd[1], cs, 0);
        cs->server = server;
        cs->fd = status_fd[1];
        zev_init(&(cs->status_ev), zvar_master_evbase, status_fd[1]);
        zev_set_context(&(cs->status_ev), cs);
        zev_read(&(cs->status_ev), child_strike);
        cs->stamp = ztimeout_set(0);
        return;
    } else {
        printf("GGGGGGG\n");
        zargv_t *exec_argv = zargv_create(128);
        char buf[4100];

        close(status_fd[1]);
        dup2(status_fd[0], ZMASTER_SERVER_STATUS_FD);
        close(status_fd[0]);

        close(master_status_fd[0]);
        dup2(master_status_fd[1], ZMASTER_MASTER_STATUS_FD);
        close(master_status_fd[1]);

        zargv_add(exec_argv, server->cmd);
        zargv_add(exec_argv, "-M");

        if (config_path) {
            snprintf(buf, 4096, "%s/main.cf", config_path);
            if (zfile_get_size(buf) > -1) {
                zargv_add(exec_argv, "--c");
                zargv_add(exec_argv, buf);
            }
            if (server->config_fn) {
                zargv_add(exec_argv, "--c");
                snprintf(buf, 4096, "%s/%s", config_path, server->config_fn);
                zargv_add(exec_argv, buf);
            }
        }
        if (!zempty(server->module)) {
            zargv_add(exec_argv, "--module");
            zargv_add(exec_argv, server->module);
        }
        zidict_node_t *ind;
        int fdnext = ZMASTER_LISTEN_FD;
        ZIDICT_WALK_BEGIN(server->listens, ind) {
            char iuffd[111];
            listen_pair_t *lp2 = (listen_pair_t *)ZDICT_VALUE(ind);
            dup2(lp2->fd, fdnext);
            close(lp2->fd);
            snprintf(iuffd, 100, "-s%s", lp->service_name);
            zargv_add(exec_argv, iuffd);
            sprintf(iuffd, "%c%d", lp2->iuf, fdnext);
            zargv_add(exec_argv, iuffd);
            fdnext++;

        } ZDICT_WALK_END;

        printf("HHHHHHHHHHHHHHHH\n");
        execvp(server->cmd, (char **)(zmemdup(ZARGV_DATA(exec_argv), (ZARGV_LEN(exec_argv) + 1) * sizeof(char *))));
        printf("JJJJJJJJJJJJJJJ\n");

        zfatal("zmaster: start child error: %m");
    }
}

static void remove_old_child(void)
{
    child_status_t *cs;
    zidict_node_t *n;
    while((n=zidict_first(child_status_dict))) {
        cs = (child_status_t *)(ZIDICT_VALUE(n));
        zev_fini(&(cs->status_ev));
        close(cs->fd);
        zfree(cs);
        zidict_erase_node(child_status_dict, n);
    }
}

static void remove_server_entry(void)
{
    server_entry_t *sv;

    ZVECTOR_WALK_BEGIN(server_vector, sv) {
        zfree(sv->config_fn);
        zfree(sv->cmd);
        zfree(sv->module);
        zidict_free(sv->listens);
        if (sv->wait_on) {
            sv->wait_on = 0;
            zevtimer_stop(&(sv->wait_timer));
        }
        zfree(sv);
    } ZVECTOR_WALK_END;

    zvector_free(server_vector);
    server_vector = zvector_create(13);
}

static void set_listen_unused(void)
{
    listen_pair_t *lp;
    zdict_node_t *n;

    ZDICT_WALK_BEGIN(listen_pair_dict, n) {
        lp = (listen_pair_t *)(ZDICT_VALUE(n));
        lp->used = 0;
        zev_fini(&(lp->listen_ev));
        lp->server = 0;
    } ZDICT_WALK_END;
}

static void prepare_server_by_config(zconfig_t *cf)
{
    char *cmd, *listen, *fn, *module;
    server_entry_t *server;
    cmd = zconfig_get_str(cf, "zcmd", "");
    listen = zconfig_get_str(cf, "zlisten", "");
    if (zempty(cmd) || zempty(listen)) {
        return;
    }
    fn = zconfig_get_str(cf, "z___Z_0428_fn", 0);
    module = zconfig_get_str(cf, "zmodule", 0);
    server = zcalloc(1, sizeof(server_entry_t));
    zvector_add(server_vector, server);
    server->listen_on = 1;
    zevtimer_init(&(server->wait_timer), zvar_master_evbase);
    server->config_fn = (fn ? zstrdup(fn) : 0);
    server->cmd = zstrdup(cmd);
    server->module = zstrdup(module);
    server->proc_limit = zconfig_get_int(cf, "zproc_limit", 1, 1, 1000);
    server->proc_count = 0;
    server->listens = zidict_create();

    char *service, lbuf[1024];
    listen_pair_t *lp;
    zargv_t *listens = zargv_create(8);
    zargv_split_append(listens, listen, ";, \t");
    ZARGV_WALK_BEGIN(listens, service) {
        snprintf(lbuf, 1000, "%s", service);
        service = lbuf;
        listen = strstr(service, "://");
        if (!listen) {
            listen = service;
            service = zblank_buffer;
        } else {
            *listen = 0;
            listen += 3;
        }
        if (!zdict_find(listen_pair_dict, listen, (char **)&lp)) {
            lp = (listen_pair_t *)zcalloc(1, sizeof(listen_pair_t));
            zdict_update(listen_pair_dict, listen, lp, 0);
            lp->fd = zlisten(listen, 5, (int *)&(lp->iuf));
            if (lp->fd < 0) {
                zfatal("zmaster: open %s error\n");
            }
            zclose_on_exec(lp->fd, 1);
        }
        if (lp->used) {
            zfatal("zmaster: open %s twice", listen);
        }
        lp->used = 1;
        printf("SSS start\n");
        zfree(lp->service_name);
        lp->service_name = zstrdup(service);
        lp->server = server;
        zev_init(&(lp->listen_ev), zvar_master_evbase, lp->fd);
        zev_set_context(&(lp->listen_ev), lp);
        zev_read(&(lp->listen_ev), start_one_child);

        zidict_update(server->listens, lp->fd, lp, 0);
    } ZARGV_WALK_END;
    zargv_free(listens);
}

void zmaster_load_server_config_by_dir(const char *config_path, zvector_t * cfs)
{
    DIR *dir;
    struct dirent ent, *ent_list;
    char *fn, *p;
    char pn[4100];
    zconfig_t *cf;

    dir = opendir(config_path);
    if (!dir) {
        zfatal("zmaster: open %s/(%m)", config_path);
    }

    while ((!readdir_r(dir, &ent, &ent_list)) && (ent_list)) {
        fn = ent.d_name;
        if (fn[0] == '.') {
            continue;
        }
        p = strrchr(fn, '.');
        if ((!p) || (strcasecmp(p + 1, "cf"))) {
            continue;
        }
        if (!strcasecmp(fn, "main.cf")) {
            continue;
        }

        cf = zconfig_create();
        snprintf(pn, 4096, "%s/%s", config_path, fn);
        zconfig_load(cf, pn);
        zconfig_update(cf, "z___Z_0428_fn", fn);
        zvector_add(cfs, cf);
    }
    closedir(dir);
}

static void reload_config(void)
{
    zconfig_t *cf;
    zvector_t *cfs = zvector_create(128);

    if (zmaster_load_server_config_fn) {
        zmaster_load_server_config_fn(cfs);
    } else {
        zmaster_load_server_config_by_dir(config_path, cfs);
    }

    ZVECTOR_WALK_BEGIN(cfs, cf) {
        prepare_server_by_config(cf);
        zconfig_free(cf);
    } ZVECTOR_WALK_END;

    zvector_free(cfs);
}

static void release_unused_listen(void)
{
    zvector_t *vec = zvector_create(128);
    listen_pair_t *lp;
    zdict_node_t *n;

    ZDICT_WALK_BEGIN(listen_pair_dict, n) {
        lp = (listen_pair_t *)(ZDICT_VALUE(n));
        if (lp->used) {
            continue;
        }
        zvector_add(vec, n);
    } ZDICT_WALK_END;

    ZVECTOR_WALK_BEGIN(vec, n) {
        lp = (listen_pair_t *)(ZDICT_VALUE(n));
        close(lp->fd);
        zfree(lp->service_name);
        zfree(lp);
        zdict_erase_node(listen_pair_dict, n);
    }ZVECTOR_WALK_END;
    zvector_free(vec);
}

static void reload_server(void)
{
    remove_old_child();
    remove_server_entry();
    set_listen_unused();
    reload_config();
    release_unused_listen();
}

static int zmaster_lock_pfile(char *lock_file)
{
    int lock_fd;
    char pid_buf[33];

    lock_fd = open(lock_file, O_RDWR | O_CREAT, 0666);
    if (lock_fd < 0) {
        return 0;
    }
    zclose_on_exec(lock_fd, 1);

    if (flock(lock_fd, LOCK_EX | LOCK_NB)) {
        close(lock_fd);
        return 0;
    } else {
        sprintf(pid_buf, "%d          ", getpid());
        if (write(lock_fd, pid_buf, 10) != 10) {
            close(lock_fd);
            return 0;
        }
        return 1;
    }

    return 0;
}

static void sighup_handler(int sig)
{
    sighup_on = sig;
}

static void ___usage(char *parameter)
{
    exit(1);
}

static void init_all(int argc, char **argv)
{
    int len;
    char *lock_file = 0;
    int try_lock = 0;

    ZPARAMETER_BEGIN() {
        if (!strcmp(optname, "-t")) {
            try_lock = 1;
            opti+=1;
            continue;
        }
        if (!strcmp(optname, "-d")) {
            debug_mode = 1;
            opti+=1;
            continue;
        }
        if (!optval) {
            ___usage(0);
        }
        if (!strcmp(optname, "-c")) {
            config_path = optval;
            opti+=2;
            continue;
        }
        if (!strcmp(optname, "-p")) {
            lock_file = optval;
            opti+=2;
            continue;
        }
    } ZPARAMETER_END;

    if (zempty(lock_file)) {
        lock_file = "master.pid";
    }
    if (try_lock) {
        if (zmaster_lock_pfile(lock_file)) {
            exit(0);
        } else {
            exit(1);
        }
    }

    if (!zmaster_lock_pfile(lock_file)) {
        exit(1);
    }

    if (config_path) {
        config_path = zstrdup(config_path);
        len = strlen(config_path);
        if (len < 1) {
            zfatal("zmaster: config dir is blank");
        }
        if (config_path[len - 1] == '/') {
            config_path[len - 1] = 0;
        }
    } else if (zmaster_load_server_config_fn == 0) {
        zfatal("zmaster: need service config path");
    }

    /* SIG */
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);

    /* SIG RELOAD */
    struct sigaction sig;
    sigemptyset(&sig.sa_mask);
    sig.sa_flags = 0;
    sig.sa_handler = sighup_handler;
    if (sigaction(SIGHUP, &sig, (struct sigaction *)0) < 0) {
        zfatal("%s: sigaction(%d) : %m", __FUNCTION__, SIGHUP);
    }

    /* MASTER STATUS */
    if (pipe(master_status_fd) == -1) {
        zfatal("zmaster: pipe : %m");
    }
    zclose_on_exec(master_status_fd[0], 1);


    /* VAR */
    listen_pair_dict = zdict_create();
    child_status_dict = zidict_create();
    server_vector = zvector_create(128);
}

static void fini_all(void)
{
    remove_old_child();
    zidict_free(child_status_dict);

    remove_server_entry();
    zvector_free(server_vector);

    set_listen_unused();
    release_unused_listen();
    zdict_free(listen_pair_dict);

    zevbase_free(zvar_master_evbase);
    zconfig_free(zvar_default_config);
    zvar_default_config = 0;

    zfree(config_path);
}

int zmaster_main(int argc, char **argv)
{
    zvar_fatal_catch = 1;
    init_all(argc, argv);
    reload_server();
    sighup_on = 0;
    long t1 = ztimeout_set(0);
    while (1) {
        zevbase_dispatch(zvar_master_evbase, 1 * 1000);
        if (sighup_on) {
            zdebug("zmaster_reload");
            sighup_on = 0;
            reload_server();
        }
        if (ztimeout_set(0) - t1 > 10 * 1000) {
            break;
        }
    }
    fini_all();

    return 0;
}

/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2015-11-10
 * ================================
 */

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include "zc.h"
#include "master.h"
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <time.h>

#define mydebug(fmt,args...) { if(_log_debug_enable){zinfo(fmt, ##args);} }

static zbool_t _log_debug_enable = 0;

int zvar_master_server_reload_signal = SIGHUP;

void (*zmaster_server_load_config)(zvector_t *cfs) = 0;
void (*zmaster_server_before_service)() = 0;

static void set_signal_handler();

/* LOG ############################################################ */
/* log {{{ */
static zbool_t ___log_stop = 0;
static char *log_service = 0;
static char *log_path = 0; /* need free */
static zlist_t *log_content_list;
static int log_timeunit = -1;
static long log_hour_id = -1;
static int log_sock = -1;
static int log_fd = -1;
static zbuf_t *log_cache = 0;

static pthread_t log_save_pthread_id;
static pthread_t log_recv_pthread_id;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t log_cond = PTHREAD_COND_INITIALIZER;


static void log_write_file(int fd, const char *content, int clen)
{
    int wrotelen = 0;
    while (clen > wrotelen) {
        int ret = write(fd, content + wrotelen, clen - wrotelen);
        if (ret >= 0) {
            wrotelen += ret;
            continue;
        }
        if (errno == EINTR) {
            continue;
        }
        break;
    }
}

static void log_save_content(char *logcontent) 
{
    char *identity = logcontent;
    char *p = strchr(identity, ',');
    if (!p) {
        return;
    }
    *p = 0;
    char *pids = p + 1;
    p = strchr(pids, ',');
    if (!p) {
        return;
    }
    *p = 0;
    char *content = p + 1;

    time_t tl = time(0);
    struct tm tm;
    if (!localtime_r(&tl, &tm)) {
        return;
    }
    long hour_id;
    if (log_timeunit == 1) {
        hour_id = tm.tm_year * (100 * 100 * 100) + tm.tm_mon * (100 * 100) + tm.tm_mday * 100 + tm.tm_hour;
    } else {
        hour_id = tm.tm_year * (100 * 100 * 100) + tm.tm_mon * (100 * 100) + tm.tm_mday * 100;
    }
    if (log_hour_id != hour_id) {
        if (log_fd != -1) {
            if (zbuf_len(log_cache)) {
                log_write_file(log_fd, zbuf_data(log_cache), zbuf_len(log_cache));
            }
            zclose(log_fd);
            log_fd = -1;
        }
        zbuf_reset(log_cache);
    }
    log_hour_id = hour_id;
    if (log_fd == -1) {
        char fpath[4096];
        if (log_timeunit == 1) {
            snprintf(fpath, 4096, "%s%d%02d%02d%02d.log", log_path, tm.tm_year+1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour); 
        } else {
            snprintf(fpath, 4096, "%s%d%02d%02d.log", log_path, tm.tm_year+1900, tm.tm_mon + 1, tm.tm_mday); 
        }
        while (((log_fd = open(fpath, O_CREAT|O_APPEND|O_RDWR|O_CLOEXEC, 0666))==-1) && (errno == EINTR)) {
        }
        if (log_fd == -1) {
            zinfo("can not open %s (%m)", fpath);
            return;
        }
    }
    zbuf_printf_1024(log_cache, "%02d:%02d:%02d %s[%s] ", tm.tm_hour, tm.tm_min, tm.tm_sec, identity, pids);
    zbuf_puts(log_cache, content);
    zbuf_puts(log_cache, "\n");
    if (zbuf_len(log_cache) > 1024 * (100 -1)) {
        log_write_file(log_fd, zbuf_data(log_cache), zbuf_len(log_cache));
        zbuf_reset(log_cache);
    }
}

static void log_flush_all()
{
    if ((log_fd != -1) && (zbuf_len(log_cache))) {
        log_write_file(log_fd, zbuf_data(log_cache), zbuf_len(log_cache));
    }
    zbuf_reset(log_cache);

    if (___log_stop) {
        if (log_fd != -1) {
            zclose(log_fd);
            log_fd = -1;
        }
    }
}

static void *log_save_pthread(void *arg)
{
    struct timespec cond_timeout;
    long last_stamp = 0;
    char *logcontent;

    while(1) {
        cond_timeout.tv_sec = time(0) + 1;
        cond_timeout.tv_nsec = 300;
        pthread_cond_timedwait(&log_cond, &log_mutex, &cond_timeout);
        zpthread_unlock(&log_mutex);

        while (zlist_len(log_content_list)) {
            zpthread_lock(&log_mutex);
            logcontent = 0;
            zlist_shift(log_content_list, (void **)&logcontent);
            zpthread_unlock(&log_mutex);
            if (logcontent) {
                log_save_content(logcontent);
                zfree(logcontent);
            }
        }

        if (time(0) > last_stamp) {
            log_flush_all();
            last_stamp = time(0) + 1;
        }
        if (___log_stop) {
            log_flush_all();
            break;
        }
    }
    return 0;
}

static void *log_recv_pthread(void *arg)
{
    char logbuf[10240 + 1];
    while(1) {
        if (___log_stop) {
            break;
        }
        int num = recvfrom(log_sock, logbuf, 10240, 0, 0, 0);
        if (num == 0) {
            continue;
        }
        if (num < 0) {
            int ec = errno;
            if (ec == EINTR) {
                continue;
            }
            if (ec == EAGAIN) {
                ztimed_read_wait_millisecond(log_sock, 1 * 1000);
                continue;
            }
            zfatal("FATAL log sock error (%m)");
        }

        char *buf_insert = (char *)zmemdupnull(logbuf, num);
        zpthread_lock(&log_mutex);
        zlist_push(log_content_list, buf_insert);
        zpthread_unlock(&log_mutex);
        pthread_cond_signal(&log_cond);
    }

    return 0;
}

static void log_pthread_init()
{
    char *p;
    zargv_t *sv = zargv_create(-1);
    zargv_split_append(sv, log_service, ",");
    zargv_add(sv, "");
    zargv_add(sv, "");
    zargv_add(sv, "");
    char **svp = zargv_data(sv);

    char *listen_path = svp[0];
    if (zempty(listen_path)) {
        zfatal("FATAL ERR -log-service %s, no listen address", log_service);
    }

    log_path = zstrdup(svp[1]);
    if (strlen(log_path) < 1) {
        zfatal("FATAL ERR -log-service %s, no path", log_service);
    }

    p = svp[2];
    if (!strcmp(p, "hour")) {
        log_timeunit = 1;
    } else if (!strcmp(p, "day")) {
        log_timeunit = 2;
    } else {
        log_timeunit = 2;
    }

    log_cache = zbuf_create(1024 * 100);

    struct sockaddr_un server_un;
    if ((log_sock = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        zfatal("FATAL create socket error (%m)");
    }

    if (strlen(listen_path) >= (int)sizeof(server_un.sun_path)) {
        zfatal("FATAL socket path too long: %s", listen_path);
    }

    memset(&server_un, 0, sizeof(struct sockaddr_un));
    server_un.sun_family = AF_UNIX;
    memcpy(server_un.sun_path, listen_path, strlen(listen_path) + 1);

    if ((log_sock = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        zfatal("FATAL create socket error (%m)");
    }

    if ((zunlink(listen_path) < 0) && (errno != ENOENT)) {
        zfatal("FATAL unlink: %s(%m)", listen_path);
    }

    if (bind(log_sock, (struct sockaddr *)&server_un, sizeof(struct sockaddr_un)) < 0) {
        zfatal("FATAL bind %s (%m)", listen_path);
    }
    zclose_on_exec(log_sock, 1);

    log_content_list = zlist_create();
    if(pthread_create(&log_save_pthread_id, 0, log_save_pthread, 0)) {
        zfatal("FATAL pthread_create error (%m)");
    }

    if (pthread_create(&log_recv_pthread_id, 0, log_recv_pthread, 0)) {
        zfatal("FATAL pthread_create error (%m)");
    }

    zargv_free(sv);
}

static void log_pthread_fini()
{
    pthread_join(log_save_pthread_id, 0);
    zlist_free(log_content_list);
    zfree(log_path);
    zbuf_free(log_cache);
}

/* }}} */

/* MASTER ######################################################### */
typedef struct server_info_t server_info_t;
typedef struct listen_pair_t listen_pair_t;
typedef struct child_status_t child_status_t;

struct server_info_t {
    char *config_pn;
    char *config_realpn;
    char *cmd;
    char *master_chroot;
    char *master_chdir;
    int proc_limit;
    int proc_count;
    long stamp_next_start;
    zmap_t *listens; /* <int, listen_pair_t *> */
    zargv_t *args;
};

struct listen_pair_t {
    char *service_name;
    char *uri;
    int used;
    int fd;
    int iuf;
    server_info_t *server;
};

struct child_status_t {
    server_info_t *server;
    zaio_t *status_ev;
    int fd;
    int pid;
    long stamp;
    zargv_t *uri_argv;
};

static char *config_path = 0;
static char *config_realpath = 0;
static int flag_reload = 0;
static int flag_stop = 0;
static int master_status_fd[2];
static zmap_t *listen_pair_map = 0; /* <char *, listen_pair_t *> */
static zmap_t *child_status_map = 0; /* <int, child_status_t *> */
static zvector_t *server_info_vec = 0; /* <server_info_t *> */

static void start_one_child(server_info_t *server);

/* ########################################################### */
server_info_t *server_info_create()
{
    server_info_t *s = (server_info_t *)zcalloc(1, sizeof(server_info_t));
    s->config_pn = 0;
    s->config_realpn = 0;
    s->cmd = 0;
    s->master_chroot = 0;
    s->master_chdir = 0;
    s->listens = zmap_create();
    s->args = zargv_create(10);
    return s;
}

void server_info_free(server_info_t *s)
{
    zfree(s->config_pn);
    zfree(s->config_realpn);
    zfree(s->cmd);
    zfree(s->master_chroot);
    zfree(s->master_chdir);
    zmap_free(s->listens);
    zargv_free(s->args);
    zfree(s);
}

void server_info_start_all(server_info_t *s)
{
    int left = s->proc_limit - s->proc_count;
    long time_permit = ztimeout_set_millisecond(0) - s->stamp_next_start;
    mydebug("master: start_all server:%s, left:%d, permit: %d", s->cmd, left, (time_permit>0?1:0));

    if ((left < 1) || (time_permit <1)) {
        return;
    }

    for (int i = 0; i < left; i++) {
        start_one_child(s);
    }
}

/* ########################################################### */
listen_pair_t *listen_pair_create()
{
    listen_pair_t *lp = (listen_pair_t *)zcalloc(1, sizeof(listen_pair_t));
    lp->service_name = 0;
    lp->uri = 0;
    lp->fd = -1;
    lp->used = 0;
    lp->server = 0;
    return lp;
}

void listen_pair_free(listen_pair_t *lp)
{
    zfree(lp->service_name);
    zfree(lp->uri);
    if (lp->fd != -1) {
        zclose(lp->fd);
    }
    zfree(lp);
}

void listen_pair_set_unused(listen_pair_t *lp)
{
    lp->used = 0;
    lp->server = 0;
}

/* ########################################################### */
child_status_t *child_status_create()
{
    child_status_t *c = (child_status_t *)zcalloc(1, sizeof(child_status_t));
    return c;
}

void child_status_free(child_status_t *c)
{
    if (c->status_ev) {
        zaio_free(c->status_ev, 1);
        c->status_ev = 0;
    }
    if (c->uri_argv) {
        zargv_free(c->uri_argv);
        c->uri_argv = 0;
    }
    zfree(c);
}

/* ########################################################### */
static void child_strike(zaio_t *eio)
{
    child_status_t *cs = (child_status_t *)(zaio_get_context(eio));
    server_info_t *server = cs->server;
    if (server) {
        mydebug("master: child_strike, cmd:%s", server->cmd);
        if (ztimeout_set_millisecond(0) - cs->stamp < 100) {
            server->stamp_next_start  = ztimeout_set_millisecond(0) + 100;
        } else if (ztimeout_set_millisecond(0) - cs->stamp > 1000) {
            server->stamp_next_start  = 0;
        }
        server->proc_count--;
    }
    int fd = cs->fd;
    char intbuf[64];
    sprintf(intbuf, "%d", fd);
    zmap_delete(child_status_map, intbuf, 0);
    child_status_free(cs);
}

static char *_get_relative_path(const char *path, const char *chroot_path, char *resut)
{
    resut[0] = 0;
    int len = strlen(path);
    int clen = strlen(chroot_path);
    if ((clen > len) || (strncmp(path, chroot_path, clen))) {
        zfatal("FATAL path(%s) not in chroot_path(%s)", path, chroot_path);
    }
    strcpy(resut, path + clen);
    return resut;
}

static void start_one_child(server_info_t *server)
{
    mydebug("master: start_one_child, cmd:%s", server->cmd);

    char buf[4100];

    if (server->proc_count >= server->proc_limit) {
        return;
    }

    int status_fd[2];
    if (pipe(status_fd) == -1) {
        zfatal("FATAL master: pipe (%m)");
    }
    pid_t pid = fork();
    if (pid == -1) {
        /* error */
        zclose(status_fd[0]);
        zclose(status_fd[1]);
        zsleep_millisecond(100);
        return;
    } else if (pid) {
        /* parent */
        server->proc_count++;
        zclose(status_fd[0]);
        child_status_t *cs = child_status_create();
        cs->server = server;
        cs->fd = status_fd[1];
        cs->pid = pid;
        cs->status_ev = zaio_create(status_fd[1], zvar_default_aio_base);
        zaio_set_context(cs->status_ev, cs);
        zaio_readable(cs->status_ev, child_strike);
        cs->uri_argv = zargv_create(zmap_len(server->listens));
        ZMAP_WALK_BEGIN(server->listens, fdp, listen_pair_t *, lp) {
            zargv_add(cs->uri_argv, lp->uri);
        } ZMAP_WALK_END;
        sprintf(buf, "%d", cs->fd);
        zmap_update(child_status_map, buf, cs, 0);
        cs->stamp = ztimeout_set_millisecond(0);
        return;
    } else {
        /* child */
        zargv_t *exec_argv = zargv_create(-1);

        zclose(status_fd[1]);
        dup2(status_fd[0], zvar_aio_server_status_fd);
        zclose(status_fd[0]);

        zclose(master_status_fd[0]);
        dup2(master_status_fd[1], zvar_master_server_status_fd);
        zclose(master_status_fd[1]);

        const char *cmdname = strrchr(server->cmd, '/');
        if (cmdname) {
            cmdname++;
        } else {
            cmdname = server->cmd;
        }
        zargv_add(exec_argv, cmdname);
        zargv_add(exec_argv, "MASTER");

        if (config_path && (*config_path)) {
            if (zempty(server->master_chroot)) {
                zargv_add(exec_argv, "-server-config-path");
                zargv_add(exec_argv, config_path);

                if (!zempty(server->config_pn)) {
                    zargv_add(exec_argv, "-config");
                    zargv_add(exec_argv, server->config_pn);
                }
            } else {
                zargv_add(exec_argv, "-server-config-path");
                zargv_add(exec_argv, _get_relative_path(config_realpath, server->master_chroot, buf));

                if (!zempty(server->config_pn)) {
                    zargv_add(exec_argv, "-config");
                    zargv_add(exec_argv, _get_relative_path(server->config_realpn, server->master_chroot, buf));
                }
            }
        }

        int fdnext = zvar_aio_server_listen_fd;
        zbuf_t *service_fork = zbuf_create(-1);
        ZMAP_WALK_BEGIN(server->listens, fdp, listen_pair_t *, lp2) {
            dup2(lp2->fd, fdnext);
            zclose(lp2->fd);
            zbuf_printf_1024(service_fork, "%s:%c%d,", lp2->service_name, lp2->iuf, fdnext);
            fdnext++;
        } ZMAP_WALK_END;
        if (zbuf_len(service_fork) > 1) {
            zbuf_truncate(service_fork, zbuf_len(service_fork)-1);
            zargv_add(exec_argv, "-server-service");
            zargv_add(exec_argv, zbuf_data(service_fork));
        }
        zbuf_free(service_fork);

        ZARGV_WALK_BEGIN(server->args, a) {
            zargv_add(exec_argv, a);
        } ZARGV_WALK_END;

        if (!zempty(server->master_chroot)) {
            if (chroot(server->master_chroot)) {
                zfatal("FATAL chroot (%s) : %m", server->master_chroot);
            }
        }
        if (!zempty(server->master_chdir)) {
            if (chdir(server->master_chdir) == -1) {
                zfatal("FATAL chdir (%s) : %m", server->master_chdir);
            }
        }

        execvp(server->cmd, (char **)(zmemdup(zargv_data(exec_argv), (zargv_len(exec_argv) + 1) * sizeof(char *))));
        zfatal("FATAL master: start child(%s) error: %m", server->cmd);
    }
}

static void remove_old_child()
{
    ZMAP_WALK_BEGIN(child_status_map, pid, child_status_t *, cs) {
        int have = 0;
        ZARGV_WALK_BEGIN(cs->uri_argv, uri) {
            if (have == 0) {
                if (zmap_find(listen_pair_map, uri, 0)) {
                    have = 1;
                }
            }
        } ZARGV_WALK_END;
        if (have == 0) {
            mydebug("kill child");
            kill(cs->pid, SIGTERM);
        }
        cs->server = 0;
    } ZMAP_WALK_END;
}

static void remove_server()
{
    ZVECTOR_WALK_BEGIN(server_info_vec, server_info_t *, info) {
        server_info_free(info);
    } ZVECTOR_WALK_END;
    zvector_reset(server_info_vec);
}

static void set_listen_unused()
{
    ZMAP_WALK_BEGIN(listen_pair_map, key, listen_pair_t *, lp) {
        listen_pair_set_unused(lp);
    } ZMAP_WALK_END;
}

static char *_strdup_realpath(const char *path)
{
    char realpath_buf[4096+1];
    if (zempty(path)) {
        return 0;
    }
    if (!realpath(path, realpath_buf)) {
        zfatal("FATAL master: realpath of %s(%m)", path); 
    }
    return zstrdup(realpath_buf);
}

static void prepare_server_by_config(zconfig_t *cf)
{
    char *cmd, *listen, *pn;
    server_info_t *server;
    cmd = zconfig_get_str(cf, "server-command", 0);
    listen = zconfig_get_str(cf, "server-service", 0);
    if (zempty(cmd) || zempty(listen)) {
        return;
    }
    pn = zconfig_get_str(cf, "___Z_20181216_fn", 0);
    server = server_info_create();
    zvector_push(server_info_vec, server);
    server->config_pn = zstrdup(pn);
    server->cmd = zstrdup(cmd);
    server->config_realpn = _strdup_realpath(pn);
    server->master_chroot = _strdup_realpath(zconfig_get_str(cf, "master-chroot", 0));
    server->master_chdir = zstrdup(zconfig_get_str(cf, "master-chdir", 0));
    server->proc_limit = zconfig_get_int(cf, "server-proc-count", 1);
    server->proc_count = 0;
    if (zempty(pn)) {
        zbuf_t *kk = zbuf_create(-1);
        ZDICT_WALK_BEGIN(cf, k, v) {
            if (!strcmp(k, "server-proc-count")) {
                continue;
            }
            if (!strcmp(k, "server-command")) {
                continue;
            }
            if (!strcmp(k, "server-service")) {
                continue;
            }
            if (!strcmp(k, "master-chroot")) {
                continue;
            }
            zbuf_strcpy(kk, "-");
            zbuf_strcat(kk, k);
            zargv_add(server->args, zbuf_data(kk));
            zargv_add(server->args, zbuf_data(v));
        } ZDICT_WALK_END;
        zbuf_free(kk);
    }

    /* listens */
    zargv_t *splitor = zargv_create(-1);
    zargv_split_append(splitor, listen,  ";, \t");
    ZARGV_WALK_BEGIN(splitor, uri_str) {
        char lbuf[1024];
        if (strlen(uri_str) > 1000) {
            zfatal("FATAL master: service url too long, %s", uri_str);
        }
        strcpy(lbuf, uri_str);
        char *service = lbuf;
        char *uri = strstr(service, "://");
        if (!uri) {
            uri = service;
            service = zblank_buffer;
        } else {
            *uri = 0;
            uri += 3;
        }

        listen_pair_t *lp;
        if (!zmap_find(listen_pair_map, uri, (void **)&lp)) {
            lp = listen_pair_create();
            zmap_update(listen_pair_map, uri, lp, 0);
            lp->uri = zstrdup(uri);
            lp->fd = zlisten(uri, (int *)&(lp->iuf), 5);
            if (lp->fd < 0) {
                zfatal("FATAL master: open %s error", uri);
            }
            znonblocking(lp->fd, 1);
            zclose_on_exec(lp->fd, 1);
        }
        if (lp->used) {
            zfatal("FATAL master: open %s twice", uri);
        }
        lp->used = 1;
        zfree(lp->service_name);
        lp->service_name = zstrdup(service);
        lp->server = server;
        char intbuf[32];
        sprintf(intbuf, "%d", lp->fd);
        zmap_update(server->listens, intbuf, lp, 0);
    } ZARGV_WALK_END;
    zargv_free(splitor);
}

static void reload_config()
{
    zvector_t *cfs = zvector_create(-1);

    if (zmaster_server_load_config) {
        zmaster_server_load_config(cfs);
    } else {
        if (zempty(config_path)) {
            zfatal("FATAL master: need service config path");
        }
        zmaster_server_load_config_from_dirname(config_path, cfs);
    }

    ZVECTOR_WALK_BEGIN(cfs, zconfig_t *, cf) {
        prepare_server_by_config(cf);
        zconfig_free(cf);
    } ZVECTOR_WALK_END;

    zvector_free(cfs);
}

static void release_unused_listen()
{
    zargv_t *delete_list = zargv_create(-1);

    ZMAP_WALK_BEGIN(listen_pair_map, key, listen_pair_t *, lp) {
        if (lp->used) {
            continue;
        }
        zargv_add(delete_list, key);
        listen_pair_free(lp);
    } ZMAP_WALK_END;

    ZARGV_WALK_BEGIN(delete_list, key) {
        zmap_delete(listen_pair_map, key, 0);
    } ZARGV_WALK_END;

    zargv_free(delete_list);
}

static long ___next_start_all_child_stamp = 0;
static void start_all_child()
{
    if (ztimeout_set_millisecond(0) < ___next_start_all_child_stamp) {
        return;
    }
    ZVECTOR_WALK_BEGIN(server_info_vec, server_info_t *, info) {
        server_info_start_all(info);
    } ZVECTOR_WALK_END;
    ___next_start_all_child_stamp = ztimeout_set_millisecond(0) + 100;
}

static void reload_server()
{
    /* MASTER STATUS */
    zclose(master_status_fd[0]);
    zclose(master_status_fd[1]);
    if (pipe(master_status_fd) == -1) {
        zfatal("FATAL master: pipe : %m");
    }
    zclose_on_exec(master_status_fd[0], 1);

    remove_server();
    set_listen_unused();
    reload_config();
    release_unused_listen();
    remove_old_child();

    start_all_child();
}

static zbool_t master_lock_pfile(char *lock_file)
{
    int lock_fd;
    char pid_buf[33];

    lock_fd = open(lock_file, O_RDWR | O_CREAT, 0666);
    if (lock_fd < 0) {
        zinfo("master: open %s(%m)", lock_file);
        return 0;
    }
    zclose_on_exec(lock_fd, 1);

    if (zflock(lock_fd, LOCK_EX | LOCK_NB) < 0) {
        zclose(lock_fd);
        return 0;
    } else {
        sprintf(pid_buf, "%d          ", getpid());
        if (write(lock_fd, pid_buf, 10) != 10) {
            zclose(lock_fd);
            return 0;
        }
        return 1;
    }

    return 0;
}

static void sighup_handler(int sig)
{
    flag_reload = sig;
}

static void sigterm_handler(int sig)
{
    flag_stop = sig;
}

static void set_signal_handler()
{
    zsignal(SIGPIPE, SIG_IGN);
    zsignal(SIGCHLD, SIG_IGN);

    zsignal(SIGTERM, sigterm_handler);

    struct sigaction sig;
    sigemptyset(&sig.sa_mask);
    sig.sa_flags = 0;
    sig.sa_handler = sighup_handler;
    if (sigaction(zvar_master_server_reload_signal, &sig, (struct sigaction *)0) < 0) {
        zfatal("FATAL %s: sigaction(%d) : %m", __FUNCTION__, zvar_master_server_reload_signal);
    }
}

static zbool_t ___init_flag = 0;
static void init_all(int argc, char **argv)
{
    char *lock_file = 0;
    zbool_t try_lock = 0;

    if (___init_flag) {
        zfatal("FATAL master: master::run only be excuted once");
    }
    ___init_flag = 1;

    zmain_argument_run(argc, argv);
    int sl = zconfig_get_int(zvar_default_config, "sleep", 0); 
    if (sl > 0) {
        zsleep_millisecond(sl);
        exit(0);
    }

    _log_debug_enable = zconfig_get_bool(zvar_default_config, "DEBUG", _log_debug_enable);
    try_lock = zconfig_get_bool(zvar_default_config, "try-lock", 0); 
    lock_file = zconfig_get_str(zvar_default_config, "pid-file", 0);
    log_service = zconfig_get_str(zvar_default_config, "log-service", 0);
    config_path = zstrdup(zconfig_get_str(zvar_default_config, "C", ""));
    config_realpath = _strdup_realpath(config_path);
    if (*config_path) {
        if (config_path[strlen(config_path)-1] == '/') {
            config_path[strlen(config_path)-1] = 0;
        }
    }

    if (*config_path) {
        zconfig_t *cf = zconfig_create();
        zmaster_load_global_config_from_dir_inner(cf, config_path);
        zconfig_load_annother(cf, zvar_default_config);
        zconfig_load_annother(zvar_default_config, cf);
        zconfig_free(cf);
    }
    if (zmaster_server_before_service) {
        zmaster_server_before_service();
    }

    if (!zempty(lock_file)) {
        if (try_lock) {
            if (master_lock_pfile(lock_file)) {
                exit(0);
            } else {
                exit(1);
            }
        }
        if (!master_lock_pfile(lock_file)) {
            exit(1);
        }
    }

    /* VARS */
    listen_pair_map = zmap_create();
    child_status_map = zmap_create();
    server_info_vec = zvector_create(-1);
    zvar_default_aio_base = zaio_base_create();

    /* LOG SERVICE */
    if (log_service && strlen(log_service)) {
        log_pthread_init();
    }

    /* SIG */
    set_signal_handler();

    /* MASTER STATUS */
    if (pipe(master_status_fd) == -1) {
        zfatal("FATAL master: pipe : %m");
    }
    zclose_on_exec(master_status_fd[0], 1);

    /* SELF LOG */
    zmaster_log_use_inner(argv[0], zconfig_get_str(zvar_default_config, "server-log", 0));
}

static void fini_all()
{
    ___log_stop = 1;
    if (log_service && strlen(log_service)) {
        pthread_cond_signal(&log_cond);
    }

    remove_server();
    set_listen_unused();
    release_unused_listen();
    remove_old_child();

    if (log_service && strlen(log_service)) {
        mydebug("log_pthread_fini");
        log_pthread_fini();
        mydebug("log_pthread_fini");
    }

    zfree(config_path);
    zfree(config_realpath);
    zmap_free(listen_pair_map);
    zmap_free(child_status_map);
    zvector_free(server_info_vec);

    zaio_base_free(zvar_default_aio_base);
}

static void zmaster_server_main_loop(zaio_base_t *eb)
{
    if (flag_stop) {
        zaio_base_stop_notify(zvar_default_aio_base);
        return;
    }
    if (flag_reload) {
        mydebug("master_reload");
        flag_reload = 0;
        reload_server();
    }
    start_all_child();
}

int zmaster_server_main(int argc, char **argv)
{
    init_all(argc, argv);
    reload_server();
    flag_reload = 0;
    zaio_base_set_loop_fn(zvar_default_aio_base, zmaster_server_main_loop);
    zaio_base_run(zvar_default_aio_base);
    fini_all();
    return 0;
}

void zmaster_server_load_config_from_dirname(const char *config_path, zvector_t *cfs)
{
    DIR *dir;
    struct dirent ent, *ent_list;
    char *fn, *p;
    char pn[4100];

    dir = opendir(config_path);
    if (!dir) {
        zfatal("FATAL master: open %s/(%m)", config_path);
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

        zconfig_t *cf = zconfig_create();
        snprintf(pn, 4096, "%s/%s", config_path, fn);
        zconfig_load_from_pathname(cf, pn);
        zconfig_update_string(cf, "___Z_20181216_fn", pn, -1);
        zvector_push(cfs, cf);
#if 0
        /* do not free cf */
        zconfig_free(cf);
#endif
    }
    closedir(dir);
}

/* Local variables:
* End:
* vim600: fdm=marker
*/

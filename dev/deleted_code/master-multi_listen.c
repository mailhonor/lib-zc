/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-11-10
 * ================================
 */

//
//#include "libzc.h"
//#include <sys/file.h>
//#include <sys/types.h>
//#include <sys/stat.h>
//#include <fcntl.h>
//#include <signal.h>
//#include <sys/types.h>
//#include <sys/wait.h>
//
///* FIXME FREE entry */
//
//#define zmaster_entry_info(m) zverbose("master entry: %s, limit:%d,count:%d, line:%d", \
//	       	m->cmd, m->proc_limit, m->proc_count, __LINE__)
//
//typedef struct
//{
//    char *name;
//    int type;
//    int sock_fd;
//    zev_t zev;
//} zmaster_server_info_t;
//
//typedef struct
//{
//    int enable;
//    int proc_limit;
//    int proc_count;
//    int wakeup;
//    ztimer_t wakeup_timer;
//    int wakeup_flag;
//    char *cmd;
//    char *config;
//    zdict_t *args;
//    zmaster_server_info_t *service_info;
//    int fd_count;
//    int listen_flag;
//    int status_fd[2];
//    int child_error;
//    ztimer_t error_idle;
//    zargv_t *listen_list;
//} zmaster_entry_t;
//
//typedef struct
//{
//    long start_time;
//    zmaster_entry_t *men;
//} zmster_pid_t;
//
//static union
//{
//    struct
//    {
//        int flag;
//        int value;
//    } pid;
//    void *ptr;
//} ___pid_flag;
//static zmaster_load_config_fn_t re_load_config_fn;
//static void *re_load_config_context;
//static zgrid_t *listen_fd_list;
//static zarray_t *server_list;
//static zigrid_t *server_child_list = 0;
//
//static int z_master_pid;
//static int z_master_sig_hup;
//static int z_master_sig_child;
//static int z_master_lock_fd;
//
//static void zmaster_sig_exit(int sig);
//static void zmaster_sig_hup(int sig);
//static void zmaster_sig_child(int sig);
//static void zmaster_reload_service(void);
//static int zmaster_wakeup_child(ztimer_t * tm);
//static int zmaster_start_child(zev_t * zev);
//static void zmaster_reap_child(void);
//static void zmaster_listen_set(zmaster_entry_t * men);
//
//static void zmaster_sig_exit(int sig)
//{
//
//    if (kill(-z_master_pid, SIGTERM) < 0)
//    {
//        zfatal("zmaster_sig_exit: kill process group: %m");
//    }
//    zdebug("now exit");
//    exit(1);
//}
//
//static void zmaster_sig_hup(int sig)
//{
//    zdebug("now reload");
//    z_master_sig_hup = sig;
//}
//
//static void zmaster_sig_child(int sig)
//{
//    z_master_sig_child = sig;
//}
//
//static int ___zmaster_server_init = 0;
//static void zmaster_server_init(void)
//{
//    int i;
//
//    if (___zmaster_server_init)
//    {
//        return;
//    }
//    ___zmaster_server_init = 1;
//
//    /* EVENT */
//    if (!zvar_evbase)
//    {
//        zvar_evbase = zevbase_create();
//    }
//
//    /* VAR */
//    listen_fd_list = zgrid_create();
//    server_child_list = zigrid_create();
//    server_list = zarray_create(1);
//
//    /* SIG */
//    struct sigaction action;
//    int sigs[] = { SIGINT, SIGQUIT, SIGILL, SIGBUS, SIGSEGV, SIGTERM };
//
//    sigemptyset(&action.sa_mask);
//    action.sa_flags = 0;
//
//    action.sa_handler = zmaster_sig_exit;
//    for (i = 0; i < sizeof(sigs) / sizeof(sigs[0]); i++)
//    {
//        if (sigaction(sigs[i], &action, (struct sigaction *)0) < 0)
//        {
//            zfatal("%s: sigaction(%d) : %m", __FUNCTION__, sigs[i]);
//        }
//    }
//
//    action.sa_handler = zmaster_sig_hup;
//    if (sigaction(SIGHUP, &action, (struct sigaction *)0) < 0)
//    {
//        zfatal("%s: sigaction(%d) : %m", __FUNCTION__, SIGHUP);
//    }
//
//    action.sa_handler = zmaster_sig_child;
//    if (sigaction(SIGCHLD, &action, (struct sigaction *)0) < 0)
//    {
//        zfatal("%s: sigaction(%d) : %m", __FUNCTION__, SIGCHLD);
//    }
//}
//
//static void zmaster_reload_config(void)
//{
//    if (zvar_config)
//    {
//        zconfig_free(zvar_config);
//    }
//    zvar_config = zconfig_create();
//
//    if (re_load_config_fn)
//    {
//        re_load_config_fn(re_load_config_context);
//    }
//    else
//    {
//        zconfig_load(zvar_config, (char *)re_load_config_context);
//    }
//}
//
//static int zmaster_parse_service(zmaster_entry_t * men, char *service_str, zmaster_server_info_t * service_info)
//{
//    zargv_t *slist;
//    int slen = 0, i, j;
//    int sock_fd = 0;
//    int type = 0, port;
//    char *stype, *iuf, *p;
//    zgrid_node_t *zdn;
//    char iuf_buf[1024];
//
//    if (!service_str || !*service_str)
//    {
//        return 0;
//    }
//
//    slist = zargv_create(1);
//    zargv_split_append(slist, service_str, " ,;");
//
//    for (i = 0; i < ZARGV_LEN(slist); i++)
//    {
//        stype = ZARGV_ARGV(slist)[i];
//        if (!*stype)
//        {
//            continue;
//        }
//        p = strstr(stype, "://");
//        if (p)
//        {
//            *p = 0;
//            iuf = p + 3;
//        }
//        else
//        {
//            iuf = stype;
//            stype = "";
//        }
//        zargv_add(men->listen_list, iuf);
//        zdn = zgrid_lookup(listen_fd_list, iuf, 0);
//
//        strncpy(iuf_buf, iuf, 1000);
//
//        port = 0;
//        if (!zdn)
//        {
//            p = strchr(iuf_buf, ':');
//            if (p)
//            {
//                type = ZMASTER_LISTEN_INET;
//                *p = 0;
//                p++;
//                port = atoi(p);
//                sock_fd = zinet_listen(iuf_buf, port, -1);
//                p[-1] = ':';
//            }
//            else
//            {
//                type = ZMASTER_LISTEN_UNIX;
//                sock_fd = zunix_listen(iuf_buf, -1);
//            }
//            if (sock_fd < 0)
//            {
//                zfatal("zmaster: can not linsten on %s : %m", iuf);
//            }
//            zclose_on_exec(sock_fd, 1);
//            ___pid_flag.pid.value = sock_fd;
//            ___pid_flag.pid.flag = 1;
//            zdn = zgrid_add(listen_fd_list, iuf, ___pid_flag.ptr, 0);
//        }
//        ___pid_flag.ptr = zgrid_value(zdn);
//        sock_fd = ___pid_flag.pid.value;
//        service_info->name = zstrdup(stype);
//        service_info->type = type;
//        service_info->sock_fd = sock_fd;
//        service_info++;
//        slen++;
//
//        ___pid_flag.pid.flag = 1;
//        zgrid_set_value(zdn, ___pid_flag.ptr);
//    }
//
//    /* sort fd */
//    for (i = 0; i < slen; i++)
//    {
//        for (j = 0; j < slen - i; j++)
//        {
//            if (j + 1 == slen - i)
//            {
//                continue;
//            }
//            if (service_info[j].sock_fd > service_info[j + 1].sock_fd)
//            {
//                int _t, _s;
//                char *_n;
//                _t = service_info[j].type;
//                _n = service_info[j].name;
//                _s = service_info[j].sock_fd;
//
//                service_info[j + 1].type = service_info[j].type;
//                service_info[j + 1].name = service_info[j].name;
//                service_info[j + 1].sock_fd = service_info[j].sock_fd;
//
//                service_info[j].type = _t;
//                service_info[j].name = _n;
//                service_info[j].sock_fd = _s;
//            }
//        }
//    }
//
//    return slen;
//}
//
//static void zmaster_load_server(char *sn, zconfig_t * czc)
//{
//    zmaster_server_info_t service_info[128];
//    int i, fd_count;
//    char *name, *value;
//    int sock_fd;
//    char *listen = 0;
//    zmaster_entry_t *men;
//
//    men = (zmaster_entry_t *) zcalloc(1, sizeof(zmaster_entry_t));
//    men->proc_limit = 1;
//    listen = zstrdup(zconfig_get_str(czc, "zlisten", ""));
//    men->listen_list = zargv_create(1);
//    fd_count = zmaster_parse_service(men, listen, service_info);
//    if (fd_count < 1)
//    {
//        zfree(listen);
//        zargv_free(men->listen_list);
//        zfree(men);
//        zwarning("zmaster: %s have not listen or config error", sn);
//        return;
//    }
//
//    men->args = zdict_create();
//
//    ZCONFIG_WALK_BEGIN(czc, name, value)
//    {
//        if (*name != 'z')
//        {
//            zdict_add(men->args, name, value);
//            continue;
//        }
//        if (!strcmp(name, "zlisten"))
//        {
//        }
//        else if (!strcmp(name, "zcmd"))
//        {
//            men->cmd = zstrdup(value);
//        }
//        else if (!strcmp(name, "zconfig"))
//        {
//            men->config = zstrdup(value);
//        }
//        else if (!strcmp(name, "zproc_limit"))
//        {
//            men->proc_limit = atoi(value);
//            if (men->proc_limit == 0)
//            {
//                men->proc_limit = 1;
//            }
//        }
//        else if (!strcmp(name, "zwakeup"))
//        {
//            men->wakeup = atoi(value);
//        }
//        else
//        {
//            zdict_add(men->args, name, value);
//        }
//    }
//    ZCONFIG_WALK_END;
//
//    men->service_info = (zmaster_server_info_t *) zmemdup(service_info, sizeof(zmaster_server_info_t) * fd_count);
//    men->fd_count = fd_count;
//
//    zarray_add(server_list, men);
//
//    if (pipe(men->status_fd) == -1)
//    {
//        zfatal("zmaster: pipe : %m");
//    }
//    zclose_on_exec(men->status_fd[0], 1);
//    zclose_on_exec(men->status_fd[1], 1);
//
//    for (i = 0; i < men->fd_count; i++)
//    {
//        sock_fd = men->service_info[i].sock_fd;
//        zev_init(&(men->service_info[i].zev), zvar_evbase, sock_fd);
//    }
//    if (men->wakeup > 0)
//    {
//        ztimer_init(&(men->wakeup_timer), zvar_evbase);
//    }
//    ztimer_init(&(men->error_idle), zvar_evbase);
//}
//
//static void zmaster_reload_service(void)
//{
//    int i, pid, kn;
//    /* config */
//    zconfig_t *czc;
//    char *cname;
//    /* server_list */
//    zarray_t *old_server_list;
//    zmaster_entry_t *men;
//    char *keys[10240];
//    char *k;
//    zgrid_node_t *dn;
//    char *iuf;
//
//    old_server_list = server_list;
//    server_list = zarray_create(1);
//
//    /* LISTEN FD clear flag */
//    for (dn = zgrid_first(listen_fd_list); dn; dn = zgrid_next(dn))
//    {
//        ___pid_flag.ptr = zgrid_value(dn);
//        ___pid_flag.pid.flag = 0;
//        zgrid_set_value(dn, ___pid_flag.ptr);
//    }
//
//    /* RELOAD config */
//    zmaster_reload_config();
//
//    ZCONFIG_CHILD_WALK_BEGIN(zvar_config, cname, czc)
//    {
//        if (strncasecmp("zserver ", cname, 8))
//        {
//            continue;
//        }
//        zmaster_load_server(cname + 8, czc);
//    }
//    ZCONFIG_CHILD_WALK_END;
//
//    /* remove old child */
//    zigrid_node_t *idn;
//    zmster_pid_t *mpid;
//    zargv_t *listen_list;
//    for (idn = zigrid_first(server_child_list); idn; idn = zigrid_next(idn))
//    {
//        int find = 0;
//        mpid = (zmster_pid_t *) (zigrid_value(idn));
//        pid = zigrid_key(idn);
//        listen_list = mpid->men->listen_list;
//        ZARGV_WALK_BEGIN(listen_list, iuf)
//        {
//            if (zgrid_lookup(listen_fd_list, iuf, 0))
//            {
//                find = 1;
//                break;
//            }
//        }
//        ZARGV_WALK_END;
//        if (!find)
//        {
//            (void)kill(pid, SIGTERM);
//        }
//        zfree(zidict_value(idn));
//    }
//    zigrid_free(server_child_list, 0, 0);
//    server_child_list = zigrid_create();
//
//    /* RELEASE zmaster_entry_t */
//    ZARRAY_WALK_BEGIN(old_server_list, cname)
//    {
//        men = (zmaster_entry_t *) cname;
//        if (men->cmd)
//        {
//            zfree(men->cmd);
//        }
//        if (men->config)
//        {
//            zfree(men->config);
//        }
//        zargv_free(men->listen_list);
//        zdict_free(men->args);
//        close(men->status_fd[0]);
//        close(men->status_fd[1]);
//        for (i = 0; i < men->fd_count; i++)
//        {
//            /* DETACH ALL socket TRIGGER  */
//            zev_fini(&(men->service_info[i].zev));
//            zfree(men->service_info[i].name);
//        }
//        zfree(men->service_info);
//        if (men->wakeup > 0)
//        {
//            ztimer_fini(&(men->wakeup_timer));
//        }
//        ztimer_fini(&(men->error_idle));
//
//        zfree(men);
//    }
//    ZARRAY_WALK_END;
//    zarray_free(old_server_list, 0, 0);
//
//    /* LISTEN FD clean */
//    kn = zgrid_keys(listen_fd_list, keys, 10240);
//    for (i = 0; i < kn; i++)
//    {
//        k = keys[i];
//        dn = zgrid_lookup(listen_fd_list, k, 0);
//        ___pid_flag.ptr = zgrid_value(dn);
//        pid = ___pid_flag.pid.value;
//        if (!___pid_flag.pid.flag)
//        {
//            close(pid);
//            zgrid_delete(listen_fd_list, k, 0);
//        }
//    }
//
//    /* REGIST NEW TRIGGER */
//    ZARRAY_WALK_BEGIN(server_list, cname)
//    {
//        /* new SOCKET , first TRIGGER SET */
//        men = (zmaster_entry_t *) cname;
//        zmaster_listen_set(men);
//    }
//    ZARRAY_WALK_END;
//}
//
//static int zmaster_wakeup_idle(ztimer_t * tm)
//{
//    zmaster_entry_t *men;
//
//    men = (zmaster_entry_t *) (ztimer_get_context(tm));
//    ztimer_start(&(men->error_idle), 0, 0);
//    men->child_error = 0;
//    zmaster_listen_set(men);
//
//    return 0;
//}
//
//static int zmaster_wakeup_child(ztimer_t * tm)
//{
//    zmaster_entry_t *men;
//
//    men = (zmaster_entry_t *) (ztimer_get_context(tm));
//    if (men->proc_count)
//    {
//        return 0;
//    }
//    zev_set_context(&(men->service_info[0].zev), men);
//    zmaster_start_child(&(men->service_info[0].zev));
//
//    return 0;
//}
//
//static void zmaster_listen_set(zmaster_entry_t * men)
//{
//    int i;
//    int child_error;
//
//    child_error = men->child_error;
//    if (child_error || (men->proc_count >= men->proc_limit))
//    {
//        if (men->listen_flag == 1)
//        {
//            for (i = 0; i < men->fd_count; i++)
//            {
//                zev_set(&(men->service_info[i].zev), 0, 0);
//            }
//            men->listen_flag = 0;
//        }
//        if (child_error)
//        {
//            ztimer_set_context(&(men->error_idle), men);
//            ztimer_start(&(men->error_idle), zmaster_wakeup_idle, 100);
//        }
//    }
//    else if ((men->listen_flag == 0) && (men->proc_count < men->proc_limit))
//    {
//        for (i = 0; i < men->fd_count; i++)
//        {
//            zev_set_context(&(men->service_info[i].zev), men);
//            zev_set(&(men->service_info[i].zev), ZEV_READ, zmaster_start_child);
//        }
//        men->listen_flag = 1;
//    }
//
//    if (men->wakeup > 0)
//    {
//        if (men->proc_count > 0)
//        {
//            if (men->wakeup_flag)
//            {
//                ztimer_start(&(men->wakeup_timer), 0, 0);
//                men->wakeup_flag = 0;
//            }
//        }
//        else
//        {
//            if (!men->wakeup_flag)
//            {
//                ztimer_set_context(&(men->wakeup_timer), men);
//                ztimer_start(&(men->wakeup_timer), zmaster_wakeup_child, men->wakeup * 1000);
//                men->wakeup_flag = 1;
//            }
//        }
//    }
//}
//
//static int zmaster_start_child(zev_t * zev)
//{
//    int rwe, pid, i;
//    zmaster_entry_t *men;
//    zmster_pid_t *mpid;
//    zdict_node_t *dn;
//    char *k, *v;
//
//    rwe = zev_get_events(zev);
//
//    if (ZEV_EXCEPTION & rwe)
//    {
//        return 0;
//    }
//    men = (zmaster_entry_t *) (zev_get_context(zev));
//
//    pid = fork();
//    if (pid == -1)
//    {
//        return 0;
//    }
//    else if (pid)
//    {
//        men->proc_count++;
//        zmaster_listen_set(men);
//        zmaster_entry_info(men);
//        mpid = (zmster_pid_t *) zmalloc(sizeof(zmster_pid_t));
//        mpid->start_time = ztimeout_set(100);
//        mpid->men = men;
//        zigrid_add(server_child_list, pid, mpid, 0);
//        return 0;
//    }
//
//    zargv_t *argv;
//    zbuf_t *child_arg_zbuf;
//
//    argv = zargv_create(1);
//    child_arg_zbuf = zbuf_create(1024);
//
//    zargv_add(argv, men->cmd);
//
//    zargv_add(argv, "-M");
//    zargv_add(argv, "master");
//
//    dup2(men->status_fd[1], ZMASTER_STATUS_FD);
//    for (i = 0; i < men->fd_count; i++)
//    {
//        zargv_add(argv, "-l");
//        zmaster_server_info_t *_si = men->service_info + i;
//        dup2(_si->sock_fd, ZMASTER_STATUS_FD + (i + 1));
//        ZBUF_RESET(child_arg_zbuf);
//        zbuf_sprintf(child_arg_zbuf, "%s://%c:%d", _si->name, _si->type, ZMASTER_STATUS_FD + (i + 1));
//        zargv_add(argv, ZBUF_DATA(child_arg_zbuf));
//    }
//
//    if (men->config)
//    {
//        zargv_t *config_list;
//        char *config_one;
//        config_list = zargv_create(1);
//        zargv_split_append(config_list, men->config, ";, ");
//        ZARGV_WALK_BEGIN(config_list, config_one)
//        {
//            if (*config_one)
//            {
//                zargv_add(argv, "-c");
//                zargv_add(argv, config_one);
//            }
//        }
//        ZARGV_WALK_END;
//        zargv_free(config_list);
//    }
//
//    for (dn = zdict_first(men->args); dn; dn = zdict_next(dn))
//    {
//        zargv_add(argv, "-o");
//        k = zgrid_key(dn);
//        v = (char *)zgrid_value(dn);
//        ZBUF_RESET(child_arg_zbuf);
//        zbuf_strcat(child_arg_zbuf, k);
//        zbuf_strcat(child_arg_zbuf, "=");
//        zbuf_strcat(child_arg_zbuf, v);
//        zargv_add(argv, ZBUF_DATA(child_arg_zbuf));
//    }
//
//    execvp(men->cmd, (char **)(zmemdup(ZARGV_DATA(argv), (ZARGV_LEN(argv) + 1) * sizeof(char *))));
//
//    zfatal("zmaster_start_child error: %m");
//
//    return 0;
//}
//
//static void zmaster_reap_child(void)
//{
//    pid_t pid;
//    int status;
//    zmaster_entry_t *men;
//    zmster_pid_t *mpid;
//    long left;
//
//    while ((pid = waitpid((pid_t) - 1, &status, WNOHANG)) > 0)
//    {
//        zdebug("master: child exit, pid: %d", pid);
//        if (!zigrid_lookup(server_child_list, pid, (char **)&mpid))
//        {
//            continue;
//        }
//        men = mpid->men;
//        men->proc_count--;
//        left = ztimeout_left(mpid->start_time);
//        zfree(mpid);
//        zigrid_delete(server_child_list, pid, 0);
//        if (left > 0)
//        {
//            men->child_error = 1;
//        }
//        zmaster_listen_set(men);
//        zmaster_entry_info(men);
//    }
//}
//
//int zmaster_start(zmaster_load_config_fn_t re_load_config, void *context)
//{
//    if (!re_load_config_fn && !context)
//    {
//        context = "main.cf";
//    }
//    re_load_config_fn = re_load_config;
//    re_load_config_context = context;
//
//    if (setsid()) ;
//    z_master_pid = getpid();
//    zdebug("zmaster_start");
//
//    zmaster_server_init();
//    zmaster_reload_service();
//
//    z_master_sig_hup = 0;
//    z_master_sig_child = 0;
//
//    while (1)
//    {
//        zevbase_dispatch(zvar_evbase, 0);
//        if (z_master_sig_hup)
//        {
//            zdebug("zmaster_reload");
//            z_master_sig_hup = 0;
//            zmaster_reload_service();
//        }
//        if (z_master_sig_child)
//        {
//            z_master_sig_child = 0;
//            zmaster_reap_child();
//        }
//    }
//
//    return 0;
//}
//
//int zmaster_lock_pid(char *lock_fn)
//{
//    char pid_buf[33];
//
//    z_master_lock_fd = open(lock_fn, O_RDWR);
//    if (z_master_lock_fd < 0)
//    {
//        return 0;
//    }
//    zclose_on_exec(z_master_lock_fd, 1);
//
//    if (flock(z_master_lock_fd, LOCK_EX | LOCK_NB))
//    {
//        close(z_master_lock_fd);
//        return 0;
//    }
//    else
//    {
//        sprintf(pid_buf, "%d          ", getpid());
//        if (write(z_master_lock_fd, pid_buf, 10) != 10)
//        {
//            close(z_master_lock_fd);
//            return 0;
//        }
//        return 1;
//    }
//
//    return 0;
//}
//
//int zmaster_get_pid(char *lock_fn)
//{
//    int lock_fd;
//    char pid_buf[33];
//
//    lock_fd = open(lock_fn, O_RDWR);
//    if (lock_fd < 0)
//    {
//        return -1;
//    }
//
//    if (read(lock_fd, pid_buf, 10) != 10)
//    {
//        close(lock_fd);
//        return -1;
//    }
//    close(lock_fd);
//    pid_buf[10] = 0;
//
//    return (atoi(pid_buf));
//}

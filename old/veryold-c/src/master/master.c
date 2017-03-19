#include "zc.h"
/* FIXME FREE entry */

#define zmaster_entry_info(m) ZVERBOSE("master entry: %s, limit:%d,count:%d, line:%d", \
	       	m->cmd, m->proc_limit, m->proc_count, __LINE__)

typedef struct {
	char *name;
	int type;
	int sock_fd;
	ZEVENT zev;
} ZMASTER_SERVICE_INFO;

typedef struct {
	int enable;
	int proc_limit;
	int proc_count;
	int wakeup;
	ZTIMER wakeup_timer;
	int wakeup_flag;
	char *cmd;
	char *cmd_magic;
	char *config;
	ZDICT *args;
	ZMASTER_SERVICE_INFO *service_info;
	int fd_count;
	int listen_flag;
	int status_fd[2];
	int child_error;
	ZTIMER error_idle;
	ZARGV *listen_list;
} MASTER_ENTRY;

typedef struct {
	long start_time;
	MASTER_ENTRY *men;
} MASTER_PID;

static union {
	struct {
		int flag;
		int value;
	} pid;
	void *ptr;
} ___pid_flag;
static ZMASTER_LOAD_CONFIG re_load_config_fn;
static void *re_load_config_context;
static ZDICT *listen_fd_list;
static ZARRAY *server_list;
static ZIDICT *server_child_list = 0;

static int z_master_pid;
static int z_master_sig_hup;
static int z_master_sig_child;
static int z_master_lock_fd;

static void zmaster_sig_exit(int sig);
static void zmaster_sig_hup(int sig);
static void zmaster_sig_child(int sig);
static void zmaster_reload_service(void);
static int zmaster_wakeup_child(ZTIMER * tm, void *ctx);
static int zmaster_start_child(ZEVENT * zev, void *ctx);
static void zmaster_reap_child(void);
static void zmaster_listen_set(MASTER_ENTRY * men);

static void zmaster_sig_exit(int sig)
{

	if (kill(-z_master_pid, SIGTERM) < 0) {
		zlog_fatal("zmaster_sig_exit: kill process group: %m");
	}
	zlog_debug("now exit");
	exit(1);
}

static void zmaster_sig_hup(int sig)
{
	zlog_debug("now reload");
	z_master_sig_hup = sig;
}

static void zmaster_sig_child(int sig)
{
	z_master_sig_child = sig;
}

static int ___zmaster_server_init = 0;
static void zmaster_server_init(void)
{
	int i;

	if (___zmaster_server_init) {
		return;
	}
	___zmaster_server_init = 1;

	/* EVENT */
	if (!zvar_default_event_base) {
		zvar_default_event_base = zevent_base_create();
	}

	/* VAR */
	listen_fd_list = zdict_create();
	server_child_list = zidict_create();
	server_list = zarray_create(1);

	/* SIG */
	struct sigaction action;
	int sigs[] = { SIGINT, SIGQUIT, SIGILL, SIGBUS, SIGSEGV, SIGTERM };

	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;

	action.sa_handler = zmaster_sig_exit;
	for (i = 0; i < sizeof(sigs) / sizeof(sigs[0]); i++) {
		if (sigaction(sigs[i], &action, (struct sigaction *)0) < 0) {
			zlog_fatal("%s: sigaction(%d) : %m", __FUNCTION__, sigs[i]);
		}
	}

	action.sa_handler = zmaster_sig_hup;
	if (sigaction(SIGHUP, &action, (struct sigaction *)0) < 0) {
		zlog_fatal("%s: sigaction(%d) : %m", __FUNCTION__, SIGHUP);
	}

	action.sa_handler = zmaster_sig_child;
	if (sigaction(SIGCHLD, &action, (struct sigaction *)0) < 0) {
		zlog_fatal("%s: sigaction(%d) : %m", __FUNCTION__, SIGCHLD);
	}
}

static void zmaster_reload_config(void)
{
	if (zvar_default_config) {
		zdict_config_free(zvar_default_config);
	}
	zvar_default_config = zdict_create();

	if (re_load_config_fn) {
		re_load_config_fn(re_load_config_context);
	} else {
		zdict_config_load_file(zvar_default_config, (char *)re_load_config_context);
	}
}

static int zmaster_parse_service(MASTER_ENTRY * men, char *service_str, ZMASTER_SERVICE_INFO * service_info)
{
	ZARGV *slist;
	int slen = 0, i, j;
	int sock_fd = 0;
	int type, port;
	char *stype, *iuf, *p;
	char *host_path;
	ZDICT_NODE *zdn;
	char iuf_buf[128];

	if (!service_str || !*service_str) {
		return 0;
	}

	slist = zargv_split(service_str, " ,;");

	for (i = 0; i < ZARGV_LEN(slist); i++) {
		stype = ZARGV_ARGV(slist)[i];
		if (!*stype) {
			continue;
		}
		p = strstr(stype, "://");
		if (p) {
			*p = 0;
			iuf = p + 3;
		} else {
			iuf = stype;
			stype = "";
		}
		zargv_add(men->listen_list, iuf);
		zdn = zdict_lookup(listen_fd_list, iuf, 0);

		strcpy(iuf_buf, iuf);
		host_path = 0;
		port = 0;
		___ziuf_parse(iuf_buf, type, host_path, port);
		if (!zdn) {
			if (type == ZSOCKET_TYPE_INET) {
				sock_fd = zsocket_inet_listen(host_path, port, -1);
			} else if (type == ZSOCKET_TYPE_UNIX) {
				sock_fd = zsocket_unix_listen(host_path, -1, 1, 0);
			} else if (type == ZSOCKET_TYPE_FIFO) {
				sock_fd = zsocket_fifo_listen(host_path, 1, 0);
			} else {
				zlog_fatal("master: services config error: %s", iuf);
			}
			if (sock_fd < 0) {
				zlog_fatal("zmaster: can not linsten on %s : %m", iuf);
			}
			zio_close_on_exec(sock_fd, 1);
			___pid_flag.pid.value = sock_fd;
			___pid_flag.pid.flag = 1;
			zdn = zdict_enter(listen_fd_list, iuf, ___pid_flag.ptr, 0);
		}
		___pid_flag.ptr = ZDICT_VALUE(zdn);
		sock_fd = ___pid_flag.pid.value;
		service_info->name = zstr_strdup(stype);
		service_info->type = type;
		service_info->sock_fd = sock_fd;
		service_info++;
		slen++;

		___pid_flag.pid.flag = 1;
		ZDICT_UPDATE_VALUE(zdn, ___pid_flag.ptr);
	}

	/* sort fd */
	for (i = 0; i < slen; i++) {
		for (j = 0; j < slen - i; j++) {
			if (j + 1 == slen - i) {
				continue;
			}
			if (service_info[j].sock_fd > service_info[j + 1].sock_fd) {
				int _t, _s;
				char *_n;
				_t = service_info[j].type;
				_n = service_info[j].name;
				_s = service_info[j].sock_fd;

				service_info[j + 1].type = service_info[j].type;
				service_info[j + 1].name = service_info[j].name;
				service_info[j + 1].sock_fd = service_info[j].sock_fd;

				service_info[j].type = _t;
				service_info[j].name = _n;
				service_info[j].sock_fd = _s;
			}
		}
	}

	return slen;
}

static void zmaster_load_server(char *sn, ZDICT * czc)
{
	ZMASTER_SERVICE_INFO service_info[128];
	int i, fd_count;
	char *name, *value;
	int sock_fd;
	char *listen = 0;
	MASTER_ENTRY *men;

	men = (MASTER_ENTRY *) zcalloc(1, sizeof(MASTER_ENTRY));
	listen = zstr_strdup(zdict_get_str(czc, "zlisten", ""));
	men->listen_list = zargv_create(1);
	fd_count = zmaster_parse_service(men, listen, service_info);
	if (fd_count < 1) {
		zfree(listen);
		zargv_free(men->listen_list);
		zfree(men);
		zlog_warning("zmaster: %s have not listen or config error", sn);
		return;
	}

	men->args = zdict_create();

	ZDICT_CONFIG_WALK_BEGIN(czc, name, value) {
		if (*name != 'z') {
			zdict_enter_STR(men->args, name, value);
			continue;
		}
		if (!strcmp(name, "zlisten")) {
		} else if (!strcmp(name, "zcmd")) {
			men->cmd = zstr_strdup(value);
		} else if (!strcmp(name, "zcmd_magic")) {
			men->cmd_magic = zstr_strdup(value);
		} else if (!strcmp(name, "zconfig")) {
			men->config = zstr_strdup(value);
		} else if (!strcmp(name, "zproc_limit")) {
			men->proc_limit = atoi(value);
			if (men->proc_limit == 0) {
				men->proc_limit = 1;
			}
		} else if (!strcmp(name, "zwakeup")) {
			men->wakeup = atoi(value);
		} else {
			zdict_enter_STR(men->args, name, value);
		}
	}
	ZDICT_CONFIG_WALK_END;

	men->service_info = (ZMASTER_SERVICE_INFO *) zstr_memdup(service_info, sizeof(ZMASTER_SERVICE_INFO) * fd_count);
	men->fd_count = fd_count;

	zarray_enter(server_list, men);

	if (pipe(men->status_fd) == -1) {
		zlog_fatal("zmaster: pipe : %m");
	}
	zio_close_on_exec(men->status_fd[0], 1);
	zio_close_on_exec(men->status_fd[1], 1);

	for (i = 0; i < men->fd_count; i++) {
		sock_fd = men->service_info[i].sock_fd;
		zevent_init(&(men->service_info[i].zev), zvar_default_event_base, sock_fd);
	}
	if (men->wakeup > 0) {
		ztimer_init(&(men->wakeup_timer), zvar_default_event_base);
	}
	ztimer_init(&(men->error_idle), zvar_default_event_base);
}

static void zmaster_reload_service(void)
{
	int i, pid, kn;
	/* config */
	ZDICT *czc;
	char *cname;
	/* server_list */
	ZARRAY *old_server_list;
	MASTER_ENTRY *men;
	char *keys[10240];
	char *k;
	ZDICT_NODE *dn;
	char *iuf;

	old_server_list = server_list;
	server_list = zarray_create(1);

	/* LISTEN FD clear flag */
	for (dn = zdict_first(listen_fd_list); dn; dn = zdict_next(dn)) {
		___pid_flag.ptr = ZDICT_VALUE(dn);
		___pid_flag.pid.flag = 0;
		ZDICT_UPDATE_VALUE(dn, ___pid_flag.ptr);
	}

	/* RELOAD config */
	zmaster_reload_config();

	ZDICT_CONFIG_WALK_CHILD_BEGIN(zvar_default_config, cname, czc) {
		if (strncasecmp("zserver ", cname, 8)) {
			continue;
		}
		zmaster_load_server(cname + 8, czc);
	}
	ZDICT_CONFIG_WALK_CHILD_END;

	/* remove old child */
	ZIDICT_NODE *idn;
	MASTER_PID *mpid;
	ZARGV *listen_list;
	for (idn = zidict_first(server_child_list); idn; idn = zidict_next(idn)) {
		int find = 0;
		mpid = (MASTER_PID *) (ZIDICT_VALUE(idn));
		pid = ZIDICT_KEY(idn);
		listen_list = mpid->men->listen_list;
		ZARGV_WALK_BEGIN(listen_list, iuf) {
			if (zdict_lookup(listen_fd_list, iuf, 0)) {
				find = 1;
				break;
			}
		}
		ZARRAY_WALK_END;
		if (!find) {
			(void)kill(pid, SIGTERM);
		}
		zfree(ZIDICT_VALUE(idn));
	}
	zidict_free(server_child_list, 0, 0);
	server_child_list = zidict_create();

	/* RELEASE MASTER_ENTRY */
	ZARRAY_WALK_BEGIN(old_server_list, cname) {
		men = (MASTER_ENTRY *) cname;
		if (men->cmd) {
			zfree(men->cmd);
		}
		if (men->cmd_magic) {
			zfree(men->cmd_magic);
		}
		if (men->config) {
			zfree(men->config);
		}
		zargv_free(men->listen_list);
		zdict_free_STR(men->args);
		close(men->status_fd[0]);
		close(men->status_fd[1]);
		for (i = 0; i < men->fd_count; i++) {
			/* DETACH ALL socket TRIGGER  */
			zevent_fini(&(men->service_info[i].zev));
			zfree(men->service_info[i].name);
		}
		zfree(men->service_info);
		if (men->wakeup > 0) {
			ztimer_fini(&(men->wakeup_timer));
		}
		ztimer_fini(&(men->error_idle));

		zfree(men);
	}
	ZARRAY_WALK_END;
	zarray_free(old_server_list, 0, 0);

	/* LISTEN FD clean */
	kn = zdict_keys(listen_fd_list, keys, 10240);
	for (i = 0; i < kn; i++) {
		k = keys[i];
		dn = zdict_lookup(listen_fd_list, k, 0);
		___pid_flag.ptr = ZDICT_VALUE(dn);
		pid = ___pid_flag.pid.value;
		if (!___pid_flag.pid.flag) {
			close(pid);
			zdict_remove(listen_fd_list, k, 0);
		}
	}

	/* REGIST NEW TRIGGER */
	ZARRAY_WALK_BEGIN(server_list, cname) {
		/* new SOCKET , first TRIGGER SET */
		men = (MASTER_ENTRY *) cname;
		zmaster_listen_set(men);
	}
	ZARRAY_WALK_END;
}

static int zmaster_wakeup_idle(ZTIMER * tm, void *ctx)
{
	MASTER_ENTRY *men;

	men = (MASTER_ENTRY *) ctx;
	ztimer_set(&(men->error_idle), 0, 0, 0);
	men->child_error = 0;
	zmaster_listen_set(men);

	return 0;
}

static int zmaster_wakeup_child(ZTIMER * tm, void *ctx)
{
	MASTER_ENTRY *men;

	men = (MASTER_ENTRY *) ctx;
	if (men->proc_count) {
		return 0;
	}
	zmaster_start_child(&(men->service_info[0].zev), ctx);

	return 0;
}

static void zmaster_listen_set(MASTER_ENTRY * men)
{
	int i;
	int child_error;

	child_error = men->child_error;
	if (child_error || (men->proc_count >= men->proc_limit)) {
		if (men->listen_flag == 1) {
			for (i = 0; i < men->fd_count; i++) {
				zevent_set(&(men->service_info[i].zev), 0, 0, 0, 0);
			}
			men->listen_flag = 0;
		}
		if (child_error) {
			ztimer_set(&(men->error_idle), zmaster_wakeup_idle, men, 100);
		}
	} else if ((men->listen_flag == 0) && (men->proc_count < men->proc_limit)) {
		for (i = 0; i < men->fd_count; i++) {
			zevent_set(&(men->service_info[i].zev), ZEVENT_READ, zmaster_start_child, men, 0);
		}
		men->listen_flag = 1;
	}

	if (men->wakeup > 0) {
		if (men->proc_count > 0) {
			if (men->wakeup_flag) {
				ztimer_set(&(men->wakeup_timer), 0, 0, 0);
				men->wakeup_flag = 0;
			}
		} else {
			if (!men->wakeup_flag) {
				ztimer_set(&(men->wakeup_timer), zmaster_wakeup_child, men, men->wakeup * 1000);
				men->wakeup_flag = 1;
			}
		}
	}
}

static int zmaster_start_child(ZEVENT * zev, void *ctx)
{
	int rwe, pid, i;
	MASTER_ENTRY *men;
	MASTER_PID *mpid;
	ZDICT_NODE *dn;
	char *k, *v;

	rwe = zevent_get_events(zev);

	if (ZEVENT_EXCEPTION & rwe) {
		return 0;
	}
	men = (MASTER_ENTRY *) ctx;

	pid = fork();
	if (pid == -1) {
		return 0;
	} else if (pid) {
		men->proc_count++;
		zmaster_listen_set(men);
		zmaster_entry_info(men);
		mpid = (MASTER_PID *) zmalloc(sizeof(MASTER_PID));
		mpid->start_time = zmtime_set_timeout(100);
		mpid->men = men;
		zidict_enter(server_child_list, pid, mpid, 0);
		return 0;
	}

	ZARRAY *argv;
	ZBUF *child_arg_zbuf;

	argv = zarray_create(1);
	child_arg_zbuf = zbuf_create(1024);

	zarray_enter(argv, men->cmd);

	zarray_enter(argv, "c");

	if (men->cmd_magic) {
		ZBUF_RESET(child_arg_zbuf);
		zbuf_strcat(child_arg_zbuf, "zcmd_magic=");
		zbuf_strcat(child_arg_zbuf, men->cmd_magic);
		zarray_enter(argv, zstr_strdup(ZBUF_DATA(child_arg_zbuf)));
	}

	dup2(men->status_fd[1], ZMASTER_STATUS_FD);
	for (i = 0; i < men->fd_count; i++) {
		ZMASTER_SERVICE_INFO *_si = men->service_info + i;
		dup2(_si->sock_fd, ZMASTER_STATUS_FD + (i + 1));
		ZBUF_RESET(child_arg_zbuf);
		zbuf_sprintf(child_arg_zbuf, "zlisten=%s://%c:%d", _si->name, _si->type, ZMASTER_STATUS_FD + (i + 1));
		zarray_enter(argv, zstr_strdup(ZBUF_DATA(child_arg_zbuf)));
	}

	if (men->config) {
		ZBUF_RESET(child_arg_zbuf);
		zbuf_strcat(child_arg_zbuf, "zconfig=");
		zbuf_strcat(child_arg_zbuf, men->config);
		zarray_enter(argv, zstr_strdup(ZBUF_DATA(child_arg_zbuf)));
	}

	for (dn = zdict_first(men->args); dn; dn = zdict_next(dn)) {
		k = ZDICT_KEY(dn);
		v = (char *)ZDICT_VALUE(dn);
		ZBUF_RESET(child_arg_zbuf);
		zbuf_strcat(child_arg_zbuf, k);
		zbuf_strcat(child_arg_zbuf, "=");
		zbuf_strcat(child_arg_zbuf, v);
		zarray_enter(argv, zstr_strdup(ZBUF_DATA(child_arg_zbuf)));
	}
	zarray_enter(argv, 0);

	execvp(men->cmd, (char **)(zstr_memdup(ZARRAY_DATA(argv), ZARRAY_LEN(argv) * sizeof(char *))));

	zlog_fatal("zmaster_start_child error: %m");

	return 0;
}

static void zmaster_reap_child(void)
{
	pid_t pid;
	int status;
	MASTER_ENTRY *men;
	MASTER_PID *mpid;
	long left;

	while ((pid = waitpid((pid_t) - 1, &status, WNOHANG)) > 0) {
		ZDEBUG("master: child exit, pid: %d", pid);
		if (!zidict_lookup(server_child_list, pid, (char **)&mpid)) {
			continue;
		}
		men = mpid->men;
		men->proc_count--;
		left = zmtime_left(mpid->start_time);
		zfree(mpid);
		zidict_remove(server_child_list, pid, 0);
		if (left > 0) {
			men->child_error = 1;
		}
		zmaster_listen_set(men);
		zmaster_entry_info(men);
	}
}

int zmaster_start(ZMASTER_LOAD_CONFIG re_load_config, void *context)
{
	if (!re_load_config_fn && !context) {
		context = "main.cf";
	}
	re_load_config_fn = re_load_config;
	re_load_config_context = context;

	if (setsid()) ;
	z_master_pid = getpid();
	ZDEBUG("zmaster_start");

	zmaster_server_init();
	zmaster_reload_service();

	z_master_sig_hup = 0;
	z_master_sig_child = 0;

	while (1) {
		zevent_base_dispatch(zvar_default_event_base, 0);
		if (z_master_sig_hup) {
			ZDEBUG("zmaster_reload");
			z_master_sig_hup = 0;
			zmaster_reload_service();
		}
		if (z_master_sig_child) {
			z_master_sig_child = 0;
			zmaster_reap_child();
		}
	}
	return 0;
}

int zmaster_lock_pid(char *lock_fn)
{
	char pid_buf[33];

	z_master_lock_fd = open(lock_fn, O_RDWR);
	if (z_master_lock_fd < 0) {
		return 0;
	}
	zio_close_on_exec(z_master_lock_fd, 1);

	if (flock(z_master_lock_fd, LOCK_EX | LOCK_NB)) {
		close(z_master_lock_fd);
		return 0;
	} else {
		sprintf(pid_buf, "%d          ", getpid());
		if (write(z_master_lock_fd, pid_buf, 10) != 10) {
			close(z_master_lock_fd);
			return 0;
		}
		return 1;
	}

	return 0;
}

int zmaster_get_pid(char *lock_fn)
{
	int lock_fd;
	char pid_buf[33];

	lock_fd = open(lock_fn, O_RDWR);
	if (lock_fd < 0) {
		return -1;
	}

	if (read(lock_fd, pid_buf, 10) != 10) {
		close(lock_fd);
		return -1;
	}
	close(lock_fd);
	pid_buf[10] = 0;

	return (atoi(pid_buf));
}

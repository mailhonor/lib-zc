#include "zc.h"

ZFN_VOID zmaster_server_before_service = 0;
ZFN_VOID zmaster_server_on_reload = 0;
int zmaster_server_base_stopped = 0;

static ZARRAY *z_master_server_listen_fd_list = 0;
static int z_master_server_test_mode = 0;
static ZEVENT z_master_server_status_zev;
static int zmaster_server_register_once = 0;

typedef struct {
	ZMASTER_SERVER_FN callback;
	int type;
	void *ctx;
} ___ACCEPT_CONTEXT;

static int zmaster_server_master_reload(ZEVENT * zev, void *ctx)
{
	zevent_fini(&z_master_server_status_zev);
	close(ZMASTER_STATUS_FD);

	if (zmaster_server_on_reload) {
		zmaster_server_on_reload();
	} else {
		exit(0);
	}

	return 0;
}

void zmaster_server_load_config_list(char *config_list)
{
	ZARGV *zav;
	char *cf;

	zav = zargv_split(config_list, ",");
	ZARGV_WALK_BEGIN(zav, cf) {
		if (*cf) {
			zdict_config_load_file(zvar_default_config, cf);
		}
	}
	ZARGV_WALK_END;
	zargv_free(zav);
}

static ZMASTER_SERVER_SERVICE *zmaster_server_register_find_service(ZMASTER_SERVER_SERVICE * service_list, char *id)
{
	ZMASTER_SERVER_SERVICE *service;

	for (service = service_list; service->id; service++) {
		if (!strcmp(service->id, id)) {
			return service;
		}
	}

	return service_list;
}

static int zmaster_server_accept(ZEVENT * zev, void *ctx)
{
	int fd;
	int listen_fd;
	___ACCEPT_CONTEXT *a_c;

	listen_fd = zev->fd;
	a_c = (___ACCEPT_CONTEXT *) ctx;

	if (a_c->type == 'i') {
		fd = zsocket_inet_accept(listen_fd);
	} else if (a_c->type == 'u') {
		fd = zsocket_unix_accept(listen_fd);
	} else {
		fd = listen_fd;
	}

	if (fd < 0) {
		if (errno != EAGAIN) {
			zlog_fatal("zmaster_server_accept: %m");
		}
		return -1;
	}
	if ((a_c->type == 'i') || (a_c->type == 'u')) {
		zio_nonblocking(fd, 1);
	}

	if (a_c->callback) {
		a_c->callback(fd, a_c->ctx, a_c->type);
	}

	return 0;
}

static void zmaster_server_register_one(char *service_str, ZMASTER_SERVER_SERVICE * service_list)
{
	ZMASTER_SERVER_SERVICE *service;
	int type, sock_fd;
	ZEVENT *zev;
	___ACCEPT_CONTEXT *a_c;
	char _service_str[1024];
	char *stype, *uri, *p;

	strcpy(_service_str, service_str);
	stype = _service_str;
	p = strstr(_service_str, "://");
	if (p) {
		*p = 0;
		uri = p + 3;
	} else {
		stype = "zdefault";
		uri = _service_str;
	}

	if (!z_master_server_test_mode) {
		p = strchr(uri, ':');
		if (p == 0) {
			zlog_fatal("%s: args error: %s", zvar_program_name, service_str);
		}
		*p = 0;
		type = *uri;
		sock_fd = atoi(p + 1);
	} else {
		char *host_path;
		int port;

		___ziuf_parse(uri, type, host_path, port);

		if (type == ZSOCKET_TYPE_INET) {
			sock_fd = zsocket_inet_listen(host_path, port, -1);
		} else if (type == ZSOCKET_TYPE_UNIX) {
			sock_fd = zsocket_unix_listen(host_path, -1, 1, 0);
		} else if (type == ZSOCKET_TYPE_FIFO) {
			sock_fd = zsocket_fifo_listen(host_path, 1, 0);
		} else {
			sock_fd = -1;
			zlog_fatal("%s: args error: %s", zvar_program_name, service_str);
		}
		if (sock_fd < 0) {
			zlog_fatal("%s: open: %s error (%m)", zvar_program_name, service_str);
		}
	}

	service = zmaster_server_register_find_service(service_list, stype);

	if (service->raw_flag) {
		if (service->callback) {
			service->callback(sock_fd, service->ctx, type);
		}
		return;
	}
	a_c = (___ACCEPT_CONTEXT *) zmalloc(sizeof(___ACCEPT_CONTEXT));
	a_c->callback = service->callback;
	a_c->type = type;
	a_c->ctx = service->ctx;

	zio_close_on_exec(sock_fd, 1);
	zio_nonblocking(sock_fd, 1);
	zev = (ZEVENT *) zmalloc(sizeof(ZEVENT));
	zevent_init(zev, zvar_default_event_base, sock_fd);
	zevent_set(zev, ZEVENT_READ, zmaster_server_accept, a_c, 0);

	zarray_enter(z_master_server_listen_fd_list, ZINT_TO_VOID_PTR(sock_fd));
}

static ZARGV *___zmaster_server_register_save = 0;
static void zmaster_server_register(char *service_str, ZMASTER_SERVER_SERVICE * service_list)
{
	if (___zmaster_server_register_save == 0) {
		___zmaster_server_register_save = zargv_create(1);
	}
	zargv_add(___zmaster_server_register_save, service_str);
	zmaster_server_register_once = 1;
}

static void zmaster_server_register_true(ZMASTER_SERVER_SERVICE * service_list)
{
	char *ss;

	if (___zmaster_server_register_save == 0) {
		return;
	}

	ZARGV_WALK_BEGIN(___zmaster_server_register_save, ss) {
		zmaster_server_register_one(ss, service_list);
	}
	ZARGV_WALK_END;
}

int zmaster_server_main(int argc, char **argv, ZMASTER_SERVER_SERVICE * service_list)
{
	int i;
	char *n, *v;

	signal(SIGPIPE, SIG_IGN);

	zvar_program_name = zstr_strdup(argv[0]);
	zvar_default_event_base = zevent_base_create();
	if (!zvar_default_config) {
		zvar_default_config = zdict_create();
	}

	if (argc == 1) {
		z_master_server_test_mode = 1;
	}

	z_master_server_listen_fd_list = zarray_create(1);
	for (i = 1; i < argc; i++) {
		n = argv[i];
		if ((i == 1)) {
			if (strcmp(n, "c")) {
				z_master_server_test_mode = 1;
			} else {
				continue;
			}
		}
		v = strchr(n, '=');
		if (!v) {
			zlog_fatal("%s: cmd args error", zvar_program_name);
		}
		*v++ = 0;

		if (!strcmp("zlisten", n)) {
			zmaster_server_register(v, service_list);
		} else if (!strcmp("zcmd_magic", n)) {
		} else if (!strcmp("zconfig", n)) {
			zmaster_server_load_config_list(v);
		} else {
			zdict_enter_STR(zvar_default_config, n, v);
		}
	}

	if (!zmaster_server_register_once && z_master_server_test_mode) {
		zmaster_server_register("zdefault://inet:127.0.0.1:99", service_list);
	}

	if (zmaster_server_before_service) {
		zmaster_server_before_service();
	}

	zmaster_server_register_true(service_list);

	zio_close_on_exec(ZMASTER_STATUS_FD, 1);
	if (!z_master_server_test_mode) {
		zio_nonblocking(ZMASTER_STATUS_FD, 1);
		zevent_init(&z_master_server_status_zev, zvar_default_event_base, ZMASTER_STATUS_FD);
		zevent_set(&z_master_server_status_zev, ZEVENT_READ, zmaster_server_master_reload, 0, 0);
	}

	while (1) {
		zevent_base_dispatch(zvar_default_event_base, 0);
		if (zmaster_server_base_stopped) {
			break;
		}
	}

	ZARRAY_WALK_BEGIN(z_master_server_listen_fd_list, v) {
		i = ZVOID_PTR_TO_INT(v);
		close(i);
	}
	ZARRAY_WALK_END;

	return 0;
}

void zmaster_server_disconnect(int fd)
{
	close(fd);
}

void zmaster_server_event_stop(void)
{
	zmaster_server_base_stopped = 1;
	zevent_base_notify(zvar_default_event_base);
}

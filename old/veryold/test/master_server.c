#include "zc.h"
#include "test_lib.h"

static ZEVENT_BASE *some_event_base_1 = 0;
static ZEVENT_BASE *some_event_base_2 = 0;
static ZEVENT_BASE *some_event_base_3 = 0;
static ZEVENT_BASE *some_aio_base_1 = 0;

void fn_before(void)
{
	some_event_base_1 = zvar_default_event_base;
	some_event_base_2 = zvar_default_event_base;
	some_event_base_3 = zvar_default_event_base;

	some_aio_base_1 = zevent_base_create();
	test_event_base_pthread_start(some_aio_base_1);

	zlog_info("before");
}

/* TRIGGER */
int fn_error_1_1(ZEVENT * zev, void *ctx)
{
	int events, fd;

	events = zevent_get_events(zev);
	fd = zevent_get_fd(zev);

	if (events & ZEVENT_TIMEOUT) {
		zlog_info("%d: idle too long", fd);
	} else {
		zlog_info("%d: connection error", fd);
	}

	zevent_fini(zev);
	zfree(zev);
	if (ctx) {
		zsio_free((ZSIO *) ctx);
	}
	zmaster_server_disconnect(fd);

	return 0;
}

int fn_error_1_2(int ret, ZEVENT * zev, void *ctx)
{
	int fd;

	fd = zevent_get_fd(zev);

	if (ret == -1) {
		zlog_info("%d: connection error", fd);
	} else {
		zlog_info("%d: idle too long", fd);
	}

	zevent_fini(zev);
	zfree(zev);
	if (ctx) {
		zsio_free((ZSIO *) ctx);
	}
	zmaster_server_disconnect(fd);

	return 0;
}

int fn_onread_1(ZEVENT * zev, void *ctx)
{
	int fd, events, ret;
	char buf[1024], *p;
	ZSIO *fp;

	events = zevent_get_events(zev);
	fd = zevent_get_fd(zev);

	if (events & ZEVENT_EXCEPTION) {
		fn_error_1_1(zev, ctx);
		return 1;
	}

	fp = (ZSIO *) ctx;
	ret = zsio_read_line(fp, buf, 1024);

	if (ret < 0) {
		fn_error_1_2(ret, zev, fp);
		return -1;
	}
	buf[ret] = 0;
	if (!strncmp(buf, "exit", 4)) {
		zsio_free(fp);
		zevent_fini(zev);
		zfree(zev);
		zmaster_server_disconnect(fd);

		return 0;
	}
	p = strchr(buf, '\r');
	if (p) {
		*p = 0;
	}
	p = strchr(buf, '\n');
	if (p) {
		*p = 0;
	}

	zsio_fprintf(fp, "your input: %s\n", buf);
	ret = ZSIO_FFLUSH(fp);
	if (ret < 0) {
		fn_error_1_2(ret, zev, fp);
		return -1;
	}

	zevent_set(zev, ZEVENT_READ, fn_onread_1, fp, 100 * 1000);

	return 0;
}

int fn_welcome_1(ZEVENT * zev, void *ctx)
{
	int events, ret;
	ZSIO *fp;

	events = zevent_get_events(zev);
	if (events & ZEVENT_EXCEPTION) {
		fn_error_1_1(zev, 0);
		return 1;
	}

	fp = zsio_create(0);
	zsio_set_FD(fp, zevent_get_fd(zev));
	zsio_fprintf(fp, "welcome service id : %s\n", (char *)ctx);
	ret = ZSIO_FFLUSH(fp);
	if (ret < 0) {
		fn_error_1_2(ret, zev, fp);
		return 1;
	}

	zevent_set(zev, ZEVENT_READ, fn_onread_1, fp, 100 * 1000);

	return 0;
}

void fn_server_1(int fd, void *ctx, int type)
{
	ZEVENT *zev;

	zev = (ZEVENT *) zmalloc(sizeof(ZEVENT));
	zevent_init(zev, some_event_base_1, fd);
	zevent_set(zev, ZEVENT_WRITE, fn_welcome_1, ctx, 100 * 1000);
}

void fn_server_3(int fd, void *ctx, int type)
{
	ZSIO *fp;
	char buf[10240];
	int ret;

	fp = zsio_create(0);
	zsio_set_FD(fp, fd);
	zsio_set_timeout(fp, 60 * 1000);
	zsio_fprintf(fp, "welcome service id : %s\n", (char *)ctx);
	while (1) {
		ret = zsio_read_line(fp, buf, 1024);
		if (ret < 1) {
			printf("read len: %d, eof: %d, err: %d, errno str: %m\n", ret, ZSIO_FEOF(fp), ZSIO_FERROR(fp));
			break;
		}
		buf[ret] = 0;
		if (!strncmp(buf, "exit", 4)) {
			break;
		}
		if (!strncmp(buf, "EXIT", 4)) {
			printf("now EXIT\n");
			exit(0);
		}
		zsio_fprintf(fp, "your input: %s", buf);
	}
	zsio_free(fp);
	zmaster_server_disconnect(fd);
}

int main(int argc, char **argv)
{
	zmaster_server_before_service = fn_before;

	ZMASTER_SERVER_SERVICE service_list[] = {
		{"s1", fn_server_1, "111", 0},	/* trigger */
		{"s2", test_server_aio, &some_aio_base_1, 0},	/* aio */
		{"s3", fn_server_3, "333", 0},	/* block, like signle pthread mode */
		{0}
	};

	zmaster_server_main(argc, argv, service_list);

	return 0;
}

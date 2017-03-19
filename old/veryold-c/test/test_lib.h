#ifndef ___ZC_INCLUDE__

#include<stdio.h>
int main()
{
	fprintf(stderr, "This programe only provide some basic funcs to other programs used for test in this test-dir.\n");
	return 0;
}

#else
#include <syslog.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <pthread.h>

/* time used */
static struct timeval ___var_debug_usetimer_start;
static struct timeval ___var_debug_usetimer_end;
static struct timeval ___var_debug_usetimer_sub;

void test_usetime(char *p)
{
	gettimeofday(&___var_debug_usetimer_end, 0);
	ZTIME_SUB(___var_debug_usetimer_sub, ___var_debug_usetimer_end, ___var_debug_usetimer_start);
	zlog_info("%s: use time: %ld.%ld", p ? p : "", ___var_debug_usetimer_sub.tv_sec, ___var_debug_usetimer_sub.tv_usec);
	gettimeofday(&___var_debug_usetimer_start, 0);
}

/* log */
static pthread_mutex_t _vshow11111_mutex_lock = PTHREAD_MUTEX_INITIALIZER;

int _vshow11111(int level, const char *fmt, va_list ap)
{
	pthread_mutex_lock(&_vshow11111_mutex_lock);

	fprintf(stderr, "%s[%d] [%lu]: ", zvar_program_name, getpid(), pthread_self());
	vfprintf(stderr, fmt, ap);
	fputs("\n", stderr);

	pthread_mutex_unlock(&_vshow11111_mutex_lock);

	return 0;
}

void test_log_on_pthread(void)
{
	zlog_set_output(_vshow11111);
}

/* parameters */
ZDICT *test_argv_dict = 0;
char *test_argv_extra = 0;

void test_init(int argc, char **argv)
{
	int i;

	zvar_program_name = argv[0];
	/* debug */
	{
		char *env;
		gettimeofday(&___var_debug_usetimer_start, 0);

		env = getenv("ZDEBUG");
		if (env && zstr_to_bool(env, 0)) {
			zvar_log_debug = 1;
		}
		env = getenv("ZVERBOSE");
		if (env && zstr_to_bool(env, 0)) {
			zvar_log_verbose = 1;
		}
	}
	
	/* deal argv */
	{
		char key[1024], *value;
		test_argv_dict = zdict_create();
		for(i=1;i<argc;i++){
			value = argv[i];
			if(strcmp(value, "-o")){
				test_argv_extra = value;
				continue;
			}
			if(i+1==argc){
				continue;
			}
			i++;
			strncpy(key, argv[i], 1024);
			value=strchr(key, '=');
			if(!value){
				continue;
			}
			*value++ = 0;
			zdict_enter_STR(test_argv_dict, key, value);
		}
	}
}

/* load /etc/passwd */
void test_load_passwd(char **buf, ZARGV ** as)
{
	FILE *fp;
	char p[1024000];
	int ret;

	fp = fopen("/etc/passwd", "r");
	ret = fread(p, 1, 1024000, fp);
	p[ret] = 0;

	if (buf) {
		*buf = zstr_strndup(p, ret);
	}
	if (as) {
		*as = zargv_split(p, "\n");
	}
	fclose(fp);
}

/* event base dispatch run pthread */
static int test_event_base_pthread_stop = 0;
void *test_event_base_pthread(void *arg)
{
	ZEVENT_BASE *eb;

	pthread_detach(pthread_self());
	eb = (ZEVENT_BASE *) arg;
	while (1) {
		zevent_base_dispatch(eb, 0);
		if(test_event_base_pthread_stop){
			break;
		}
	}
	printf("exit\n");

	return arg;
}
void test_event_base_pthread_start(ZEVENT_BASE *eb)
{
	pthread_t pth;
	pthread_create(&pth, 0, test_event_base_pthread, eb);
}

/* print dict */
void test_dict_print(ZDICT *zd, int value_is_str)
{
	ZDICT_NODE *zn;
	for(zn = zdict_first(zd);zn;zn=zdict_next(zn)){
		if(value_is_str){
			zlog_info("dict: %s: %s", ZDICT_KEY(zn), ZDICT_VALUE(zn));
		}else{
			zlog_info("dict: %s", ZDICT_KEY(zn));
		}
	}
}


/* aio */
static int test_server_aio_after_write(ZAIO * aio, void *context, char *wbuf);

static int test_server_aio_error(ZAIO * aio, void *context)
{
	int ret, fd;

	ret = zaio_get_ret(aio);
	fd = zaio_get_fd(aio);

	if (ret == -1) {
		zlog_info("%d: connection error", fd);
	} else {
		zlog_info("%d: idle too long", fd);
	}
	zaio_fini(aio);
	zfree(aio);
	zmaster_server_disconnect(fd);

	return -1;
}

static int test_server_aio_read(ZAIO * aio, void *context, char *rbuf)
{
	int ret, fd, len;
	char *p;

	ret = zaio_get_ret(aio);
	fd = zaio_get_fd(aio);
	if (ret < 1) {
		return test_server_aio_error(aio, context);
	}

	if (ret > 3 && !strncmp(rbuf, "exit", 4)) {
		zaio_fini(aio);
		zfree(aio);
		zmaster_server_disconnect(fd);
		return 0;
	}

	rbuf[ret] = 0;
	p = strchr(rbuf, '\r');
	if (p) {
		*p = 0;
	}
	p = strchr(rbuf, '\n');
	if (p) {
		*p = 0;
	}
	len = strlen(rbuf);

	zaio_write_cache_append(aio, "your input:   ", 12);
	zaio_write_cache_append(aio, rbuf, len);
	zaio_write_cache_append(aio, "\n", 1);
	zaio_write_cache_flush(aio, test_server_aio_after_write, context, -1);

	return 0;
}

static int test_server_aio_after_write(ZAIO * aio, void *context, char *wbuf)
{
	int ret;

	ret = zaio_get_ret(aio);

	if (ret < 1) {
		return test_server_aio_error(aio, context);
	}

	zaio_read_delimiter(aio, '\n', 1024, test_server_aio_read, 0, 5 * 1000);

	return 0;
}

static int test_server_aio_attach(ZAIO *aio, void *ctx, char *unused)
{
	char buf[1024];
	long t;

	t = time(0);
	sprintf(buf, "welcome: %s\n", ctime(&t));
	zaio_write_cache_append(aio, buf, strlen(buf));
	zaio_write_cache_flush(aio, test_server_aio_after_write, 0, 10 * 1000);

	return 0;
}

void test_server_aio(int fd, void *ctx, int type)
{
	ZAIO *aio;
	ZEVENT_BASE *eb;

	eb = *(ZEVENT_BASE **)ctx;

	aio = (ZAIO *) zmalloc(sizeof(ZAIO));
	zaio_init(aio, eb, fd);

	zaio_attach(aio, test_server_aio_attach, 0);
}

#endif

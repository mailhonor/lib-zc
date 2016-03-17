#include "zc.h"

int zvar_log_debug = 0;
int zvar_log_verbose = 0;

static volatile int fatal_times = 0;
static int zlog_vshow_default(int level, const char *fmt, va_list ap);
static ZLOG_HANDLER zlog_vshow_handler = zlog_vshow_default;

static int zlog_vshow_default(int level, const char *fmt, va_list ap)
{
	vfprintf(stderr, fmt, ap);
	fputs("\n", stderr);

	return 0;
}

void zlog_set_output(ZLOG_HANDLER handler)
{
	if (handler) {
		zlog_vshow_handler = handler;
	} else {
		zlog_vshow_handler = zlog_vshow_default;
	}
}

int zlog_fatal(const char *fmt, ...)
{
	va_list ap;

	if (fatal_times++ == 0) {
		va_start(ap, fmt);
		zlog_vshow_handler(LOG_CRIT, fmt, ap);
		va_end(ap);
	}
	sleep(1);
	_exit(1);

	return -1;
}

int zlog_error(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	zlog_vshow_handler(LOG_ERR, fmt, ap);
	va_end(ap);

	return 0;
}

int zlog_warning(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	zlog_vshow_handler(LOG_WARNING, fmt, ap);
	va_end(ap);

	return 0;
}

int zlog_info(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	zlog_vshow_handler(LOG_INFO, fmt, ap);
	va_end(ap);

	return 0;
}

int zlog_debug(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	zlog_vshow_handler(LOG_DEBUG, fmt, ap);
	va_end(ap);

	return 0;
}

/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-09-27
 * ================================
 */

#include "libzc.h"

static volatile int fatal_times = 0;
static int zlog_output_default(int level, char *fmt, ...);
static int zlog_fatal_output_default(int level, char *fmt, ...);
static int zlog_voutput_default(int level, char *fmt, va_list ap);

int zvar_fatal_catch = 0;
int zvar_log_level = ZLOG_INFO;
zlog_output_t zlog_output = zlog_output_default;
zlog_output_t zlog_fatal_output = zlog_fatal_output_default;
zlog_voutput_t zlog_voutput = zlog_voutput_default;

static int zlog_voutput_default(int level, char *fmt, va_list ap)
{
    vfprintf(stderr, fmt, ap);
    fputs("\n", stderr);

    return 0;
}

int zlog_output_default(int level, char *fmt, ...)
{
    va_list ap;

    if (zlog_voutput) {
        va_start(ap, fmt);
        zlog_voutput(level, fmt, ap);
        va_end(ap);
    }
    return 0;
}

int zlog_fatal_output_default(int level, char *fmt, ...)
{
    va_list ap;

    if (fatal_times++ == 0) {
        if (zlog_voutput) {
            va_start(ap, fmt);
            zlog_voutput(level, fmt, ap);
            va_end(ap);
        }
    }

    if (zvar_fatal_catch) {
        /* 段错误,方便 gdb 调试 */
        char *p = 0;
        *p = 0;
    }

    sleep(1);
    _exit(1);

    return 0;
}

zlog_voutput_t zlog_set_voutput(zlog_voutput_t voutput_fn)
{
    zlog_voutput_t last;

    last = zlog_voutput;
    zlog_voutput = voutput_fn;

    return last;
}

static int ___zlog_set_level_console = 0;
int zlog_set_level(int level)
{
    int last;

    last = zvar_log_level;
    if (___zlog_set_level_console) {
        return last;
    }
    zvar_log_level = level;

    return last;
}

int zlog_set_level_from_console(int level)
{
    int last;

    ___zlog_set_level_console = 1;
    last = zvar_log_level;
    zvar_log_level = level;

    return last;
}

int zlog_parse_level(char *levstr)
{
    char *level_list[] = { "",
        "fatal", "error", "warning", "notice", "info", "debug", "verbose",
        0
    };
    char *ptr;
    int level_i;

    for (level_i = 1, ptr = level_list[1]; (ptr = level_list[level_i], ptr); level_i++) {
        if (!strcasecmp(levstr, ptr)) {
            return level_i;
        }
    }

    return ZLOG_INFO;
}

/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2015-10-12
 * ================================
 */

#include "zc.h"

int zvar_fatal_catch = 0;
int zvar_debug_mode = 0;
static void zlog_voutput_default(const char *fmt, va_list ap);
zlog_voutput_t zlog_voutput = zlog_voutput_default;

static volatile int fatal_times = 0;
static void zlog_voutput_default(const char *fmt, va_list ap)
{
    vfprintf(stderr, fmt, ap);
    fputs("\n", stderr);
}

void zinfo(const char *fmt, ...)
{
    va_list ap;
    if (zlog_voutput) {
        va_start(ap, fmt);
        zlog_voutput(fmt, ap);
        va_end(ap);
    }
}

void zfatal(const char *fmt, ...)
{
    va_list ap;

    if (fatal_times++ == 0) {
        if (zlog_voutput) {
            va_start(ap, fmt);
            zlog_voutput(fmt, ap);
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
}

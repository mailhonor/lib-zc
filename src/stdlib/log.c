/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-10-12
 * ================================
 */

#include "zc.h"

#include <assert.h>
#include <unistd.h>
#ifdef __linux__
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <syslog.h>
#endif

zbool_t zvar_log_fatal_catch = 0;
zbool_t zvar_log_debug_enable = 0;
zbool_t zvar_log_output_disable = 0;

static void zvprintf_default(const char *source_fn, size_t line_number, const char *fmt, va_list ap);

static void zvprintf_default(const char *source_fn, size_t line_number, const char *fmt, va_list ap)
{
    char fmt_buf[1024 + 1];
    if (line_number)
    {
        zsnprintf(fmt_buf, 1024, "%s [%s:%zu]\n", fmt, source_fn, line_number);
    }
    else
    {
        zsnprintf(fmt_buf, 1024, "%s\n", fmt);
    }
    zvfprintf(stderr, fmt_buf, ap);
}
void (*zlog_vprintf)(const char *source_fn, size_t line_number, const char *fmt, va_list ap) = zvprintf_default;

void zlog_info(const char *source_fn, size_t line_number, const char *fmt, ...)
{
    va_list ap;
    if (zlog_vprintf && (!zvar_log_output_disable))
    {
        va_start(ap, fmt);
        zlog_vprintf(source_fn, line_number, fmt, ap);
        va_end(ap);
    }
}

void zvlog_info(const char *source_fn, size_t line_number, const char *fmt, va_list ap)
{
    if (zlog_vprintf && (!zvar_log_output_disable))
    {
        zlog_vprintf(source_fn, line_number, fmt, ap);
    }
}

static volatile int fatal_times = 0;
void zlog_fatal(const char *source_fn, size_t line_number, const char *fmt, ...)
{

    va_list ap;
    va_start(ap, fmt);
    zvlog_fatal(source_fn, line_number, fmt, ap);
    va_end(ap);
}

void zvlog_fatal(const char *source_fn, size_t line_number, const char *fmt, va_list ap)
{
    if (fatal_times++ == 0)
    {
        if (zlog_vprintf)
        {
            va_list ap2;
            va_copy(ap2, ap);
            zlog_vprintf(source_fn, line_number, fmt, ap2);
            va_end(ap2);
        }

        zvfprintf(stderr, fmt, ap);
        zfprintf(stderr, "\n");
    }

    if (zvar_log_fatal_catch)
    {
        assert(0);
    }

    _exit(1);
}

/* STDOUT ############################################## */
static void zvprintf_stdout(const char *source_fn, size_t line_number, const char *fmt, va_list ap)
{
    char fmt_buf[1024 + 1];
    if (line_number)
    {
        zsnprintf(fmt_buf, 1024, "%s [%s:%zu]\n", fmt, source_fn, line_number);
    }
    else
    {
        zsnprintf(fmt_buf, 1024, "%s\n", fmt);
    }
    zvfprintf(stdout, fmt_buf, ap);
}

void zlog_use_stdout()
{
    zlog_vprintf = zvprintf_stdout;
}

void zlog_use_default()
{
    zlog_vprintf = zvprintf_default;
}

#ifdef __linux__
/* SYSLOG ############################################## */
static void zvprintf_syslog(const char *source_fn, size_t line_number, const char *fmt, va_list ap)
{
    char buf[10240 + 10];
    zvsnprintf(buf, 10240, fmt, ap);
    if (line_number)
    {
        syslog(LOG_INFO, "%s [%s:%ld]", buf, source_fn, (long)line_number);
    }
    else
    {
        syslog(LOG_INFO, "%s", buf);
    }
}

static const char *zvar_syslog_identity = 0;
void zlog_use_syslog(const char *identity, int facility)
{
    if (identity)
    {
        zfree(zvar_syslog_identity);
    }
    zvar_syslog_identity = zstrdup(identity);
    openlog(identity, LOG_NDELAY | LOG_PID, facility);
    zlog_vprintf = zvprintf_syslog;
}

int zlog_get_facility_from_str(const char *facility)
{
    int fa = LOG_SYSLOG;
    if (!strncasecmp(facility, "LOG_", 4))
    {
        facility += 4;
    }
#define ___LOG_S_I(S)              \
    if (!strcasecmp(#S, facility)) \
    {                              \
        fa = LOG_##S;              \
    }
    ___LOG_S_I(KERN);
    ___LOG_S_I(USER);
    ___LOG_S_I(MAIL);
    ___LOG_S_I(DAEMON);
    ___LOG_S_I(AUTH);
    ___LOG_S_I(SYSLOG);
    ___LOG_S_I(LPR);
    ___LOG_S_I(NEWS);
    ___LOG_S_I(UUCP);
    ___LOG_S_I(CRON);
    ___LOG_S_I(AUTHPRIV);
    ___LOG_S_I(FTP);
    ___LOG_S_I(LOCAL0);
    ___LOG_S_I(LOCAL1);
    ___LOG_S_I(LOCAL2);
    ___LOG_S_I(LOCAL3);
    ___LOG_S_I(LOCAL4);
    ___LOG_S_I(LOCAL5);
    ___LOG_S_I(LOCAL6);
    ___LOG_S_I(LOCAL7);
#undef ___LOG_S_I

    return fa;
}

/* MASTER LOG ########################################################## */
static char *zvar_master_log_prefix = 0;
static int zvar_master_log_prefix_len = 0;
static int zvar_master_log_sock = -1;
static struct sockaddr_un *zvar_master_log_client_un = 0;

static void vprintf_masterlog(const char *source_fn, size_t line_number, const char *fmt, va_list ap)
{
    char *buf = (char *)zmalloc(10240 + 10);
    int len = 0, tmplen;
    int left = 10240;

    memcpy(buf, zvar_master_log_prefix, zvar_master_log_prefix_len);
    len = zvar_master_log_prefix_len;
    left = left - len;

    if (left > 1)
    {
        zvsnprintf(buf + len, left, fmt, ap);
        tmplen = strlen(buf + len);
        len += tmplen;
        left -= tmplen;
    }
    if (left > 1)
    {
        if (line_number)
        {
            zsnprintf(buf + len, left, " [%s:%ld]", source_fn, (long)line_number);
        }
        else
        {
        }
        tmplen = strlen(buf + len);
        len += tmplen;
        left -= tmplen;
    }
    buf[len] = 0;
    sendto(zvar_master_log_sock, buf, len, 0, (struct sockaddr *)zvar_master_log_client_un, sizeof(struct sockaddr_un));
    free(buf);
}

static void clear_masterlog(void *unused)
{
    if (zvar_master_log_prefix)
    {
        zfree(zvar_master_log_prefix);
        zvar_master_log_prefix = 0;
    }
    if (zvar_master_log_sock != -1)
    {
        zclose(zvar_master_log_sock);
        zvar_master_log_sock = -1;
    }
    if (zvar_master_log_client_un)
    {
        zfree(zvar_master_log_client_un);
        zvar_master_log_client_un = 0;
    }
}

#include <sys/syscall.h>
zinline static int ___syscall_socket(int domain, int type, int protocol)
{
    return syscall(__NR_socket, domain, type, protocol);
}

void zlog_use_masterlog(const char *identity, const char *dest)
{
    zlog_vprintf = vprintf_masterlog;
    char buf[256];
    zsnprintf(buf, 255, "%s,%d,", identity, getpid());
    zvar_master_log_prefix = zstrdup(buf);
    zvar_master_log_prefix_len = strlen(buf);

    if ((zvar_master_log_sock = ___syscall_socket(AF_UNIX, SOCK_DGRAM, 0)) < 0)
    {
        zdebug_show("ERROR socket (%m)");
        exit(1);
    }

    zvar_master_log_client_un = (struct sockaddr_un *)zcalloc(1, sizeof(struct sockaddr_un));
    zvar_master_log_client_un->sun_family = AF_UNIX;
    if (strlen(dest) >= sizeof(zvar_master_log_client_un->sun_path))
    {
        zdebug_show("socket unix path too long: %s", dest);
        exit(1);
    }
    strcpy(zvar_master_log_client_un->sun_path, dest);

    zatexit(clear_masterlog, 0);
}
#endif // __linux__

/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-10-12
 * ================================
 */

#include <cstdarg>
#include "zcc/zcc_stdlib.h"
#if __linux__
#include <assert.h>
#include <syslog.h>
#endif // __linux__

zcc_namespace_begin;
zcc_general_namespace_begin(logger);

bool var_fatal_catch = false;
bool var_debug_enable = false;
bool var_verbose_enable = false;
bool var_output_disable = false;

static void output_handler_default(const char *source_fn, uint64_t line_number, level ll, const char *fmt, va_list ap)
{
    if (ll == level::output)
    {
        std::vfprintf(stderr, fmt, ap);
        std::fputs("\n", stderr);
    }
    else
    {
        char fmt_buf[1024 + 1];
        std::snprintf(fmt_buf, 1024, "%s [%s:%zu]\n", fmt, source_fn, line_number);
        std::vfprintf(stderr, fmt_buf, ap);
    }
}

output_handler_type var_output_handler = output_handler_default;

static int fatal_times = 0;
void vlog_output(const char *source_fn, uint64_t line_number, level ll, const char *fmt, va_list ap)
{
    if (ll == fatal)
    {
        if (fatal_times++)
        {
            return;
        }
        output_handler_default(source_fn, line_number, ll, fmt, ap);
        if ((!var_output_disable) && var_output_handler && (var_output_handler != output_handler_default))
        {
            var_output_handler(source_fn, line_number, ll, fmt, ap);
        }
#ifdef __linux__
        if (var_fatal_catch)
        {
            assert(0);
        }
#endif // __linux__
        exit(1);
    }
    else
    {
        if ((!var_output_disable) && var_output_handler)
        {
            var_output_handler(source_fn, line_number, ll, fmt, ap);
        }
        if (ll == error_and_exit)
        {
            exit(1);
        }
    }
}

void log_output(const char *source_fn, uint64_t line_number, level ll, const char *fmt, ...)
{
    if (var_output_disable)
    {
        return;
    }
    va_list ap;
    va_start(ap, fmt);
    vlog_output(source_fn, line_number, ll, fmt, ap);
    va_end(ap);
}

// syslog

void use_syslog_by_config(const char *attr /* facility[,identity */)
{
    if (empty(attr))
    {
        return;
    }
    auto ks = split(attr, ',');
    if (ks.size() == 1)
    {
        std::string cmdname = get_cmd_pathname();
        if (cmdname.empty())
        {
            use_syslog(nullptr, ks[0]);
        }
        else
        {
            const char *ps = cmdname.c_str();
            const char *p = strrchr(ps, '/');
            if (p)
            {
                p++;
            }
            else
            {
                p = ps;
            }
            use_syslog(p, ks[0]);
        }
    }
    else if (ks.size() == 2)
    {
        use_syslog(ks[1], ks[0]);
    }
}

static void output_handler_syslog(const char *source_fn, uint64_t line_number, level ll, const char *fmt, va_list ap)
{
    if (ll == level::output)
    {
        std::vfprintf(stderr, fmt, ap);
        std::fputs("\n", stderr);
    }
    else
    {
#ifdef _WIN64
#else  // _WIN64
        char fmt_buf[1024 + 1];
        std::snprintf(fmt_buf, 1024, "%s [%s:%zu]\n", fmt, source_fn, line_number);
        vsyslog(LOG_INFO, fmt_buf, ap);
#endif // _WIN64
    }
}

void use_syslog(int facility)
{
    use_syslog(nullptr, facility);
}

static std::string var_syslog_identity;
void use_syslog(const char *identity, int facility)
{
#ifdef _WIN64
#else  // _WIN64
    if (identity)
    {
        var_syslog_identity = identity;
    }
    if (var_syslog_identity.empty())
    {
        identity = nullptr;
    }
    else
    {
        identity = var_syslog_identity.c_str();
    }

    openlog(identity, LOG_NDELAY | LOG_PID, facility);
    var_output_handler = output_handler_syslog;
#endif // _WIN64
}

void use_syslog(const char *identity, const char *facility)
{
    int f = get_facility(facility);
    use_syslog(identity, f);
}

int get_facility(const char *facility)
{
#ifdef _WIN64
    return -1;
#else // _WIN64
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
#endif // _WIN64
}

zcc_general_namespace_end(logger);
zcc_namespace_end;
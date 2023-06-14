/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2022-01-14
 * ================================
 */

#ifdef __linux__
#include "zc.h"
#include <signal.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

/* {{{ _exit_after */
static void ___timeout_do(int pid)
{
    exit(1);
}

static void _exit_after()
{
    int i = zconfig_get_second(zvar_default_config, "exit-after", 0);
    if (i < 1)
    {
        return;
    }
    alarm(0);
    zsignal(SIGALRM, ___timeout_do);
    alarm(i);
}
/* }}} */

/* {{{ _fork_concurrency */
static void _fork_concurrency()
{
    /* 主要用于测试 */
    int cc = zconfig_get_int(zvar_default_config, "fork-concurrency", 0);
    if (cc < 1)
    {
        return;
    }

    for (int ci = 0; ci < cc; ci++)
    {
        if (!fork())
        {
            return;
        }
    }

    int status;
    while (wait(&status) > 0)
    {
        cc--;
        if (cc == 0)
        {
            _exit(0);
        }
    }
    _exit(1);
}
/* }}} */

/* {{{ _config_memleak_check */
int zvar_memleak_check = 0;

static void _sigint_handler(int sig)
{
    zvar_sigint_flag = 1;
}

static void _config_memleak_check()
{
    if (!(zvar_memleak_check = zconfig_get_bool(zvar_default_config, "memleak-check", zvar_memleak_check)))
    {
        char *env_ld_preload = getenv("LD_PRELOAD");
        if (env_ld_preload && strstr(env_ld_preload, "vgpreload_memcheck"))
        {
            zvar_memleak_check = 1;
        }
    }
    if (!zvar_memleak_check)
    {
        return;
    }
    zsignal(SIGINT, _sigint_handler);
}
/* }}} */

/* {{{ _quick_setrlimit */
static zbool_t _quick_setrlimit(int cmd, unsigned long cur_val)
{
    struct rlimit rlim;
    if (getrlimit(RLIMIT_CORE, &rlim) == 0)
    {
        if (rlim.rlim_cur != cur_val)
        {
            rlim.rlim_cur = cur_val;
            if (setrlimit(cmd, &rlim))
            {
                zinfo("WARNING: setrlimit(%m)");
                return 0;
            }
        }
    }
    else
    {
        zinfo("WARNING: getrlimit(%m)");
    }
    return 1;
}
/* }}} */

/* {{{ zset_core_file_size */
zbool_t zset_core_file_size(int megabyte)
{
    unsigned long l = RLIM_INFINITY;
    if (megabyte >= 0)
    {
        l = 1UL * 1024 * 1024 * megabyte;
    }
    return _quick_setrlimit(RLIMIT_CORE, l);
}

static void _config_core_file_size()
{
    int v;
    long vv = zconfig_get_size(zvar_default_config, "core-file-size", -2);
    if (vv < -1)
    {
        return;
    }
    if (vv == -1)
    {
        v = vv;
    }
    else
    {
        v = (int)(vv / (1024 * 1024));
    }
    if (!zset_core_file_size(v))
    {
        zinfo("WARNING set core-file-size error");
    }
}
/* }}} */

/* {{{ zset_max_mem */
zbool_t zset_max_mem(int megabyte)
{
    int ret = 1;
    unsigned long l = RLIM_INFINITY;
    if (megabyte >= 0)
    {
        l = 1UL * 1024 * 1024 * megabyte;
    }
#ifdef RLIMIT_AS
    ret = ret && _quick_setrlimit(RLIMIT_AS, l);
#else
#ifdef RLIMIT_RSS
    ret = ret && _quick_setrlimit(RLIMIT_RSS, l);
#endif
#endif

#ifdef RLIMIT_DATA
    ret = ret && _quick_setrlimit(RLIMIT_DATA, l);
#endif
    return ret;
}

static void _config_max_mem()
{
    int v;
    long vv = zconfig_get_size(zvar_default_config, "max-memory", -2);
    if (vv < -1)
    {
        return;
    }
    if (vv == -1)
    {
        v = vv;
    }
    else
    {
        v = (int)(vv / (1024 * 1024));
    }
    if (!zset_max_mem(v))
    {
        zinfo("WARNING set max-memory error");
    }
}
/* }}} */

/* {{{ zset_cgroup_name */
zbool_t zset_cgroup_name(const char *name)
{
    if (!zempty(name))
    {
        return 0;
    }
    int r = 0;

    long pid = (long)getpid();
    char pbuf[128];
    sprintf(pbuf, "%ld", pid);

    char fn[1024 + 1];
    snprintf(fn, 1024, "/sys/fs/cgroup/memory/%s/cgroup.procs", name);

    int fd = zopen(fn, O_RDONLY, 0);
    if (fd == -1)
    {
        zinfo("WARNING open %s(%m)", fn);
        return 0;
    }
    while (1)
    {
        if (write(fd, pbuf, strlen(pbuf)) != -1)
        {
            if (errno == EINTR)
            {
                continue;
            }
            break;
        }
        r = 1;
        break;
    }
    close(fd);

    return r;
}

static void _config_cgroup_name()
{
    char *name = zconfig_get_str(zvar_default_config, "cgroup-name", 0);
    if (zempty(name))
    {
        return;
    }
    if (!zset_cgroup_name(name))
    {
        zinfo("WARNING set cgroup-name error");
    }
}
/* }}} */

/* {{{ _config_path_user */
static void _config_path_user()
{
    char *root = zconfig_get_str(zvar_default_config, "run-chroot", 0);
    char *user = zconfig_get_str(zvar_default_config, "run-user", 0);
    if (zempty(root))
    {
        root = 0;
    }
    if (zempty(user))
    {
        user = 0;
    }
    if ((!zempty(root)) || (!zempty(user)))
    {
        if (zchroot_user(root, user) < 0)
        {
            zfatal("chroot_user(%s, %s): %m", root ? root : "", user ? user : "");
        }
    }

    char *dir = zconfig_get_str(zvar_default_config, "run-chdir", 0);
    if (!zempty(dir))
    {
        if (chdir(dir) == -1)
        {
            zfatal("chdir(%s): %m", dir);
        }
    }
}
/* }}} */

/* {{{ _zmain_argument_do_set_run_config */
void _zmain_argument_do_set_run_config()
{
    _exit_after();
    _fork_concurrency();
    _config_memleak_check();
    _config_core_file_size();
    _config_max_mem();
    _config_cgroup_name();
    _config_path_user();
}
/* }}} */

/*
 * vim600: fdm=marker
 */
#else // __linux__
int zvar_memleak_check = 0;

void _zmain_argument_do_set_run_config()
{
}

#endif // __linux__

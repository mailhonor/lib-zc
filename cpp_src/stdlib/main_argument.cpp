/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-10-12
 * ================================
 */

#include "zcc/zcc_stdlib.h"
#include "zcc/zcc_win64.h"
#ifdef _WIN64
#include <windows.h>
#include <list>
#endif // _WIN64
#ifdef __linux__
#include <fcntl.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#endif // __linux__

zcc_namespace_begin;

const char *progname = "";
bool var_memleak_check_enable = false;
bool var_sigint_flag = false;

#ifdef __linux__
static void ___timeout_do(int pid)
{
    exit(1);
}

static void _exit_after()
{
    int i = var_main_config.get_int("exit-after", 0);
    if (i < 1)
    {
        return;
    }
    ::alarm(0);
    signal(SIGALRM, ___timeout_do);
    alarm(i);
}

static void _fork_concurrency()
{
    /* 主要用于测试 */
    int cc = var_main_config.get_int("fork-concurrency", 0);
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

static void _sigint_handler(int sig)
{
    var_sigint_flag = true;
}

static void _config_memleak_check()
{
    if (!(var_memleak_check_enable = var_main_config.get_bool("memleak-check", var_memleak_check_enable)))
    {
        char *env_ld_preload = getenv("LD_PRELOAD");
        if (env_ld_preload && strstr(env_ld_preload, "vgpreload_memcheck"))
        {
            var_memleak_check_enable = true;
        }
    }
    if (!var_memleak_check_enable)
    {
        return;
    }
    signal(SIGINT, _sigint_handler);
}

static void _config_core_file_size()
{
    int64_t v;
    int64_t vv = var_main_config.get_long("core-file-size", -2);
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
        v = (vv / (1024 * 1024));
    }
    if (!set_core_file_size(v))
    {
        zcc_warning("set core-file-size error");
    }
}

static void _config_max_mem()
{
    int64_t v;
    int64_t vv = var_main_config.get_size("max-memory", -2);
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
    if (!set_max_mem(v))
    {
        zcc_warning("set max-memory error");
    }
}

static void _config_cgroup_name()
{
    const char *name = var_main_config.get_cstring("cgroup-name", 0);
    if (empty(name))
    {
        return;
    }
    if (!set_cgroup_name(name))
    {
        zcc_warning("set cgroup-name error");
    }
}

static void _config_path_user()
{
    const char *root = var_main_config.get_cstring("run-chroot", 0);
    const char *user = var_main_config.get_cstring("run-user", 0);
    if (empty(root))
    {
        root = 0;
    }
    if (empty(user))
    {
        user = 0;
    }
    if ((!empty(root)) || (!empty(user)))
    {
        if (chroot_user(root, user) < 0)
        {
            zcc_fatal("chroot_user(%s, %s): %m", root ? root : "", user ? user : "");
        }
    }

    const char *dir = var_main_config.get_cstring("run-chdir", 0);
    if (!empty(dir))
    {
        if (chdir(dir) == -1)
        {
            zcc_fatal("chdir(%s): %m", dir);
        }
    }
}

static void run_config()
{
    _exit_after();
    _fork_concurrency();
    _config_memleak_check();
    _config_core_file_size();
    _config_max_mem();
    _config_cgroup_name();
    _config_path_user();
}
#endif // __linux__

zcc_general_namespace_begin(main_argument);
int var_argc = 0;
char **var_argv = nullptr;
std::vector<option> var_options;
std::vector<const char *> var_parameters;
int var_parameter_argc = 0;
char **var_parameter_argv = nullptr;

#ifdef _WIN64
std::list<std::string> args_buffer;
#endif // _WIN64

#ifdef _WIN64
static const char *optval_charset_deal(const char *optval, bool cmd_mode)
{
    if (!cmd_mode)
    {
        return optval;
    }
    int need = 0;
    for (unsigned char *p = (unsigned char *)optval; *p; p++)
    {
        if (*p > 127)
        {
            need = 1;
            break;
        }
    }
    if (!need)
    {
        return optval;
    }
    int len = std::strlen(optval);
    int result_size = len * 4 + 16;
    char *result_ptr = (char *)zcc::malloc(result_size + 16);
    int new_len = MultiByteToUTF8(optval, len, result_ptr, result_size);
    if (new_len < 1)
    {
        new_len = 0;
    }
    result_ptr[new_len] = 0;
    args_buffer.push_back(std::string(result_ptr, new_len));
    zcc::free(result_ptr);
    return args_buffer.back().c_str();
}
#else  // _WIN64
static inline const char *optval_charset_deal(const char *optval, bool cmd_mode)
{
    return optval;
}
#endif // _WIN64

static void prepare_config(int _argc, char **_argv, bool cmd_mode)
{
    int i;
    const char *optname, *optval;
    option kvp;
    config cmd_cf;

    progname = _argv[0];
    var_argc = _argc;
    var_argv = _argv;

#ifdef _WIN64
    SetConsoleOutputCP(65001);
#endif // _WIN64

    for (i = 1; i < _argc; i++)
    {
        optname = _argv[i];
        /* abc */
        if ((optname[0] != '-') || (optname[1] == 0))
        {
            optname = optval_charset_deal(optname, cmd_mode);
            var_parameters.push_back(optname);
            continue;
        }

        /* --abc */
        if (optname[1] == '-')
        {
            kvp.key = optname + 2;
            kvp.val = "yes";
            var_options.push_back(kvp);
            cmd_cf.update(optname + 2, "yes");
            continue;
        }

        /* -abc */
        if (i + 1 >= _argc)
        {
            zcc_error("ERROR parameter %s need value", optname);
            exit(1);
        }
        i++;
        optval = _argv[i];
        optval = optval_charset_deal(optval, cmd_mode);
        kvp.key = optname + 1;
        kvp.val = optval;
        var_options.push_back(kvp);
        if (!std::strcmp(optname, "-config"))
        {
            if (!var_main_config.load_from_file(optval))
            {
                zcc_error("ERROR load config error from %s", optval);
                exit(1);
            }
        }
        else
        {
            cmd_cf.update(optname + 1, optval);
        }
    }
    var_main_config.load_another(cmd_cf);

    var_parameters.push_back(nullptr);
    var_parameters.pop_back();
    var_parameter_argc = (int)var_parameters.size();
    var_parameter_argv = (char **)(void *)var_parameters.data();
}

static void do_something(int argc, char **argv, bool cmd_mode)
{
    if (var_main_config.get_bool("debug-config"))
    {
        var_main_config.debug_show();
    }
    logger::var_debug_enable = var_main_config.get_bool("debug", logger::var_debug_enable);
    logger::var_verbose_enable = var_main_config.get_bool("verbose", logger::var_verbose_enable);
    logger::var_fatal_catch = var_main_config.get_bool("fatal-catch", logger::var_fatal_catch);
    logger::var_output_disable = var_main_config.get_bool("log-output-disable", logger::var_output_disable);
}

void run(int _argc, char **_argv, bool cmd_mode)
{
    static bool done_flag = false;
    if (done_flag)
    {
        return;
    }
    done_flag = true;
    prepare_config(_argc, _argv, cmd_mode);
    do_something(_argc, _argv, cmd_mode);
#ifdef __linux__
    run_config();
#endif // __linux__
}
zcc_general_namespace_end(main_argument);

zcc_namespace_end;

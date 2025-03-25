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

/**
 * @brief 存储当前程序的名称
 */
const char *progname = "";
/**
 * @brief 内存泄漏检测功能的开关标志
 */
bool var_memleak_check_enable = false;
/**
 * @brief SIGINT信号触发标志，用于标记是否接收到中断信号
 */
bool var_sigint_flag = false;

#ifdef __linux__
/**
 * @brief 超时处理函数，当定时器触发SIGALRM信号时调用，直接退出程序
 * @param pid 信号处理函数要求的参数，此处未使用
 */
static void ___timeout_do(int pid)
{
    exit(1);
}

/**
 * @brief 根据配置设置程序超时退出机制
 * 从配置中读取 "exit-after" 参数，若该参数大于0，则设置定时器，超时后调用 ___timeout_do 函数退出程序
 */
static void _exit_after()
{
    // 从配置中获取 "exit-after" 参数，默认值为0
    int i = var_main_config.get_int("exit-after", 0);
    if (i < 1)
    {
        return;
    }
    // 清除之前设置的定时器
    ::alarm(0);
    // 注册SIGALRM信号的处理函数为 ___timeout_do
    signal(SIGALRM, ___timeout_do);
    // 设置新的定时器，i秒后触发SIGALRM信号
    alarm(i);
}

/**
 * @brief 并发fork测试函数，用于压力测试
 * 从配置中读取 "fork-concurrency" 参数，创建指定数量的子进程，并等待子进程退出
 */
static void _fork_concurrency()
{
    /* 主要用于测试 */
    // 从配置中获取 "fork-concurrency" 参数，默认值为0
    int cc = var_main_config.get_int("fork-concurrency", 0);
    if (cc < 1)
    {
        return;
    }

    // 创建指定数量的子进程
    for (int ci = 0; ci < cc; ci++)
    {
        // fork函数返回0表示子进程
        if (!fork())
        {
            return;
        }
    }

    int status;
    // 父进程等待子进程退出
    while (wait(&status) > 0)
    {
        cc--;
        if (cc == 0)
        {
            // 所有子进程正常退出，父进程退出
            _exit(0);
        }
    }
    // 有子进程异常退出，父进程退出
    _exit(1);
}

/**
 * @brief SIGINT信号处理函数，设置中断标志
 * @param sig 接收到的信号编号
 */
static void _sigint_handler(int sig)
{
    var_sigint_flag = true;
}

/**
 * @brief 配置内存泄漏检测功能
 * 从配置或环境变量中判断是否启用内存泄漏检测功能，若启用则注册SIGINT信号处理函数
 */
static void _config_memleak_check()
{
    // 从配置中获取 "memleak-check" 参数，更新内存泄漏检测开关标志
    if (!(var_memleak_check_enable = var_main_config.get_bool("memleak-check", var_memleak_check_enable)))
    {
        // 获取环境变量 LD_PRELOAD 的值
        char *env_ld_preload = getenv("LD_PRELOAD");
        if (env_ld_preload && strstr(env_ld_preload, "vgpreload_memcheck"))
        {
            // 如果 LD_PRELOAD 中包含 "vgpreload_memcheck"，则启用内存泄漏检测
            var_memleak_check_enable = true;
        }
    }
    if (!var_memleak_check_enable)
    {
        return;
    }
    // 注册SIGINT信号的处理函数为 _sigint_handler
    signal(SIGINT, _sigint_handler);
}

/**
 * @brief 配置核心转储文件的大小限制
 * 从配置中读取 "core-file-size" 参数，设置核心转储文件的大小限制
 */
static void _config_core_file_size()
{
    int64_t v;
    // 从配置中获取 "core-file-size" 参数，默认值为 -2
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
        // 将字节数转换为MB
        v = (vv / (1024 * 1024));
    }
    if (!set_core_file_size(v))
    {
        // 设置失败，输出警告信息
        zcc_warning("set core-file-size error");
    }
}

/**
 * @brief 配置程序的最大内存限制
 * 从配置中读取 "max-memory" 参数，设置程序的最大内存限制
 */
static void _config_max_mem()
{
    int64_t v;
    // 从配置中获取 "max-memory" 参数，默认值为 -2
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
        // 将字节数转换为MB
        v = (int)(vv / (1024 * 1024));
    }
    if (!set_max_mem(v))
    {
        // 设置失败，输出警告信息
        zcc_warning("set max-memory error");
    }
}

/**
 * @brief 配置cgroup名称
 * 从配置中读取 "cgroup-name" 参数，设置cgroup名称
 */
static void _config_cgroup_name()
{
    // 从配置中获取 "cgroup-name" 参数，默认值为0
    const char *name = var_main_config.get_cstring("cgroup-name", 0);
    if (empty(name))
    {
        return;
    }
    if (!set_cgroup_name(name))
    {
        // 设置失败，输出警告信息
        zcc_warning("set cgroup-name error");
    }
}

/**
 * @brief 配置程序的运行根目录、用户和工作目录
 * 从配置中读取 "run-chroot"、"run-user" 和 "run-chdir" 参数，设置程序的运行环境
 */
static void _config_path_user()
{
    // 从配置中获取 "run-chroot" 参数，默认值为0
    const char *root = var_main_config.get_cstring("run-chroot", 0);
    // 从配置中获取 "run-user" 参数，默认值为0
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
            // 设置失败，输出致命错误信息并退出程序
            zcc_fatal("chroot_user(%s, %s): %m", root ? root : "", user ? user : "");
        }
    }

    // 从配置中获取 "run-chdir" 参数，默认值为0
    const char *dir = var_main_config.get_cstring("run-chdir", 0);
    if (!empty(dir))
    {
        if (chdir(dir) == -1)
        {
            // 切换工作目录失败，输出致命错误信息并退出程序
            zcc_fatal("chdir(%s): %m", dir);
        }
    }
}

/**
 * @brief 按顺序执行各项配置函数
 */
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
/**
 * @brief 存储命令行参数的数量
 */
int var_argc = 0;
/**
 * @brief 存储命令行参数的指针数组
 */
char **var_argv = nullptr;
/**
 * @brief 存储解析后的命令行选项
 */
std::vector<option> var_options;
/**
 * @brief 存储解析后的命令行参数
 */
std::vector<const char *> var_parameters;
/**
 * @brief 存储解析后的命令行参数的数量
 */
int var_parameter_argc = 0;
/**
 * @brief 存储解析后的命令行参数的指针数组
 */
char **var_parameter_argv = nullptr;

#ifdef _WIN64
/**
 * @brief 用于存储Windows系统下转换后的字符串
 */
std::list<std::string> args_buffer;
#endif // _WIN64

#ifdef _WIN64
/**
 * @brief 处理Windows系统下命令行参数的字符编码，将非ASCII字符转换为UTF-8编码
 * @param optval 待处理的命令行参数值
 * @param cmd_mode 是否为命令行模式
 * @return 处理后的命令行参数值
 */
static const char *optval_charset_deal(const char *optval, bool cmd_mode)
{
    if (!cmd_mode)
    {
        return optval;
    }
    int need = 0;
    // 检查是否包含非ASCII字符
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
    // 将多字节字符串转换为UTF-8编码
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
/**
 * @brief 非Windows系统下，直接返回原始的命令行参数值
 * @param optval 待处理的命令行参数值
 * @param cmd_mode 是否为命令行模式
 * @return 原始的命令行参数值
 */
static inline const char *optval_charset_deal(const char *optval, bool cmd_mode)
{
    return optval;
}
#endif // _WIN64

/**
 * @brief 解析命令行参数，更新配置信息
 * @param _argc 命令行参数的数量
 * @param _argv 命令行参数的指针数组
 * @param cmd_mode 是否为命令行模式
 */
static void prepare_config(int _argc, char **_argv, bool cmd_mode)
{
    int i;
    const char *optname, *optval;
    option kvp;
    config cmd_cf;

    // 记录程序名称
    progname = _argv[0];
    var_argc = _argc;
    var_argv = _argv;

#ifdef _WIN64
    // 设置控制台输出编码为UTF-8
    SetConsoleOutputCP(65001);
#endif // _WIN64

    for (i = 1; i < _argc; i++)
    {
        optname = _argv[i];
        /* abc */
        if ((optname[0] != '-') || (optname[1] == 0))
        {
            // 处理非选项参数
            optname = optval_charset_deal(optname, cmd_mode);
            var_parameters.push_back(optname);
            continue;
        }

        /* --abc */
        if (optname[1] == '-')
        {
            // 处理双横线开头的选项
            kvp.key = optname + 2;
            kvp.val = "yes";
            var_options.push_back(kvp);
            cmd_cf.update(optname + 2, "yes");
            continue;
        }

        /* -abc */
        if (i + 1 >= _argc)
        {
            // 缺少选项值，输出错误信息并退出程序
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
            // 处理 -config 选项，加载配置文件
            if (!var_main_config.load_from_file(optval))
            {
                // 加载配置文件失败，输出错误信息并退出程序
                zcc_error("ERROR load config error from %s", optval);
                exit(1);
            }
        }
        else
        {
            // 更新临时配置信息
            cmd_cf.update(optname + 1, optval);
        }
    }
    // 将临时配置信息合并到主配置中
    var_main_config.load_another(cmd_cf);

    var_parameters.push_back(nullptr);
    var_parameters.pop_back();
    var_parameter_argc = (int)var_parameters.size();
    var_parameter_argv = (char **)(void *)var_parameters.data();
}

/**
 * @brief 根据配置信息设置日志系统的相关参数
 * @param argc 命令行参数的数量
 * @param argv 命令行参数的指针数组
 * @param cmd_mode 是否为命令行模式
 */
static void do_something(int argc, char **argv, bool cmd_mode)
{
    if (var_main_config.get_bool("debug-config"))
    {
        // 输出配置信息用于调试
        var_main_config.debug_show();
    }
    // 根据配置更新日志系统的相关标志
    logger::var_debug_enable = var_main_config.get_bool("debug", logger::var_debug_enable);
    logger::var_verbose_enable = var_main_config.get_bool("verbose", logger::var_verbose_enable);
    logger::var_fatal_catch = var_main_config.get_bool("fatal-catch", logger::var_fatal_catch);
    logger::var_output_disable = var_main_config.get_bool("log-output-disable", logger::var_output_disable);
}

/**
 * @brief 程序的主运行函数，负责解析参数、应用配置和执行系统配置
 * @param _argc 命令行参数的数量
 * @param _argv 命令行参数的指针数组
 * @param cmd_mode 是否为命令行模式
 */
void run(int _argc, char **_argv, bool cmd_mode)
{
    static bool done_flag = false;
    if (done_flag)
    {
        return;
    }
    done_flag = true;
    // 解析命令行参数，更新配置信息
    prepare_config(_argc, _argv, cmd_mode);
    // 根据配置信息设置日志系统的相关参数
    do_something(_argc, _argv, cmd_mode);
#ifdef __linux__
    // 执行Linux系统下的各项配置
    run_config();
#endif // __linux__
}
zcc_general_namespace_end(main_argument);

// 退出自定义命名空间
zcc_namespace_end;

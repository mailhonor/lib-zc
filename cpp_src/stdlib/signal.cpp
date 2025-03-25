/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2020-07-15
 * ================================
 */
#include "zcc/zcc_stdlib.h"
#include <signal.h>

zcc_namespace_begin;

/**
 * @brief 注册信号处理函数，根据不同的操作系统采用不同的实现方式。
 * 
 * 在 Linux 系统下，使用 sigaction 函数来注册信号处理函数，它提供了比 signal 函数更丰富的功能和更精确的控制。
 * 在非 Linux 系统下，直接使用标准库的 signal 函数来注册信号处理函数。
 * 
 * @param signum 要捕获的信号编号，例如 SIGINT、SIGTERM 等。
 * @param handler 信号处理函数的指针，当指定信号发生时会调用该函数。如果设置为 SIG_IGN，则忽略该信号；如果设置为 SIG_DFL，则使用默认处理方式。
 */
void signal(int signum, void (*handler)(int))
{
#ifdef __linux__
    // 定义 sigaction 结构体，用于设置信号处理的行为
    struct sigaction action, old;
    // 清空 action 结构体中的信号掩码，确保在信号处理函数执行期间不会屏蔽其他信号
    ::sigemptyset(&action.sa_mask);
    // 清除标志位，使用默认的信号处理选项
    action.sa_flags = 0;
    // 指定信号处理函数
    action.sa_handler = handler;

    // 清空 old 结构体，用于保存旧的信号处理设置
    std::memset(&old, 0, sizeof(struct sigaction));
    // 清空 old 结构体中的信号掩码
    ::sigemptyset(&old.sa_mask);

    // 调用 sigaction 函数设置新的信号处理行为，并保存旧的设置
    if (::sigaction(signum, &action, &old) < 0)
    {
        // 如果设置失败，输出致命错误信息并终止程序
        zcc_fatal("sigaction: %m");
    }
#else  // __linux__
    // 在非 Linux 系统下，直接使用标准库的 signal 函数注册信号处理函数
    ::signal(signum, handler);
#endif // __linux__
}

/**
 * @brief 忽略指定的信号，将信号处理方式设置为 SIG_IGN。
 * 
 * 调用 signal 函数，将指定信号的处理函数设置为 SIG_IGN，从而忽略该信号。
 * 
 * @param signum 要忽略的信号编号，例如 SIGINT、SIGPIPE 等。
 */
void signal_ignore(int signum)
{
    // 调用 signal 函数将指定信号的处理方式设置为忽略
    signal(signum, SIG_IGN);
}

zcc_namespace_end;

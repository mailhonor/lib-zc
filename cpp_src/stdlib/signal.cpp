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

void signal(int signum, void (*handler)(int))
{
#ifdef __linux__
    struct sigaction action, old;
    ::sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    action.sa_handler = handler;
    std::memset(&old, 0, sizeof(struct sigaction));
    ::sigemptyset(&old.sa_mask);
    if (::sigaction(signum, &action, &old) < 0)
    {
        zcc_fatal("sigaction: %m");
    }
#else  // __linux__
    ::signal(signum, handler);
#endif // __linux__
}

void signal_ignore(int signum)
{
    signal(signum, SIG_IGN);
}

zcc_namespace_end;

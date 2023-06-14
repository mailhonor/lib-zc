/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2020-07-15
 * ================================
 */
#include "zc.h"
#include <signal.h>

zsighandler_t zsignal(int signum, zsighandler_t handler)
{
#ifdef __linux__
    struct sigaction action, old;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    action.sa_handler = handler;
    memset(&old, 0, sizeof(struct sigaction));
    sigemptyset(&old.sa_mask);
    if (sigaction(signum, &action, &old) < 0)
    {
        zfatal("sigaction: %m");
    }
    return old.sa_handler;
#else  // __linux__
    return signal(signum, handler);
#endif // __linux__
}

zsighandler_t zsignal_ignore(int signum)
{
#ifdef __linux__
    struct sigaction action, old;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    action.sa_handler = SIG_IGN;
    memset(&old, 0, sizeof(struct sigaction));
    sigemptyset(&old.sa_mask);
    if (sigaction(signum, &action, &old) < 0)
    {
        zfatal("sigaction: %m");
    }
    return old.sa_handler;
#else // __linux__
    return signal(signum, SIG_IGN);
#endif // __linux__
}

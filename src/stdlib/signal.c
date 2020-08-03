/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2020-07-15
 * ================================
 */
#include "zc.h"
#include <signal.h>

void zsignal(int signum, void (*handler)(int))
{
    struct sigaction action;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    action.sa_handler = handler;
    if (sigaction(SIGTERM, &action, (struct sigaction *) 0) < 0){
        zfatal("FATAL sigaction: %m");
    }
}

void zsignal_ignore(int signum)
{
    struct sigaction action;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    action.sa_handler = SIG_IGN;
    if (sigaction(SIGTERM, &action, (struct sigaction *) 0) < 0){
        zfatal("FATAL sigaction: %m");
    }
}

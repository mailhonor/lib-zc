/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-11-24
 * ================================
 */

#include "zcc/zcc_server.h"
#include "zcc/zcc_stream.h"
#include "zc_coroutine.h"
#include <errno.h>

class my_server : public zcc::coroutine_worker_server
{
public:
    void service_register(const char *service_name, int fd, int fd_type);
};

my_server ms;

static void *do_echo(void *arg)
{
    int fd = ZCC_PTR_TO_NUMBER(arg);
    zcc::iostream fp(fd);

    fp.append("welcome coroutine, support exit, EXIT, DETACH\n");
    fp.flush();

    std::string line;
    while (!fp.is_exception())
    {
        line.clear();
        if (fp.gets(line, 102400) < 1)
        {
            break;
        }
        zcc::trim_line_end_rn(line);
        fp.append(line).append("\n");
        fp.flush();

        if (line == "exit")
        {
            break;
        }
        if (line == "EXIT")
        {
            ms.stop_notify();
            break;
        }
        if (line == "DETACH")
        {
            ms.detach_from_master();
            continue;
        }
    }
    fp.flush();
    fp.close();
    return 0;
}

static void *do_accept(void *arg)
{
    int sock = ZCC_PTR_TO_NUMBER(arg);
    while (1)
    {
        int fd = zcc::inet_accept(sock);
        if (fd < 0)
        {
            if (zcc::var_sigint_flag)
            {
                break;
            }
            if (errno == EAGAIN)
            {
                continue;
            }
            if (errno == EINTR)
            {
                continue;
            }
            zcc_fatal("accept(%m)");
        }
        zcoroutine_go(do_echo, ZCC_NUMBER_TO_PTR(fd), -1);
    }
    return 0;
}

void my_server::service_register(const char *service_name, int fd, int fd_type)
{
    zcoroutine_go(do_accept, ZCC_NUMBER_TO_PTR(fd), -1);
}

int main(int argc, char **argv)
{
    ms.main_run(argc, argv);
    return 0;
}

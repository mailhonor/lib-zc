/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2024-04-27
 * ================================
 */

#include "zcc/zcc_stdlib.h"
#include "zcc/zcc_stream.h"

const char *server = "";
static int listenfd = -1;

static void usage()
{
    zcc_error_and_exit("%s [ -server 127.0.0.1:8899 ]", zcc::progname);
    zcc::exit(1);
}

static bool do_accept()
{
    int fd = zcc::inet_accept(listenfd);
    if (fd < 0)
    {
        return true;
    }
    bool quit = false;
    zcc::iostream fp;
    fp.open_socket(fd);
    while (1)
    {
        std::string r;
        int ret = fp.gets(r, 10240);
        if (ret < 1)
        {
            break;
        }
        zcc::trim_line_end_rn(r);
        fp.append(zcc::rfc1123_time()).append(" ").append(r).append("\n");
        fp.flush();
        if ((r == "exit") || r == "quit")
        {
            break;
        }
        if ((r == "EXIT") || r == "QUIT")
        {
            quit = true;
            break;
        }
    }
    fp.close(true);
    return (!quit);
}

int main(int argc, char **argv)
{
    zcc::main_argument::run(argc, argv);
    server = zcc::var_main_config.get_cstring("server", "127.0.0.1:8899");
    listenfd = zcc::netpath_listen(server, 128);
    if (listenfd < 0)
    {
        zcc_error("listen %s", server);
        usage();
    }

    while (do_accept())
        ;
    zcc::close_socket(listenfd);
    return 0;
}
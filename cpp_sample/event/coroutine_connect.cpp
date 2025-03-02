/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2024-10-10
 * ================================
 */

#pragma GCC diagnostic ignored "-Wunused-result"
#include "zc_coroutine.h"
#include "zcc/zcc_imap.h"
#include <signal.h>
#include <unistd.h>

static int cc = 0;
static int times = 0;
static std::string ip;
static int port = 0;

static int64_t r_success = 0;
static int64_t r_error = 0;

static void init()
{
    cc = zcc::var_main_config.get_int("cc", 10);
    times = zcc::var_main_config.get_int("times", 10);
    ip = zcc::var_main_config.get_string("ip", "127.0.0.1");
    port = zcc::var_main_config.get_int("port", 80);
}

static void *do_console(void *)
{
    while (1)
    {
        zcc::sleep(1);
        std::fprintf(stderr, "success: %zd, error: %zd\n", r_success, r_error);
    }
}

static void do_test_one()
{
    int fd = zcc::inet_connect(ip.c_str(), port, 0);
    if (fd < 0)
    {
        std::fprintf(stderr, "sss: %m\n");
        r_error++;
        return;
    }
    char buf[128 + 1];
    ::write(fd, (const void *)"GET a\r\n", 7);
    ::read(fd, buf, 128);
    zcc::close(fd);
    r_success++;
}

static void *do_test(void *)
{
    for (int i = 0; i < times; i++)
    {
        do_test_one();
    }
    return nullptr;
}

int main(int argc, char **argv)
{
    zcc_info("%s -cc 10 -times 10 -ip ip -port port", zcc::progname);
    zcc::signal_ignore(SIGPIPE);
    zcc::main_argument::run(argc, argv);
    init();
    zcoroutine_base_init();

    zcoroutine_go(do_console, 0, 16);
    for (int i = 0; i < cc; i++)
    {
        zcoroutine_go(do_test, 0, 64);
    }

    zcoroutine_base_run();
    zcoroutine_base_fini();
    return 0;
}
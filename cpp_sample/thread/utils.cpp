/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2021-03-19
 * ================================
 */

#include "zcc/zcc_thread.h"

static void usage(void)
{
    zcc::exit(1);
}

static void worker()
{
    zcc::sleep(5);
    std::string name = zcc::get_thread_name();
    zcc_info("inner thread name: %s", name.c_str());
    zcc::sleep(100);
}

int main(int argc, char **argv)
{
    zcc::main_argument::run(argc, argv);

    {
        std::thread t(worker);
        zcc::set_thread_name(t, "worker1");
        std::string name = zcc::get_thread_name(t);
        zcc_info("thread name: %s", name.c_str());
        t.detach();
    }
    zcc::sleep(100);
    return 0;
}

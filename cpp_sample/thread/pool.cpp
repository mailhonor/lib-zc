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
    std::printf("USAGE: %s thread_count\n", zcc::progname);
    zcc::exit(1);
}

static void job1(int i, zcc::thread_pool *pool)
{
    std::printf("job1 start, %d\n", i);
    zcc::sleep_millisecond(1000);
    std::printf("job1 end, %d\n", i);
}

static void timer1(int i, zcc::thread_pool *pool)
{
    std::printf("timer1, %d\n", i);
}

int main(int argc, char **argv)
{
    zcc::main_argument::run(argc, argv);
    if (zcc::main_argument::var_parameters.size() < 1)
    {
        usage();
    }
    int count = atoi(zcc::main_argument::var_parameters[0]);

    zcc::thread_pool tp;
    tp.set_debug();
    for (int i = 0; i < count; i++)
    {
        tp.create_one_thread();
    }
    for (int i = 0; i < 100; i++)
    {
        tp.enter_task(std::bind(job1, i, &tp));
    }

    for (int i = 0; i < 100; i++)
    {
        tp.enter_timer(std::bind(timer1, i, &tp), i / 10 + 1);
    }

    zcc::sleep(5);
    zcc::sleep(10 + 1);
    std::printf("wait idle pthread quit\n");
    std::printf("soft stop\n");
    zcc::sleep(1 + 1);
    tp.softstop();
    tp.wait_all_stopped(2);

    return 0;
}

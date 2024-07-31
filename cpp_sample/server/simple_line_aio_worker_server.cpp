/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-11-24
 * ================================
 */

#include "zcc/zcc_server.h"

class my_server : public zcc::simple_line_aio_worker_server
{
public:
    void handler(simple_line_aio &slr);
    void before_service();
};

my_server ms;

void my_server::handler(simple_line_aio &slr)
{
    zcc_info("CMD: %s", slr.cmd_name_.c_str());
    std::string str;
    slr.json_.serialize(str, zcc::json_serialize_pretty);
    zcc_info("REQUEST: %s", str.c_str());
    slr.cache_append("OK\n");
    slr.cache_flush_and_request();
}

static void simple_timedo()
{
    if (zcc::var_sigint_flag)
    {
        zcc::aio_worker_server::get_instance()->stop_notify();
        return;
    }
    zcc::var_main_aio_base->enter_timer(simple_timedo, 1);
}

void my_server::before_service()
{
    simple_line_aio_worker_server::before_service();
    ms.enter_timer(simple_timedo, 1);
}

int main(int argc, char **argv)
{
    ms.main_run(argc, argv);
}
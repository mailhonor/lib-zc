/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2024-04-27
 * ================================
 */

#include "zcc/zcc_stdlib.h"

static void test_one(std::string s, std::string tzid)
{
    int64_t t;

    t = zcc::iso8601_2004_time_from_time(s, tzid);
    zcc_info("onlytime: %zd (maybe false)", t);

    t = zcc::iso8601_2004_time_from_datetime(s, tzid);
    zcc_info("datetime: %zd, %s", t, zcc::simple_date_time_with_second(t).c_str());
}

int main(int argc, char **argv)
{
    zcc::main_argument::run(argc, argv);
    if (zcc::main_argument::var_parameter_argc < 1)
    {
        zcc_info("Usage: %s timestring [ default-tzid ]", argv[0]);
        return 0;
    }
    std::string s = zcc::main_argument::var_parameter_argv[0];
    std::string tzid;
    if (zcc::main_argument::var_parameter_argc > 1)
    {
        tzid = zcc::main_argument::var_parameter_argv[1];
    }
    test_one(s, tzid);

    return 0;
}
/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2024-04-27
 * ================================
 */

#include "zcc/zcc_stdlib.h"

int main(int argc, char **argv)
{
    int64_t t;
    zcc::main_argument::run(argc, argv);
    t = zcc::millisecond();
    zcc_info("millisecond: %zd.%03zd", t / 1000, t % 1000);
    zcc_info("sleep 3000ms");
    zcc::sleep_millisecond(3000);
    t = zcc::millisecond();
    zcc_info("millisecond: %zd.%03zd", t / 1000, t % 1000);

    std::string s = zcc::rfc1123_time();
    zcc_info("rfc1123_time: %s", s.c_str());
    s = zcc::rfc822_time();
    zcc_info("rfc822_time: %s", s.c_str());

    return 0;
}
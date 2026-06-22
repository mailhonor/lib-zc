/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2026-04-10
 * ================================
 */

#include "zcc/zcc_stdlib.h"

int main(int argc, char **argv)
{
    zcc::main_argument::run(argc, argv);
    int yyyyymmdd;
    if (zcc::main_argument::var_parameters.size() < 1)
    {
        zcc_fatal("USAGE: %s yyyyymmdd", zcc::progname);
        yyyyymmdd = zcc::get_yyyyymmdd();
    }
    else
    {
        yyyyymmdd = std::stoi(zcc::main_argument::var_parameters[0]);
    }
    auto info = zcc::get_lunar_date(yyyyymmdd);
    zcc_info("yyyyymmdd: %d", yyyyymmdd);
    zcc_info("月日: %s%s, %s", info.month_name.c_str(), info.is_leap_month ? "(闰)" : "", info.day_name.c_str());
    zcc_info("节日: %s", info.festival_name.empty() ? "-" : info.festival_name.c_str());
    zcc_info("天干: %s%s年", info.year_tiangan.c_str(), info.year_dizhi.c_str());
    zcc_info("生肖: %s", info.zodiac_name.c_str());
    return 0;
}
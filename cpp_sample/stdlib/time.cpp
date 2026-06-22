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
    zcc_info("sleep 356ms");
    zcc::sleep_millisecond(356);
    t = zcc::millisecond();
    zcc_info("millisecond: %zd.%03zd", t / 1000, t % 1000);

    //
    std::string s = zcc::http_time();
    zcc_info("http_time: %s", s.c_str());
    s = zcc::rfc822_time();
    zcc_info("mail_time: %s", s.c_str());
    //
    auto tz = zcc::var_main_config.get_string("tz");
    if (!tz.empty())
    {
        struct tm t;
        if (!zcc::gmtime_with_timezone(zcc::second(), tz, &t))
        {
            zcc_info("gmtime_with_timezone failed, maybe tz is invalid");
        }
        zcc_info("gmtime_with_timezone;TZ=%s, year=%d, month=%d, day=%d, hour=%d, minute=%d, second=%d, weekday=%d", tz.c_str(), t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, t.tm_wday);
    }
    else
    {
        zcc_info("tz is empty");
    }

    //
    zcc_info("test day_to_unix: 20260409");
    int64_t unix_second = zcc::day_to_unix(20260409);
    zcc_info("day_to_unix: %zd, %zd", 20260409, unix_second);

    return 0;
}
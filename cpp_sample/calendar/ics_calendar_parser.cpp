/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2025-08-14
 * ================================
 */

#include "zcc/zcc_begin_end_text.h"
#include "zcc/zcc_vcf.h"
#include "zcc/zcc_calendar.h"

int main(int argc, char **argv)
{
    zcc::main_argument::run(argc, argv);
    if (zcc::main_argument::var_parameter_argc < 1)
    {
        zcc_error("USAGE: %s filename [ -match-day 20360407 [ -match-tzid Asia/Shanghai ] ] ", zcc::progname);
        zcc::exit(1);
        return 1;
    }
    auto filename = zcc::main_argument::var_parameters[0];
    auto data = zcc::file_get_contents_sample(filename);
    zcc::ics_calendar calendar;
    calendar.parse(data);
    zcc_info("%s", calendar.debug_info().c_str());
    auto match_day = zcc::var_main_config.get_string("match-day");
    auto match_tzid = zcc::var_main_config.get_string("match-tzid");
    if (!match_day.empty())
    {
        if (match_day.size() != 8)
        {
            zcc_error("match-day must be 8 digits like 20260407");
            zcc::exit(1);
        }

        for (auto &event : calendar.events_)
        {
            zcc::ics_calendar_event_matcher matcher;
            matcher.load_from_event(event);
            if (matcher.match_day(zcc::atoi(match_day), match_tzid))
            {
                zcc_info("match: %s", match_day.c_str());
            }
            else
            {
                zcc_info("not match: %s", match_day.c_str());
            }
        }
    }

    return 0;
}

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
        zcc_error("USAGE: %s filename", zcc::progname);
        zcc::exit(1);
        return 1;
    }
    auto filename = zcc::main_argument::var_parameters[0];
    auto data = zcc::file_get_contents_sample(filename);
    zcc::ics_calendar calendar;
    calendar.parse(data);
    zcc_info("%s", calendar.debug_info().c_str());

    auto ics_data = calendar.serialize_to_ics_string();
    zcc_info("serialize_to_ics_string:\n%s", ics_data.c_str());
    zcc::file_put_contents("./output_test.ics", ics_data);

    return 0;
}

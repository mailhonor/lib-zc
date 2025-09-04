/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2025-08-14
 * ================================
 */

#include "zcc/zcc_begin_end_text.h"
#include "zcc/zcc_vcf.h"
#include "zcc/zcc_ics.h"

int main(int argc, char **argv)
{
    zcc::main_argument::run(argc, argv);
    if (zcc::main_argument::var_parameter_argc < 1)
    {
        zcc_error("USAGE: %s [ -type vcf|ics ] filename", zcc::progname);
        zcc::exit(1);
        return 1;
    }
    auto filename = zcc::main_argument::var_parameters[0];
    auto data = zcc::file_get_contents_sample(filename);

    std::string type = zcc::var_main_config.get_string("type");
    if (type == "vcf")
    {
        zcc::vcf_contact contact;
        contact.parse(data);
        zcc_info("%s", contact.debug_info().c_str());
    }
    else if (type == "ics")
    {
    }
    else
    {
        auto node = zcc::begin_end_text_node::parse(data);
        zcc_info("%s", node->debug_info().c_str());
        delete node;
    }

    return 0;
}

/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2016-12-09
 * ================================
 */

#include "zcc/zcc_charset.h"

static void ___usage()
{
    zcc_fatal("USAGE: %s filename some-width", zcc::progname);
}

int main(int argc, char **argv)
{
    char *fn;
    int len = 0;
    zcc::main_argument::run(argc, argv);

    if (zcc::main_argument::var_parameter_argc < 2)
    {
        ___usage();
    }

    fn = zcc::main_argument::var_parameter_argv[0];
    len = std::atoi(zcc::main_argument::var_parameter_argv[1]);

    std::string con = zcc::file_get_contents(fn);

    std::string r = zcc::charset::utf8_get_simple_digest(con, len);
    zcc_info("result: %s", r.c_str());

    return 0;
}

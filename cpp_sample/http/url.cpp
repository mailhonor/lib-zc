/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2017-07-23
 * ================================
 */

#include "zcc/zcc_http.h"

int main(int argc, char **argv)
{
    zcc::main_argument::run(argc, argv);

    if (zcc::main_argument::var_parameters.size() == 0)
    {
        std::printf("USAGE: %s http_url_string\n", zcc::progname);
        zcc::exit(1);
    }
    zcc::http_url url(zcc::main_argument::var_parameters[0]);
    url.debug_show();

    std::string n = url.build_url();
    std::printf("\nNEW URL 1:\n%s\n", n.c_str());

    url.query_ = "";
    n = url.build_url();
    std::printf("\nNEW URL 2:\n%s\n", n.c_str());

    return 0;
}

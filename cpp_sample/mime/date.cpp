/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2023-12-17
 * ================================
 */

#include "zcc/zcc_mime.h"

static void usage()
{
    zcc_fatal("USAGE: %s rf822_date_string", zcc::progname);
}

int main(int argc, char **argv)
{
    zcc::main_argument::run(argc, argv);
    if (zcc::main_argument::var_parameters.size() == 0)
    {
        usage();
    }
    const char *ds = zcc::main_argument::var_parameters[0];
    int64_t u = zcc::mail_parser::decode_date(ds);
    zcc_info("unix time: %zd", u);
    std::string nd = zcc::rfc822_time(u);
    zcc_info("input: %s", ds);
    zcc_info("date : %s", nd.c_str());
    return 0;
}

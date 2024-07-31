/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2021-03-31
 * ================================
 */

#include "zcc/zcc_search.h"

static void usage()
{
    zcc_error_and_exit("USAGE: %s keyword_db_file", zcc::progname);
}

int main(int argc, char **argv)
{
    zcc::main_argument::run(argc, argv);
    if (zcc::main_argument::var_parameters.size() != 1)
    {
        usage();
    }
    const char *dbfn = zcc::main_argument::var_parameters[0];

    zcc::msearch_reader reader;
    if (reader.load_from_file(dbfn) < 1)
    {
        zcc_error_and_exit("can not open %s", dbfn);
    }
    zcc::msearch_walker walker(reader);
    const char *token;
    int tlen;
    while (walker.walk(&token, &tlen) > 0)
    {
        std::string s(token, tlen);
        zcc_info("key: %s", s.c_str());
    }
    return 0;
}

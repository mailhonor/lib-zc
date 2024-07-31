/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2020-07-29
 * ================================
 */

#include "zcc/zcc_search.h"

static void usage()
{
    zcc_error_and_exit("USAGE: %s keyword_list_file keyword_db_file", zcc::progname);
}

int main(int argc, char **argv)
{
    zcc::main_argument::run(argc, argv);
    if (zcc::main_argument::var_parameters.size() != 2)
    {
        usage();
    }
    const char *listfn = zcc::main_argument::var_parameters[0];
    const char *dbfn = zcc::main_argument::var_parameters[1];

    zcc::msearch_builder builder;
    builder.add_token_from_file(listfn);
    builder.add_over();
    if (zcc::file_put_contents(dbfn, builder.get_compiled_data(), builder.get_compiled_size()) < 1)

    {
        zcc_error_and_exit("write %s\n", dbfn);
    }
    
    return 0;
}

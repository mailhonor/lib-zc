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
    zcc_error_and_exit("USAGE: %s keyword_db_file file_for_search", zcc::progname);
}

int main(int argc, char **argv)
{
    zcc::main_argument::run(argc, argv);
    if (zcc::main_argument::var_parameters.size() != 2)
    {
        usage();
    }
    const char *dbfn = zcc::main_argument::var_parameters[0];
    const char *textfn = zcc::main_argument::var_parameters[1];

    zcc::msearch_reader reader;
    if (zcc::msearch_reader::is_my_file(dbfn))
    {
        if (reader.load_from_file(dbfn) < 1)
        {
            zcc_error_and_exit("can not open %s", dbfn);
        }
    }
    else
    {
        if (reader.add_token_from_file(dbfn) < 1)
        {
            zcc_error_and_exit("can not open %s", dbfn);
        }
        reader.add_over();
    }

    std::string con = zcc::file_get_contents_sample(textfn);
    int offset;
    const char *result;
    int len = reader.match(con.c_str(), con.size(), &result, &offset);
    if (len < 1)
    {
        zcc_info("NOT FOUND");
    }
    else
    {
        std::string s(result, offset);
        zcc_info("FOUND: %s", s.c_str());
    }

    return 0;
}

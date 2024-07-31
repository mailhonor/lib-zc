/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-10-12
 * ================================
 */

#include "zcc/zcc_buffer.h"


int main(int argc, char **argv)
{
    zcc::main_argument::run(argc, argv);
    if (zcc::main_argument::var_parameters.size() < 1)
    {
        zcc_fatal("USAGE: %s bin_file", zcc::progname);
    }
    std::string old_con, new_con;
    zcc::file_get_contents_sample(zcc::main_argument::var_parameters[0], old_con);
    zcc::buffer buffer;
    for (uint64_t i = 0; i < old_con.size(); i++)
    {
        buffer.putc(old_con[i]);
    }
    new_con.append(buffer.get_data(), buffer.get_len());

    if (old_con == new_con)
    {
        zcc_info("OK");
    }
    else
    {
        zcc_info("ERR");
    }
    return 0;
}
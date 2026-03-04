/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2024-07-31
 * ================================
 */

#include "zcc/zcc_stdlib.h"

static void cmdname()
{
    std::string r = zcc::get_cmd_pathname();
    zcc_info("cmd pathname: %s", r.c_str());
    r = zcc::get_cmd_name();
    zcc_info("cmd name: %s", r.c_str());
}

int main(int argc, char **argv)
{
    zcc::main_argument::run(argc, argv);
    cmdname();
    return 0;
}

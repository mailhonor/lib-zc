/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2023-06-13
 * ================================
 */

#include "zcc/zcc_stdlib.h"

// #ifdef ZCC_MSVC
// #pragma comment(lib, "Kernel32.lib")
// #endif // ZCC_MSVC

int main(int argc, char **argv)
{
    const char *fn = 0;
    zcc::main_argument::run(argc, argv);
    if (zcc::main_argument::var_parameters.size() < 1)
    {
        zcc_fatal("USAGE: %s [ dirname ]", zcc::progname);
    }
    fn = zcc::main_argument::var_parameters[0];

    zcc::mkdir(0777, "中文文件名", "子文件夹", 0);
    if (zcc::file_exists(fn) < 1)
    {
        if (zcc::mkdir(fn, 0777) < 0)
        {
            zcc_info("ERR mkdir: %s", fn);
        }
        else
        {
            zcc_info("OK mkdir: %s", fn);
        }
    }
    else
    {
        if (zcc::rmdir(fn) < 0)
        {
            zcc_info("ERR rmdir: %s", fn);
        }
        else
        {
            zcc_info("OK rmdir: %s", fn);
        }
    }
    return 0;
}

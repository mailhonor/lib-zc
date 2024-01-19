/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2023-06-13
 * ================================
 */

#include "zc.h"
#include <errno.h>

int main(int argc, char **argv)
{
    const char *fn_sys = 0;
    zmain_argument_run(argc, argv);
    zprintf("USAGE: %s config_pathname [ dirname ]\n", zvar_progname);
    if (zvar_main_redundant_argc > 0)
    {
        fn_sys = zvar_main_redundant_argv[0];
    }

    zmkdirs(0777, "中文文件名", "子文件夹", 0);
    if (!zempty(fn_sys))
    {
        zsys_mkdir(fn_sys, 0777);
    }
    return 0;
}

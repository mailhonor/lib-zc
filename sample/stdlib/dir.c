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
    const char *fn = 0;
    zmain_argument_run(argc, argv);
    if (zvar_main_redundant_argc < 1)
    {
        zprintf("USAGE: %s [ dirname ]\n", zvar_progname);
    }
    fn = zvar_main_redundant_argv[0];

    zmkdirs(0777, "中文文件名", "子文件夹", 0);
    if (zfile_exists(fn) < 1)
    {
        if (zmkdir(fn, 0777) < 0)
        {
            zinfo("ERR mkdir: %s", fn);
        }
        else
        {
            zinfo("OK mkdir: %s", fn);
        }
    }
    else
    {
        if (zrmdir_recurse(fn) < 0)
        {
            zinfo("ERR rmdir: %s", fn);
        }
        else
        {
            zinfo("OK rmdir: %s", fn);
        }
    }
    return 0;
}

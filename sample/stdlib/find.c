/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2024-02-15
 * ================================
 */

#include "zc.h"
#include <errno.h>

static void deal(const char *dirname)
{
    zargv_t *filenames = zargv_create(-1);
    int r;

    if ((r = zget_filenames_in_dir(dirname, filenames)) < 0)
    {
        zargv_free(filenames);
        zinfo("打开文件夹(%s)失败(%m)", dirname);
        return;
    }

    if (r == 0)
    {
        zargv_free(filenames);
        zinfo("文件夹(%s)不存在", dirname);
        return;
    }

    ZARGV_WALK_BEGIN(filenames, fn)
    {
        zinfo("fn: %s", fn);
    }
    ZARGV_WALK_END;

    zargv_free(filenames);
}

int main(int argc, char **argv)
{
    zmain_argument_run(argc, argv);
    const char *fn = zvar_main_redundant_argv[0];
    if (zempty(fn))
    {
        fn = ".";
    }
    deal(fn);
    return 0;
}

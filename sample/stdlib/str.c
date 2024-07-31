/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2024-02-26
 * ================================
 */

#include "zc.h"

static void test_strcasestr()
{
    char *r = strcasestr(zvar_main_redundant_argv[1], zvar_main_redundant_argv[2]);
    if (!r) {
        zprintf("not found\n");
        return;
    }
    zprintf("found :%s\n", r);
}

int main(int argc, char **argv)
{
    const char *fn_sys = 0;
    zmain_argument_run(argc, argv);
    zprintf("USAGE: %s strcasestr [ ... ] \n", zvar_progname);
    if (zvar_main_redundant_argc  < 1)
    {
        return 0;
    }
    const char *fn = zvar_main_redundant_argv[0];
    if (!strcmp(fn, "strcasestr")) {
        test_strcasestr();
    }

    return 0;
}

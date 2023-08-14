/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2017-07-23
 * ================================
 */

#include "zc.h"

int main(int argc, char **argv)
{
    zmain_argument_run(argc, argv);
    if (zvar_main_redundant_argc == 0) {
        zprintf("USAGE: %s http_cookie_string\n", argv[0]);
        exit(1);
    }
    zdict_t *cookies = zhttp_cookie_parse(zvar_main_redundant_argv[0], 0);
    zprintf("############################### cookie parse result:\n");
    zdict_debug_show(cookies);
    zdict_free(cookies);
    return 0;
}

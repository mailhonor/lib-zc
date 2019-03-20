/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-07-23
 * ================================
 */

#include "zc.h"

int main(int argc, char **argv)
{
    zmain_argument_run(argc, argv, 0);
    if (zvar_main_redundant_argc == 0) {
        printf("USAGE: %s http_cookie_string\n", argv[0]);
        exit(1);
    }
    zdict_t *cookies = zhttp_cookie_parse(zvar_main_redundant_argv[0], 0);
    printf("############################### cookie parse result:\n");
    zdict_debug_show(cookies);
    zdict_free(cookies);
    return 0;
}

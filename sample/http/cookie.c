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
    zmain_parameter_run(argc, argv);
    char *cookie_string = zconfig_get_str(zvar_default_config, "cookie", 0);
    if (zempty(cookie_string)) {
        printf("USAGE: %s -cookie http_cookie_string\n", argv[0]);
        exit(1);
    }
    zdict_t *cookies = zhttp_cookie_parse(cookie_string, 0);
    printf("############################### cookie parse result:\n");
    zdict_debug_show(cookies);
    zdict_free(cookies);
    return 0;
}

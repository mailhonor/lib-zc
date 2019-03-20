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
        printf("USAGE: %s http_url_string\n", argv[0]);
        exit(1);
    }
    zurl_t * url = zurl_parse(zvar_main_redundant_argv[0]);
    printf("############################### url parse result:\n");
    zurl_debug_show(url);

    zdict_t *dict = zurl_query_parse(url->query, 0);
    printf("############################### url query parse result:\n");
    zdict_debug_show(dict);

    zdict_free(dict);
    zurl_free(url);

    return 0;
}

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
        zprintf("USAGE: %s http_url_string\n", argv[0]);
        exit(1);
    }
    zurl_t * url = zurl_parse(zvar_main_redundant_argv[0]);
    zprintf("############################### url parse result:\n");
    zurl_debug_show(url);

    zdict_t *dict = zurl_query_parse(url->query, 0);
    zprintf("############################### url query parse result:\n");
    zdict_debug_show(dict);

    zdict_free(dict);
    zurl_free(url);

    return 0;
}

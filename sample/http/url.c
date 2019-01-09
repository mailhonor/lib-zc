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
    char *url_string = zconfig_get_str(zvar_default_config, "url", 0);
    if (zempty(url_string)) {
        printf("USAGE: %s -url http_url_string\n", argv[0]);
        exit(1);
    }
    zurl_t * url = zurl_parse(url_string);
    printf("############################### url parse result:\n");
    zurl_debug_show(url);

    zdict_t *dict = zurl_query_parse(url->query, 0);
    printf("############################### url query parse result:\n");
    zdict_debug_show(dict);

    zdict_free(dict);
    zurl_free(url);

    return 0;
}

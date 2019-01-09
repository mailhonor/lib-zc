/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2015-10-21
 * ================================
 */

#include "zc.h"

void usage(void)
{
    printf("USAGE: %s config_filename\n", zvar_progname);
    exit(0);
}

int main(int argc, char **argv)
{
    int i;
    char *fn;
    zconfig_t *config;

    zvar_progname = argv[0];

    if (argc == 1) {
        usage();
    }
    config = zconfig_create();

    /* load config */
    for (i = 1; i < argc; i++) {
        fn = argv[i];
        if (zconfig_load_from_filename(config, fn) < 0) {
            exit(-1);
        }
    }

    zconfig_update_string(config, "date", "2015-10-21", -1);
    zconfig_update_string(config, "author", "eli960@qq.com", -1);

    /* ouput config */
    zconfig_debug_show(config);

    zbuf_t *bf = zbuf_create(0);
    zbuf_put(bf, 'a');
    zbuf_strcat(bf, "aaaaaaaaaaa");
    printf("bf: %s\n", bf->data);

    return 0;
}

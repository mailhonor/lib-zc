/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-10-21
 * ================================
 */

#include "zc.h"

void usage(void)
{
    zinfo("USAGE: %s config_filename...\n", zvar_progname);
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
        if (zconfig_load(config, fn) < 0) {
            exit(-1);
        }
    }

    zconfig_update(config, "date", "2015-10-21");
    zconfig_update(config, "author", "eli960@163.com");

    /* ouput config */
    zconfig_show(config);

    return 0;
}

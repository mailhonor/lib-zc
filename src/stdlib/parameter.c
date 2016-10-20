/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-10-14
 * ================================
 */

#include "libzc.h"

int zparameter_run(int argc, char **argv, zparameter_fn_t param_fn)
{
    int i;
    int ret;

    for (i = 0; i < argc;) {
        ret = param_fn(argc - i, argv + i);
        if (ret < 1) {
            printf("error: parameter %s unknown or missing optval\n", argv[i]);
            exit(1);
        }
        i += ret;
    }

    return argc;
}

int zparameter_run_test(int argc, char **argv)
{
    char *optname, *optval;

    zvar_config_init();

    optname = argv[0];

    if ((optname[0] != '-') || (optname[1] == 0)) {
        return 0;
    }
    if (!strcmp(optname, "-d")) {
        zlog_set_level_from_console(ZLOG_DEBUG);
        return 1;
    }
    if (!strcmp(optname, "-v")) {
        zlog_set_level_from_console(ZLOG_VERBOSE);
        return 1;
    }

    if (argc < 2) {
        printf("error: parameter %s missing optval\n", optname);
        exit(1);
    }
    optval = argv[1];
    if (!strcmp(optname, "-c")) {
        zconfig_load(zvar_config, optval);
        return 2;
    }
    if (!strcmp(optname, "-o")) {
        char *p;

        p = strchr(optval, '=');
        if (p && (p != optval)) {
            optval = zstrndup(optval, p - optval);
            p++;
            zconfig_add(zvar_config, optval, p);
            zfree(optval);
        }
        return 2;
    }

    return 0;
}

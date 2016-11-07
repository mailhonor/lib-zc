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

    zvar_config_init();

    for (i = 0; i < argc;) {
        ret = param_fn(argc - i, argv + i);
        if (ret < 1) {
            return -1;
        }
        i += ret;
    }

    return argc;
}

int zparameter_run_test(int argc, char **argv)
{
    char *optname, *optval;

    optname = argv[0];
    if (optname[0] != '-') {
        printf("ERR parameter (%s) unknown, -h to help\r\n", optname);
        return -1;
    }
    if (!strcmp(optname + 1, "d")) {
        zlog_set_level_from_console(ZLOG_DEBUG);
        return 1;
    }
    if (!strcmp(optname + 1, "v")) {
        zlog_set_level_from_console(ZLOG_VERBOSE);
        return 1;
    }

    if (argc < 2) {
        printf("ERR parameter (%s) unknown or missing optval, -h to help\r\n", optname);
        return -1;
    }
    optval = argv[1];
    if (!strcmp(optname + 1, "c")) {
        zconfig_load(zvar_config, optval);
        return 2;
    }
    if (!strcmp(optname + 1, "o")) {
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
    if (!strcmp(optname + 1, "chdir")) {
        if (chdir(optval) == -1) {
            printf("ERR: chdir %s (%m)\r\n", optval);
            return -1;
        }
        return 2;
    }
    if (!strcmp(optname + 1, "chuser")) {
        if (zchroot_user(0, optval) < 0) {
            printf("ERR: change user %s (%m)\r\n", optval);
            return -1;
        }
        return 2;
    }

    return -1;
}

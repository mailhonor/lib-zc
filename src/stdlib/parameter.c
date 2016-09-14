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

    for (i = 0; i < argc; i++) {
        ret = param_fn(argc - i, argv + i);
        if (ret < 1) {
            zfatal("parameters: unknown or error %s", argv[i]);
        }
        i += ret - 1;
    }

    return argc;
}

int zparameter_run_dict(int argc, char **argv, zdict_t * param_fn_dict)
{
    int i;
    int ret;
    zparameter_fn_t param_fn;

    for (i = 0; i < argc; i++) {
        if (!zdict_lookup(param_fn_dict, argv[0], (char **)&param_fn)) {
            return i;
        }
        ret = param_fn(argc - i, argv + i);
        if (ret < 1) {
            zfatal("parameters: unknown or error %s", argv[i]);
        } else {
            i += ret - 1;
        }
    }

    return argc;
}

static zparameter_fn_t ___zparameter_run_list_lookup(zparameter_pair_t * param_fn_list, char *name)
{
    while (param_fn_list->name) {
        if (!strcmp(param_fn_list->name, name)) {
            return param_fn_list->func;
        }
        param_fn_list++;
    }

    return 0;
}

int zparameter_run_list(int argc, char **argv, zparameter_pair_t * param_fn_list)
{
    int i;
    int ret;
    zparameter_fn_t param_fn;

    for (i = 0; i < argc; i++) {
        param_fn = ___zparameter_run_list_lookup(param_fn_list, argv[i]);
        if (!param_fn) {
            return i;
        }
        ret = param_fn(argc - i, argv + i);
        if (ret < 1) {
            zfatal("parameters: unknown or error %s", argv[i]);
        } else {
            i += ret - 1;
        }
    }

    return argc;
}

/* ################################################################## */

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
        zfatal("%s: need optval");
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

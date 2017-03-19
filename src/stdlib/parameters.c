/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-10-12
 * ================================
 */

#include "zc.h"

char *zvar_progname = 0;
char *zvar_module_name = 0;
int zvar_test_mode = 0;

static char *___chroot = 0;
static char *___chuser = 0;

static void ___help()
{
    const char h[] = ""
        "--h                print this help\r\n"
        "--t                test mode\r\n"
        "--c config_file    continue load config\r\n"
        "--o key=value      update config\r\n"
        "--chdir path       change work path\r\n"
        "--chroot path      change root\r\n"
        "--chuser user      change root\r\n"
        "--module name\r\n";
    puts(h);
}

int zparameter_run(int argc, char **argv)
{
    char *optname, *optval;
    char buf[10240 + 1], *p;

    optname = argv[0];
    if (!strcmp(optname, "-h")) {
        return -1;
    }

    if ((optname[0] != '-') || (optname[1] != '-')) {
        return 0;
    }
    optname += 2;

    if (!strcmp(optname, "h")) {
        ___help();
        exit(1);
    }

    if (!strcmp(optname, "t")) {
        zvar_test_mode = 1;
        return 1;
    }

    optval = argv[1];
    if (!strcmp(optname, "c")) {
        if (zconfig_load(zvar_default_config, optval)) {
            printf("ERR: load config from %s\n", optval);
            exit(1);
        }
        return 2;
    }
    if (!strcmp(optname, "o")) {
        snprintf(buf, 10240, "%s", optval);
        p = strchr(buf, '=');
        if (p) {
            *p++ = 0;
            zconfig_update(zvar_default_config, buf, p);
        } else {
            zconfig_update(zvar_default_config, buf, zblank_buffer);
        }
        return 2;
    }
    if (!strcmp(optname, "chdir")) {
        if (chdir(optval) == -1) {
            printf("ERR: chdir %s (%m)\r\n", optval);
            exit(1);
        }
        return 2;
    }
    if (!strcmp(optname, "chroot")) {
        ___chroot = optval;
        return 2;
    }
    if (!strcmp(optname, "chuser")) {
        ___chuser = optval;
        return 2;
    }

    if (!strcmp(optname, "module")) {
        zvar_module_name = optval;
        return 2;
    }
    return 0;
}

void zparameter_run_2()
{
    if (___chuser == 0) {
        zfree(___chuser);
        ___chuser = zstrdup(zconfig_get_str(zvar_default_config, "zrun_user", 0));
    }
    if (___chroot == 0) {
        zfree(___chroot);
        ___chuser = zstrdup(zconfig_get_str(zvar_default_config, "zrun_root", 0));
    }
    if ((!ZEMPTY(___chroot)) || (!ZEMPTY(___chuser))) {
        if (!chroot_user(___chroot, ___chuser)) {
            printf("ERR: chroot_user (%m)\n");
            exit(1);
        }
    }
}

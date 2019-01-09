/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2015-10-12
 * ================================
 */

#include "zc.h"
#include <signal.h>


char *zvar_progname = 0;
int zvar_proc_stop = 0;
int zvar_test_mode = 0;
int zvar_max_fd = 102400;
char **zvar_main_parameter_argv = 0;
int zvar_main_parameter_argc = 0;

static void ___timeout_do2(int pid)
{
    exit(1);
}

static void ___timeout_do(int pid)
{ 
    zvar_proc_stop = 1;
    alarm(0);
    signal(SIGALRM, ___timeout_do2);
    alarm(2);
}

void zmain_parameter_run(int argc, char **argv)
{
    long i;
    char *optname, *optval;
    zconfig_t *cmd_cf = zconfig_create();

    zvar_progname = argv[0];
    zdefault_config_init();
    for (i = 1; i < argc; i++) {
        optname = argv[i];
        /* abc */
        if (optname[0] != '-') {
            zvar_main_parameter_argv = argv + i;
            zvar_main_parameter_argc = argc - i;
            break;
        }

        /* --abc */
        if (optname[1] == '-') {
            zconfig_update_string(cmd_cf, optname+2, "yes", 1);
            if (!strncmp(optname, "--debug-", 8)) {
            } else if (!strcmp(optname, "--fatal-catch")) {
                zvar_log_fatal_catch = 1;
            }
            continue;
        }

        /* -abc */
        if (i+1 >= argc) {
            printf("ERR parameter %s need value\n", optname);
            exit(1);
        }
        i++;
        optval = argv[i];
        if (!strcmp(optname, "-config")) {
            if (zconfig_load_from_filename(zvar_default_config, optval) < 0) {
                zinfo("ERR load config error from %s", optval);
                exit(1);
            }
        } else {
            zconfig_update_string(cmd_cf, optname+1, optval, -1);
        }
    }

    zconfig_load_annother(zvar_default_config, cmd_cf);
    zconfig_free(cmd_cf);

    if(!zvar_log_debug_enable) {
        if (zconfig_get_bool(zvar_default_config, "debug", 0)) {
            zvar_log_debug_enable = 1;
        }
    }

    i = zconfig_get_second(zvar_default_config, "exit-after", 0, 0, 3600L * 24 * 365 * 10);
    if (i > 0) {
        alarm(0);
        signal(SIGALRM, ___timeout_do);
        alarm(i);
    }
}


static zvector_t *___funcs_vec = 0;
void ___before_exit(void)
{
    ZVECTOR_WALK_BEGIN(___funcs_vec, void *, w) {
        void (*f)() = (void *)w;
        if(f) {
            f();
        }
    } ZVECTOR_WALK_END;
    zvector_free(___funcs_vec);
}

void zinner_atexit(void (*func)(void))
{
    if (!___funcs_vec) {
        ___funcs_vec = zvector_create(32);
        atexit(___before_exit);
    }
    zvector_add(___funcs_vec, func);
}

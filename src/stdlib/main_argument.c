/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2015-10-12
 * ================================
 */

#include "zc.h"
#include <signal.h>

char *zvar_progname = 0;
char **zvar_main_redundant_argv = 0;
int zvar_main_redundant_argc = 0;
int zvar_memleak_check = 0;
int zvar_sigint_flag = 0;

static zvector_t *zvar_main_redundant_argument_vector = 0;

static void main_redundant_argument_vector_fini(void)
{
    if (zvar_main_redundant_argument_vector) {
        zvector_free(zvar_main_redundant_argument_vector);
    }
    zvar_main_redundant_argument_vector = 0;
}

static void sigint_handler(int sig)
{
    zvar_sigint_flag = 1;
}

static void ___timeout_do2(int pid)
{
    exit(1);
}

static void ___timeout_do(int pid)
{ 
    alarm(0);
    signal(SIGALRM, ___timeout_do2);
    alarm(2);
}

void zmain_argument_run(int argc, char **argv, unsigned int (*self_argument_fn)(int argc, char **argv, int offset))
{
    int i, jump;
    char *optname, *optval;
    zconfig_t *cmd_cf = zconfig_create();

    zvar_progname = argv[0];
    zdefault_config_init();
    zvar_main_redundant_argument_vector = zvector_create(3);
    for (i = 1; i < argc; i++) {
        if (self_argument_fn) {
            jump = self_argument_fn(argc, argv, i);
            if (jump > 0) {
                i += jump - 1;
                continue;
            }
        }
        optname = argv[i];
        /* abc */
        if (optname[0] != '-') {
            zvector_push(zvar_main_redundant_argument_vector, optname);
            jump = 1;
            continue;
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
            if (zconfig_load_from_pathname(zvar_default_config, optval) < 0) {
                zinfo("ERR load config error from %s", optval);
                exit(1);
            }
        } else {
            zconfig_update_string(cmd_cf, optname+1, optval, -1);
        }
    }

    zconfig_load_annother(zvar_default_config, cmd_cf);
    zconfig_free(cmd_cf);
    
    zvar_main_redundant_argv = (char **)zvector_data(zvar_main_redundant_argument_vector);
    zvar_main_redundant_argc = zvector_len(zvar_main_redundant_argument_vector);
    zinner_atexit(main_redundant_argument_vector_fini);

    if(!zvar_log_debug_enable) {
        if (zconfig_get_bool(zvar_default_config, "debug", 0)) {
            zvar_log_debug_enable = 1;
        }
    }

    i = zconfig_get_second(zvar_default_config, "exit-after", 0);
    if (i > 0) {
        alarm(0);
        signal(SIGALRM, ___timeout_do);
        alarm(i);
    }

    zvar_memleak_check = zconfig_get_bool(zvar_default_config, "memleak-check", zvar_memleak_check);
    if (zvar_memleak_check) {
        signal(SIGINT, sigint_handler);
    }

    signal(SIGPIPE, SIG_IGN);
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

/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-10-12
 * ================================
 */

#include "zc.h"
#include <signal.h>
#ifdef _WIN32
#include <windows.h>
#endif // _WIN32

#ifdef _WIN32
int zvar_path_splitor = '\\';
#else  // _WIN32
int zvar_path_splitor = '/';
#endif // _WIN32

char *zvar_progname = 0;
static zargv_t *_zvar_main_argv_buffer = 0;

int zvar_main_argc = 0;
char **zvar_main_argv = 0;

int zvar_main_redundant_argc = 0;
char **zvar_main_redundant_argv = 0;

int zvar_main_kv_argc = 0;
char **zvar_main_kv_argv = 0;

int zvar_sigint_flag = 0;

static zvector_t *zvar_main_kv_argument_vector = 0;
static zvector_t *zvar_main_redundant_argument_vector = 0;

static void _main_argument_fini(void *unused)
{
    if (zvar_main_redundant_argument_vector)
    {
        zvector_free(zvar_main_redundant_argument_vector);
    }
    zvar_main_redundant_argument_vector = 0;

    if (zvar_main_kv_argument_vector)
    {
        zvector_free(zvar_main_kv_argument_vector);
    }
    zvar_main_kv_argument_vector = 0;

    if (_zvar_main_argv_buffer)
    {
        zargv_free(_zvar_main_argv_buffer);
    }
    _zvar_main_argv_buffer = 0;
}

#ifdef _WIN32
static char *optval_charset_deal(char *optval)
{
    int need = 0;
    for (unsigned char *p = (unsigned char *)optval; *p; p++)
    {
        if (*p > 127)
        {
            need = 1;
            break;
        }
    }
    if (!need)
    {
        return optval;
    }
    int len = strlen(optval);
    int result_size = len * 4 + 16;
    char *result_ptr = (char *)zmalloc(result_size + 1);
    int new_len = zMultiByteToUTF8(optval, len, result_ptr, result_size);
    if (new_len < 1)
    {
        new_len = 0;
    }
    result_ptr[new_len] = 0;
    zargv_addn(_zvar_main_argv_buffer, result_ptr, new_len);
    zfree(result_ptr);
    return zargv_data(_zvar_main_argv_buffer)[zargv_len(_zvar_main_argv_buffer) - 1];
}
#else  // _WIN32
static inline char *optval_charset_deal(char *optval)
{
    return optval;
}
#endif // _WIN32

static void zmain_argument_prepare_config(int argc, char **argv)
{
    int i;
    char *optname, *optval;
    zconfig_t *cmd_cf = 0;

    zvar_progname = argv[0];
    zvar_main_argc = argc;
    zvar_main_argv = argv;

#ifdef _WIN32
    SetConsoleOutputCP(65001);
#endif // _WIN32

    cmd_cf = zconfig_create();
    zdefault_config_init();
    zvar_main_kv_argument_vector = zvector_create(3);
    zvar_main_redundant_argument_vector = zvector_create(3);
#ifdef _WIN32
    _zvar_main_argv_buffer = zargv_create(13);
#else  // _WIN32
    _zvar_main_argv_buffer = zargv_create(1);
#endif // _WIN32
    for (i = 1; i < argc; i++)
    {
        optname = argv[i];
        /* abc */
        if ((optname[0] != '-') || (optname[1] == 0))
        {
            optname = optval_charset_deal(optname);
            zvector_push(zvar_main_redundant_argument_vector, optname);
            continue;
        }

        /* --abc */
        if (optname[1] == '-')
        {
            zconfig_update_string(cmd_cf, optname + 2, "yes", 3);
            zvector_push(zvar_main_kv_argument_vector, optname + 2);
            zvector_push(zvar_main_kv_argument_vector, "yes");
            continue;
        }

        /* -abc */
        if (i + 1 >= argc)
        {
            zdebug_show("ERROR parameter %s need value", optname);
            exit(1);
        }
        i++;
        optval = argv[i];
        optval = optval_charset_deal(optval);
        zvector_push(zvar_main_kv_argument_vector, optname + 1);
        zvector_push(zvar_main_kv_argument_vector, optval);
        if (!strcmp(optname, "-config"))
        {
            if (zconfig_load_from_pathname(zvar_default_config, optval) < 0)
            {
                zdebug_show("ERROR load config error from %s", optval);
                exit(1);
            }
        }
        else
        {
            zconfig_update_string(cmd_cf, optname + 1, optval, -1);
        }
    }

    zconfig_load_another(zvar_default_config, cmd_cf);
    zconfig_free(cmd_cf);

    zvar_main_redundant_argv = (char **)zvector_data(zvar_main_redundant_argument_vector);
    zvar_main_redundant_argc = zvector_len(zvar_main_redundant_argument_vector);

    zvar_main_kv_argv = (char **)zvector_data(zvar_main_kv_argument_vector);
    zvar_main_kv_argc = zvector_len(zvar_main_kv_argument_vector);
}

static void zmain_argument_do_something(int argc, char **argv)
{
    if (zconfig_get_bool(zvar_default_config, "debug-config", 0))
    {
        zconfig_debug_show(zvar_default_config);
    }

    zvar_log_debug_enable = zconfig_get_bool(zvar_default_config, "debug", zvar_log_debug_enable);
    zvar_log_fatal_catch = zconfig_get_bool(zvar_default_config, "fatal-catch", zvar_log_fatal_catch);
    zvar_log_output_disable = zconfig_get_bool(zvar_default_config, "log-output-disable", zvar_log_output_disable);

    zatexit(_main_argument_fini, 0);
    zsignal_ignore(SIGPIPE);

    void _zmain_argument_do_set_run_config();
    _zmain_argument_do_set_run_config();
}

void zmain_argument_run(int argc, char **argv)
{
    static int do_flag = 0;
    if (do_flag)
    {
        return;
    }
    zmain_argument_prepare_config(argc, argv);
    zmain_argument_do_something(argc, argv);
    do_flag = 1;
}

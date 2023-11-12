/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2017-01-06
 * ================================
 */

#include "zc.h"

static int windows_1252_flag = 0;
static void ___usage()
{
    zprintf("USAGE: %s [ --uconv ] [ --windows_1252 ] filename1 [filename2 ...]\n", zvar_progname);
    exit(1);
}

static void dorun(const char *fn)
{
    char charset[zvar_charset_name_max_size + 1];
    zbuf_t *content = zbuf_create(0);

    zfile_get_contents_sample(fn, content);

    if (windows_1252_flag)
    {
        extern char *zcharset_detect_1252(const char *data, int size, char *charset_result);
        if (zcharset_detect_1252(zbuf_data(content), zbuf_len(content), charset) == 0)
        {
            zprintf("%-30s: not found, try windows-1252\n", fn);
        }
        else
        {
            zprintf("%-30s: %s\n", fn, charset);
        }
    }
    else
    {
        if (zcharset_detect_cjk(zbuf_data(content), zbuf_len(content), charset) == 0)
        {
            zprintf("%-30s: not found\n", fn);
        }
        else
        {
            zprintf("%-30s: %s\n", fn, charset);
        }
    }

    zbuf_free(content);
}

int main(int argc, char **argv)
{
    zvar_charset_debug = 1;
    zmain_argument_run(argc, argv);
    if (zconfig_get_bool(zvar_default_config, "uconv", 0))
    {
        zcharset_convert_use_uconv();
    }
    windows_1252_flag = zconfig_get_bool(zvar_default_config, "windows_1252", 0);

    if (zvar_main_redundant_argc == 0)
    {
        ___usage();
    }
    for (int i = 0; i < zvar_main_redundant_argc; i++)
    {
        dorun(zvar_main_redundant_argv[i]);
    }

    return 0;
}

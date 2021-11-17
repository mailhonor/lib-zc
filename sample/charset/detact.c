/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2017-01-06
 * ================================
 */

#include "zc.h"

static void ___usage()
{
    printf("USAGE: %s [ --uconv ] filename1 [filename2 ...]\n", zvar_progname);
    exit(1);
}

static void dorun(const char *fn)
{
    char charset[zvar_charset_name_max_size + 1];
    zbuf_t *content = zbuf_create(0);

    zfile_get_contents_sample(fn, content);

    if (zcharset_detect_cjk(zbuf_data(content), zbuf_len(content), charset) == 0) {
        printf("%-30s: not found, maybe ASCII\n", fn);
    } else {
        printf("%-30s: %s\n", fn, charset);
    }

    zbuf_free(content);
}

int main(int argc, char **argv)
{
    zvar_charset_debug = 1;
    zmain_argument_run(argc, argv);
    if(zconfig_get_bool(zvar_default_config, "uconv", 0)) {
        zcharset_convert_use_uconv();
    }

    if (zvar_main_redundant_argc==0) {
        ___usage();
    }
    for (int i = 0; i < zvar_main_redundant_argc; i++) {
        dorun(zvar_main_redundant_argv[i]);
    }

    return 0;
}

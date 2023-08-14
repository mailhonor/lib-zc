/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2016-12-09
 * ================================
 */

#include "zc.h"
#include <ctype.h>

static void ___usage()
{
    zprintf("USAGE: %s -f from_charset -t to_charset [ --c ] [ --uconv ] filename \n", zvar_progname);
    exit(1);
}

int main(int argc, char **argv)
{
    const char *filename = 0;
    const char *from_charset = 0;
    const char *to_charset = 0;
    int ignore_bytes = 0;
    int converted_len = 0;
    zmain_argument_run(argc, argv);
    if (zvar_main_redundant_argc < 1) {
        ___usage();
    }
    filename = zvar_main_redundant_argv[0];
    ignore_bytes = (zconfig_get_bool(zvar_default_config, "c", 0) ? -1 : 0);
    from_charset = zconfig_get_str(zvar_default_config, "f", 0);
    to_charset = zconfig_get_str(zvar_default_config, "t", 0);
    if(zconfig_get_bool(zvar_default_config, "uconv", 0)) {
        zcharset_convert_use_uconv();
    }

    if (zempty(from_charset) || zempty(to_charset)) {
        ___usage();
    }

    zbuf_t *content = zbuf_create(0);
    zbuf_t *result = zbuf_create(0);
    zfile_get_contents_sample(filename, content);

    if ((zcharset_convert(from_charset, zbuf_data(content), zbuf_len(content),
                    to_charset , result, &converted_len 
                    ,ignore_bytes, 0)) < 0) {
        zprintf("ERROR can not convert\n");
    } else if (converted_len < zbuf_len(content)) {
        if (ignore_bytes == 0) {
            zprintf("ERROR illegal char at %d\n", converted_len+1);
        } else if (ignore_bytes == -1) {
            zprintf("ERROR unknown\n");
        } else {
            zprintf("ERROR illegal char too much > %d\n", ignore_bytes);
        }
    } else {
        if (zbuf_len(result)) {
            fwrite(zbuf_data(result), 1, zbuf_len(result), stdout);
        }
    }

    zbuf_free(content);
    zbuf_free(result);

    return 0;
}

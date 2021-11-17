/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2017-09-07
 * ================================
 */

#include "zc.h"
#include "lib.h"

static void usage()
{
    fprintf(stderr, "USAGE: %s [ -default-charset gb18030 ] header_file\n", zvar_progname);
    exit(1);
}

zbuf_t *result = 0;
char *default_charset;
static void show_decoded_line_utf8(zbuf_t *line)
{
    zbuf_reset(result);
    zmime_header_line_get_utf8(default_charset, zbuf_data(line), zbuf_len(line), result);
    if (zbuf_len(result)) {
        fwrite(zbuf_data(result), 1, zbuf_len(result), stdout);
    }
    printf("\n");
}

int main(int argc, char **argv)
{
    zmain_argument_run(argc, argv);
    char *fn;
    if (zvar_main_redundant_argc==0) {
        usage();
    }
    fn = zvar_main_redundant_argv[0];
    default_charset = zconfig_get_str(zvar_default_config, "default-charset", "gb18030");
    result = zbuf_create(0);
    mime_header_line_walk_test(fn, show_decoded_line_utf8);
    zbuf_free(result);
    return 0;
}

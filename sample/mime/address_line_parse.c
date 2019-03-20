/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-09-07
 * ================================
 */

#include "zc.h"
#include "lib.h"

static void usage()
{
    fprintf(stderr, "USAGE: %s header_file [ -default-charset gb18030 ]\n", zvar_progname);
    exit(1);
}

zbuf_t *result = 0;
char *default_charset;
static void show_decoded_line_utf8(zbuf_t *line)
{
    zbuf_reset(result);
    zvector_t *vec = zmime_header_line_get_address_vector_utf8(default_charset, zbuf_data(line), zbuf_len(line));
    printf("##########\n");
    if (!vec) {
        printf("    none\n");
        return;
    }
    ZVECTOR_WALK_BEGIN(vec, zmime_address_t *, addr) {
        fwrite(zbuf_data(result), 1, zbuf_len(result), stdout);
        printf("    %s <%s>\n", addr->name_utf8, addr->address);
    } ZVECTOR_WALK_END;
}

int main(int argc, char **argv)
{
    zmain_argument_run(argc, argv, 0);
    if (zvar_main_redundant_argc < 1) {
        usage();
    }
    char *fn = zvar_main_redundant_argv[0];
    default_charset = zconfig_get_str(zvar_default_config, "default-charset", "gb18030");
    result = zbuf_create(0);
    mime_header_line_walk_test(fn, show_decoded_line_utf8);
    zbuf_free(result);
    return 0;
}

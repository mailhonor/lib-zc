/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2015-12-03
 * ================================
 */

#include "zc.h"

static void usage(void)
{
    printf("USAGE:\n\t%s -decode filename\n\t%s -encode filename\n", zvar_progname, zvar_progname);
    exit(1);
}

int main(int argc, char **argv)
{
    zmmap_reader_t reader;
    char *fn = 0;
    int is_encode = 1;

    zmain_argument_run(argc, argv, 0);

    fn = zconfig_get_str(zvar_default_config, "encode", "");
    if (zempty(fn)) {
        is_encode = 0;
        fn = zconfig_get_str(zvar_default_config, "decode", "");
    }

    if (zempty(fn)) {
        usage();
    }

    if (zmmap_reader_init(&reader, fn) < 0) {
        printf("ERR read %s:%m", fn);
        exit(1);
    }

    zbuf_t *bf = zbuf_create(102400);

    if (is_encode) {
        zbase64_encode(reader.data, reader.len, bf, 1);
    } else {
        zbase64_decode(reader.data, reader.len, bf, 0);
    }
    int result_len = zbuf_len(bf);

    printf("result: %d\n%s\n", result_len, zbuf_data(bf));

    zbuf_free(bf);

    zmmap_reader_fini(&reader);

    return 0;
}

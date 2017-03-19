/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-04
 * ================================
 */

#include "libzc.h"

void usage(void)
{
    printf("USAGE:\n\t%s filename\n", zvar_progname);
    exit(1);
}

int main(int argc, char **argv)
{
    char *fn = 0;
    zmmap_reader_t reader;
    zbuf_t *result;

    zvar_progname = argv[0];

    if (argc < 2) {
        usage();
    }
    fn = argv[1];

    if (zmmap_reader_init(&reader, fn) < 0) {
        printf("error, read %s:%m", fn);
        exit(1);
    }

    result = zbuf_create(1);
    zqp_decode_2045(reader.data, reader.len, result);
    zmmap_reader_fini(&reader);

    printf("result:%s\n", ZBUF_DATA(result));
    zbuf_free(result);

    return 0;
}

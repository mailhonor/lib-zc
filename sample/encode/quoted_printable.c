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
    char *result;
    int result_len;

    zvar_progname = argv[0];

    if (argc < 2) {
        usage();
    }
    fn = argv[1];

    if (zmmap_reader_init(&reader, fn) < 0) {
        printf("error, read %s:%m", fn);
        exit(1);
    }

    result = zmalloc(reader.len + 16);
    result_len = zqp_decode_2045(reader.data, reader.len, result, reader.len);
    result[result_len] = 0;
    zmmap_reader_fini(&reader);

    printf("result:%s\n", result);
    zfree(result);

    return 0;
}

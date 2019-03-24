/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-03
 * ================================
 */

#include "zc.h"

zmmap_reader_t reader;
int cmd = 0;

void usage(void)
{
    printf("USAGE:\n\t%s -d filename\n\t%s -e filename\n", zvar_progname, zvar_progname);
    exit(1);
}

void test()
{
    zbuf_t *bf;
    int result_len;

    bf = zbuf_create(102400);

    if (cmd == 'd') {
        result_len = zbase64_decode(reader.data, reader.len, (char *)bf, Z_DF_ZBUF);
    } else {
        result_len = zbase64_encode(reader.data, reader.len, (char *)bf, Z_DF_ZBUF, 1);
    }

    printf("result: %d\n%s\n", result_len, ZBUF_DATA(bf));
    zbuf_free(bf);
}

int main(int argc, char **argv)
{
    char *fn = 0;
    int op;

    zvar_progname = argv[0];
    while ((op = getopt(argc, argv, "d:e:")) > 0) {
        if (op == 'd') {
            cmd = op;
            fn = optarg;
        } else if (op == 'e') {
            cmd = op;
            fn = optarg;
        }
    }

    if (!fn) {
        usage();
    }

    if (zmmap_reader_init(&reader, fn) < 0) {
        printf("error, read %s:%m", fn);
        exit(1);
    }

    test();

    zmmap_reader_fini(&reader);

    return 0;
}

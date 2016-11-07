/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-03
 * ================================
 */

#include "libzc.h"

zmmap_reader_t reader;
int cmd = 0;

void usage(void)
{
    printf("USAGE:\n\t%s -d filename\n\t%s -e filename\n", zvar_progname, zvar_progname);
    exit(1);
}

void test()
{
    char *result;
    int result_len;

    if (cmd == 'd') {
        result = zmalloc(reader.len + 16);
        result_len = zbase64_decode(reader.data, reader.len, result, reader.len);
    } else {
        int result_size = zbase64_encode_get_min_len(reader.len, 1);
        result = zmalloc(result_size);
        result_len = zbase64_encode(reader.data, reader.len, result, result_size, 1);
    }
    result[result_len] = 0;

    printf("result: %d\n%s\n", result_len, result);
    zfree(result);
}

void test_zbuf()
{
    zbuf_t *bf;
    int result_len;

    bf = zbuf_create(102400);

    if (cmd == 'd') {
        result_len = zbase64_decode_to_zbuf(reader.data, reader.len, bf);
    } else {
        result_len = zbase64_encode_to_zbuf(reader.data, reader.len, bf, 1);
    }

    printf("result: %d\n%s\n", result_len, ZBUF_DATA(bf));
    zbuf_free(bf);
}

void test_file()
{
    FILE *fp;
    int result_len;

    /* 这使用标注输出 */
    fp = stdout;

    if (cmd == 'd') {
        result_len = zbase64_decode_to_file(reader.data, reader.len, fp);
    } else {
        result_len = zbase64_encode_to_file(reader.data, reader.len, fp, 1);
    }

    printf("^^^result: %d\n", result_len);
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
    test_zbuf();
    test_file();

    zmmap_reader_fini(&reader);

    return 0;
}

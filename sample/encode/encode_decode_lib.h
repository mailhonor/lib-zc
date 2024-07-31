/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-12-03
 * ================================
 */

#include "../../zc.h"

static void usage(void)
{
    zprintf("USAGE: %s input_filename [ output_filename ]\n", zvar_progname);
    exit(1);
}


static int encode_decode_lib(const char *type, const char *encode_or_decode, int argc, char **argv)
{
    zmain_argument_run(argc, argv);
    if (zvar_main_redundant_argc < 1) {
        usage();
    }

    zmmap_reader_t reader;
    const char *input_filename = zvar_main_redundant_argv[0];
    const char *output_filename = 0;
    if (zvar_main_redundant_argc > 1) {
        output_filename = zvar_main_redundant_argv[1];
    }

    if (zmmap_reader_init(&reader, input_filename) < 0) {
        zprintf("ERROR read %s:%m", input_filename);
        exit(1);
    }
    char *data = reader.data;
    int len = reader.len;

    zbuf_t *bf = zbuf_create(102400);

    if (!strcmp(type, "base64")) {
        if (!strcmp(encode_or_decode, "encode")) {
            zbase64_encode(data, len, bf, 1);
        } else {
            zbase64_decode(data, len, bf);
        }
    } else if (!strcmp(type, "qp")) {
        if (!strcmp(encode_or_decode, "encode")) {
            zqp_encode_2045(data, len, bf, 1);
        } else {
            zqp_decode_2045(data, len, bf);
        }
    } else if (!strcmp(type, "uu")) {
        if (!strcmp(encode_or_decode, "decode")) {
            zuudecode(data, len, bf);
        }
    }

    zmmap_reader_fini(&reader);

    if (output_filename == 0) {
        zprintf("%s", zbuf_data(bf));
    } else {
        FILE *fp = fopen(output_filename, "wb+");
        if (!fp) {
            zprintf("ERROR open %s(%m)\n", output_filename);
        }
        fwrite(zbuf_data(bf), 1, zbuf_len(bf), fp);
        fclose(fp);
    }

    zbuf_free(bf);

    return 0;
}

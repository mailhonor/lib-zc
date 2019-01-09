/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-08-23
 * ================================
 */

#include "zc.h"

static void test(const char *src, int len)
{
    zbuf_t *result = zbuf_create(1024);
    zjson_t *j = zjson_create();

    zjson_unserialize(j, src, len);
    zjson_serialize(j, result, 0);
    puts(zbuf_data(result));

    zjson_free(j);
    zbuf_free(result);
}

int main(int argc, char **argv)
{
    zmain_parameter_run(argc, argv);
    if (zvar_main_parameter_argc < 1 ) {
        const char *s =  "{\"errcode\": \"-801\", \"errmsg\": \"Domain Not Exist\"}\r\n";
        test(s, strlen(s));
        printf("USAGE: %s json_filename\n", argv[0]);
        exit(1);
    }
    zmmap_reader_t fmap;
    if (zmmap_reader_init(&fmap, zvar_main_parameter_argv[0]) < 1) {
        printf("ERR can not open %s (%m)\n", zvar_main_parameter_argv[0]);
        exit(1);
    }

    test(fmap.data, fmap.len);

    return 0;
}

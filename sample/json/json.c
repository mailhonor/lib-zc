/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
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

static void test_loop(const char *src, int len, int times)
{
    if (times < 0) {
        times = 1000;
    }
    src = zmemdupnull(src, len);
    for (int i = 0; i < times; i++) {
        zjson_t *j = zjson_create();
        zjson_unserialize(j, src, len);
        zjson_free(j);
    }
    zfree(src);
}


int main(int argc, char **argv)
{
    zmain_argument_run(argc, argv);
    if (zvar_main_redundant_argc < 1 ) {
        const char *s =  "{\"errcode\": \"-801\", \"errmsg\": \"Domain Not Exist\"}\r\n";
        test(s, strlen(s));
        zprintf("USAGE: %s json_filename\n", argv[0]);
        exit(1);
    }
    zmmap_reader_t fmap;
    if (zmmap_reader_init(&fmap, zvar_main_redundant_argv[0]) < 1) {
        zprintf("ERROR can not open %s\n", zvar_main_redundant_argv[0]);
        exit(1);
    }

    test(fmap.data, fmap.len);

    if (zvar_main_redundant_argc > 1) {
        test_loop(fmap.data, fmap.len, atoi(zvar_main_redundant_argv[1]));
    }
    return 0;
}

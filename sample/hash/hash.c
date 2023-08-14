/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-12-06
 * ================================
 */

#include "zc.h"

void usage()
{
    zprintf("USAGE:\n\t%s filename\n", zvar_progname);
    exit(1);
}

int main(int argc, char **argv)
{
    char *fn;
    zmmap_reader_t reader;
    unsigned short int result_crc16;
    unsigned int result_crc32;
    size_t result_crc64;
    ZSTACK_BUF(p_buf, 1024);

    zvar_progname = argv[0];
    if (argc != 2) {
        usage();
    }
    fn = argv[1];

    if (zmmap_reader_init(&reader, fn) < 0) {
        zprintf("can not open %s\n", fn);
        exit(1);
    }

    result_crc16 = zcrc16(reader.data, reader.len, 0);
    zbuf_reset(p_buf);
    zhex_encode(&result_crc16, (int)(sizeof(unsigned short int)), p_buf);
    zprintf("crc16\t: %s\n", zbuf_data(p_buf));

    result_crc32 = zcrc32(reader.data, reader.len, 0);
    zbuf_reset(p_buf);
    zhex_encode(&result_crc32, (int)(sizeof(unsigned int)), p_buf);
    zprintf("crc32\t: %s\n", zbuf_data(p_buf));

    result_crc64 = zcrc64(reader.data, reader.len, 0);
    zbuf_reset(p_buf);
    zhex_encode(&result_crc64, (int)(sizeof(size_t)), p_buf);
    zprintf("crc64\t: %s\n", zbuf_data(p_buf));

    zmmap_reader_fini(&reader);

    return 0;
}

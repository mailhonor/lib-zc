/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2018-07-20
 * ================================
 */

#include "zc.h"

int main(int argc, char **argv)
{
    zmain_argument_run(argc, argv);
    zstream_t *fp = zstream_open_file("./dd", "w+");
    if (!fp) {
        printf("ERR can not open dd(%m)\n");
        return 0;
    }
    for (int i=0;i<1000;i++) {
        zstream_printf_1024(fp, "%5d XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n", i);
    }
    zstream_close(fp, 1);

    fp = zstream_open_file("./dd", "r");
    if (!fp) {
        printf("ERR can not open dd(%m) only read\n");
        return 0;
    }
    zbuf_t *linebuf = zbuf_create(0);
    zbuf_t *lastline = zbuf_create(0);
    while(1) {
        if (zstream_gets(fp, linebuf, 10240)) {
            zbuf_memcpy(lastline, zbuf_data(linebuf), zbuf_len(linebuf));
        } else {
            break;
        }
    }
    printf("last line: %s\n", zbuf_data(lastline));

    zbuf_free(linebuf);
    zbuf_free(lastline);
    zstream_close(fp, 1);

    return 0;
}

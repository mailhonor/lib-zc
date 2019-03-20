/*
 * ================================
 * eli960@qq.com
 * www.mailhonor.com
 * 2019-01-24
 * ================================
 */

#include "zc.h"

int main(int argc, char **argv)
{
    if (argc !=3 ) {
        printf("USAGE: %s ./postifx_config.zcdb somekey\n", argv[0]);
        exit(1);
    }
    char *fn = argv[1];
    char *key = argv[2];
    zbuf_t *error_msg = zbuf_create(128);
    zcdb_t *cdb = zcdb_open2(fn, error_msg);
    if (!cdb) {
        printf("ERR can not open zcdb %s, %s\n", fn, zbuf_data(error_msg));
        exit(1);
    }
    zbuf_free(error_msg);
    char *val;
    int vlen;
    int ret = zcdb_find(cdb, key, -1, (void **)&val, &vlen);
    if (ret < 0){
        printf("ERR find\n");
    } else if (ret == 0) {
        printf("OK not found\n");
    } else {
        printf("OK found, len:%d, result:", vlen);
        if (vlen) {
            fwrite(val, 1, vlen, stdout);
        }
        printf("\n");
    }
    zcdb_close(cdb);
    return 0;
}

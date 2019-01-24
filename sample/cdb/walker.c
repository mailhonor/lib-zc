/*
 * ================================
 * eli960@qq.com
 * www.mailhonor.com
 * 2019-01-23
 * ================================
 */

#include "zc.h"

int main(int argc, char **argv)
{
    if (argc !=2 ) {
        printf("USAGE: %s ./postifx_config.zcdb\n", argv[0]);
        exit(1);
    }
    char *fn = argv[1];
#if 0
    zcdb_t *cdb = zcdb_open(fn);
    if (!cdb) {
        printf("ERR can not open zcdb %s\n", fn);
        exit(1);
    }
#else
    zbuf_t *error_msg = zbuf_create(128);
    zcdb_t *cdb = zcdb_open2(fn, error_msg);
    if (!cdb) {
        printf("ERR can not open zcdb %s, %s\n", fn, zbuf_data(error_msg));
        exit(1);
    }
    zbuf_free(error_msg);
#endif
    zcdb_walker_t *walker = zcdb_walker_create(cdb);
    char *key, *val;
    int klen, vlen;
    int ret;
    while(1) {
        ret = zcdb_walker_walk(walker, (const void **)&key, &klen, (const void **)&val, &vlen);
        if (ret < 0) {
            printf("ERR walker\n");
            exit(1);
        }
        if (ret == 0) {
            break;
        }
        if (klen > 0) {
            fwrite(key, 1, klen, stdout);
        }
        printf("=");
        if (vlen > 0) {
            fwrite(val, 1, vlen, stdout);
        }
        printf("\n");
    }
    zcdb_walker_free(walker);
    zcdb_close(cdb);
    return 0;
}

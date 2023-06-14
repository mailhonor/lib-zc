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
    zcdb_t *cdb = zcdb_open(fn);
    if (!cdb) {
        printf("ERROR can not open zcdb %s\n", fn);
        exit(1);
    }
    zcdb_walker_t *walker = zcdb_walker_create(cdb);
    char *key, *val;
    int klen, vlen;
    int ret;
    while(1) {
        ret = zcdb_walker_walk(walker, (void **)&key, &klen, (void **)&val, &vlen);
        if (ret < 0) {
            printf("ERROR walker\n");
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

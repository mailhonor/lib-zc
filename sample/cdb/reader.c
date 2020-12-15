/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn
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
    zcdb_t *cdb = zcdb_open(fn);
    if (!cdb) {
        printf("ERR can not open zcdb %s\n", fn);
        exit(1);
    }
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

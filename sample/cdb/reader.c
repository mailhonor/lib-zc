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
        zprintf("USAGE: %s ./postifx_config.zcdb somekey\n", argv[0]);
        exit(1);
    }
    char *fn = argv[1];
    char *key = argv[2];
    zcdb_t *cdb = zcdb_open(fn);
    if (!cdb) {
        zprintf("ERROR can not open zcdb %s\n", fn);
        exit(1);
    }
    char *val;
    int vlen;
    int ret = zcdb_find(cdb, key, -1, (void **)&val, &vlen);
    if (ret < 0){
        zprintf("ERROR find\n");
    } else if (ret == 0) {
        zprintf("OK not found\n");
    } else {
        zprintf("OK found, len:%d, result:", vlen);
        if (vlen) {
            fwrite(val, 1, vlen, stdout);
        }
        zprintf("\n");
    }
    zcdb_close(cdb);
    return 0;
}

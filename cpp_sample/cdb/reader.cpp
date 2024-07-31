/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn
 * 2019-01-24
 * ================================
 */

#include "zcc/zcc_cdb.h"

int main(int argc, char **argv)
{
    if (argc !=3 ) {
        zcc_fatal("USAGE: %s ./postifx_config.zcdb somekey", argv[0]);
    }
    char *fn = argv[1];
    char *key = argv[2];
    zcc::cdb_reader reader;
    if (!reader.open_file(fn)) {
        zcc_fatal("can not open zcdb %s", fn);
    }
    char *val;
    int vlen;
    int ret = reader.find(key, -1, (void **)&val, &vlen);
    if (ret < 0){
        zcc_error("find");
    } else if (ret == 0) {
        zcc_info("OK not found");
    } else {
        zcc_info("OK found, len:%d, result:", vlen);
        if (vlen) {
            std::fwrite(val, 1, vlen, stderr);
            std::fprintf(stderr, "\n");
        }
    }
    return 0;
}

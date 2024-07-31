/*
 * ================================
 * eli960@qq.com
 * www.mailhonor.com
 * 2019-01-23
 * ================================
 */

#include "zcc/zcc_cdb.h"

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        zcc_fatal("USAGE: %s ./postifx_config.zcdb", argv[0]);
    }
    char *fn = argv[1];
    zcc::cdb_reader reader;
    if (!reader.open_file(fn))
    {
        zcc_fatal("ERROR can not open zcdb %s", fn);
    }
    zcc::cdb_walker walker(reader);
    
    char *key, *val;
    int klen, vlen;
    int ret;
    while (1)
    {
        ret = walker.walk((void **)&key, &klen, (void **)&val, &vlen);
        if (ret < 0)
        {
            zcc_fatal("walker\n");
        }
        if (ret == 0)
        {
            break;
        }
        if (klen > 0)
        {
            std::fwrite(key, 1, klen, stderr);
        }
        std::fprintf(stderr, " = ");
        if (vlen > 0)
        {
            fwrite(val, 1, vlen, stderr);
        }
        std::fprintf(stderr, "\n");
    }
    return 0;
}

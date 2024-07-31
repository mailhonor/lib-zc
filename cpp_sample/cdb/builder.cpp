/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn
 * 2019-01-23
 * ================================
 */

#include "zcc/zcc_cdb.h"

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        zcc_fatal("USAGE: %s configfile db", argv[0]);
    }

    char *config_filename = argv[1];
    char *cdb_filename = argv[2];

    zcc::config cf;
    if (!cf.load_from_file(config_filename))
    {
        zcc_fatal("load config from %s", config_filename);
    }

    zcc::cdb_builder builder;
    for (auto it = cf.begin(); it != cf.end(); it++)
    {
        builder.update(it->first, it->second);
    }
    if (builder.dump(cdb_filename) < 1)
    {
        zcc_fatal("build cdb to %s", cdb_filename);
    }
    zcc_info("OK %s", cdb_filename);
    return 0;
}

/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2024-02-07
 * ================================
 */

#include "zc.h"
#include <errno.h>

int main(int argc, char **argv)
{
    zmain_argument_run(argc, argv);
    zmmap_reader_t reader;
    const char *fn = zvar_main_redundant_argv[0];

    if (zconfig_get_bool(zvar_default_config, "sys", 0))
    {
        fn = argv[1];
        zinfo("test: %s", fn);
        if (zsys_mmap_reader_init(&reader, fn) < 0)
        {
            zfatal("ERR open %s(%m)", fn);
        }
    }
    else
    {
        zinfo("test: %s", fn);
        if (zmmap_reader_init(&reader, fn) < 0)
        {
            zfatal("ERR open %s(%m)", fn);
        }
    }

    fwrite(reader.data, 1, reader.len, stdout);
    zmmap_reader_fini(&reader);
    return 0;
}

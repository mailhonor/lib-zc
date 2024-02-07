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
    const char *fn = zvar_main_redundant_argv[0];
    zmmap_reader_t reader;
    if (zmmap_reader_init(&reader, fn) < 0)
    {
        zfatal("ERR open %s(%m)", fn);
    }
    fwrite(reader.data, 1, reader.len, stdout);
    zmmap_reader_fini(&reader);
    return 0;
}

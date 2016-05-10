/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-09
 * ================================
 */

#include "libzc.h"

void dorun(char *fn)
{
    zmmap_reader reader;
    char charset[128];

    if (zmmap_reader_init(&reader, fn) < 0) {
        printf("%-30s: %m", fn);
        exit(1);
    }

    if (zcharset_detect_cjk(reader.data, reader.len, charset) < 0) {
        printf("%-30s: not found, maybe ASCII\n", fn);
    } else {
        printf("%-30s: %s\n", fn, charset);
    }

    zmmap_reader_fini(&reader);
}

int main(int argc, char **argv)
{
    char *fn;

    zvar_progname = argv[0];
    if (argc < 2) {
        printf("USAGE: %s filename1 [filename2 ...]\n", zvar_progname);
        exit(1);
    }
    zparameter_run_test(argc - 1, argv + 1);

    ZARGV_WALK_BEGIN(zvar_parameter_value_list, fn)
    {
        dorun(fn);
    }
    ZARRAY_WALK_END;

    return 0;
}

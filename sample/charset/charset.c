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


zargv_t * file_list;

int do_arg(int argc, char **argv)
{
    char *optname;

    optname = argv[0];

    if(optname[0] != '-') {
        zargv_add(file_list, optname);
        return 1;
    }

    return zparameter_run_test(argc, argv);
}

int main(int argc, char **argv)
{
    char *fn;

    zvar_progname = argv[0];
    if (argc < 2) {
        printf("USAGE: %s filename1 [filename2 ...]\n", zvar_progname);
        exit(1);
    }
    file_list = zargv_create(1);
    zparameter_run(argc - 1, argv + 1, do_arg);

    ZARGV_WALK_BEGIN(file_list, fn)
    {
        dorun(fn);
    }
    ZARRAY_WALK_END;

    return 0;
}

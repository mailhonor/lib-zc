/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-09-05
 * ================================
 */

#include "libzc.h"

char *query = 0;
char *map_string = 0;

void USAGE(void)
{
    printf("USAGE: %s -q query -m map_string\n", zvar_progname);
    exit(1);
}

void do_search(void)
{
    zmap_t * map;
    zbuf_t *result;
    int ret;

    if (map_string == NULL || query == NULL) {
        USAGE();
    }
    map = zmap_create(map_string, 0);
    result = zbuf_create(1000);

    ret = zmap_query(map, query, result, 10*1000);
    if (ret == -2) {
        printf("timeout\n");
    } else if (ret == -1) {
        printf("error: %s\n", ZBUF_DATA(result));
    } else if (ret == 0) {
        printf("no result\n");
    } else {
        printf("result: %s\n", ZBUF_DATA(result));
    }
}

int do_arg(int argc, char **argv)
{
    char *optname;

    optname = argv[0];

    if (!strcmp(optname, "-m")) {
        if (argc < 2) {
            USAGE();
        }
        map_string = argv[1];

        return 2;
    }
    if (!strcmp(optname, "-q")) {
        if (argc < 2) {
            USAGE();
        }
        query = argv[1];

        return 2;
    }
    return zparameter_run_test(argc, argv);
}

int main(int argc, char **argv)
{
    zvar_progname = argv[0];
    if (argc < 2) {
        USAGE();
    }
    zparameter_run(argc - 1, argv + 1, do_arg);

    do_search();

    return 0;
}

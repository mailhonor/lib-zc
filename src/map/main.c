/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-09-14
 * ================================
 */

#include "libzc.h"

static char *query = 0;
static char *map_string = 0;

static void USAGE(void)
{
    printf("USAGE: %s -q query -m map_string\n", zvar_progname);
    exit(1);
}

static void do_search(void)
{
    zmap_t *map;
    zbuf_t *result;
    int ret;

    if (map_string == NULL || query == NULL) {
        USAGE();
    }
    map = zmap_create(map_string, 0);
    result = zbuf_create(1000);

    ret = zmap_query(map, query, result, 10 * 1000);
    if (ret < 0) {
        printf("ERR %s\n", ZBUF_DATA(result));
    } else if (ret == 0) {
        printf("NO\n");
    } else {
        printf("OK %s\n", ZBUF_DATA(result));
    }
}

static int do_arg(int argc, char **argv)
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

int zmap_main(int argc, char **argv)
{
    zvar_progname = argv[0];
    if (argc < 2) {
        USAGE();
    }
    if (zparameter_run(argc - 1, argv + 1, do_arg) < 0) {
        printf("ERR parameter error, -h for help\r\n");
        exit(1);
    }

    do_search();

    return 0;
}

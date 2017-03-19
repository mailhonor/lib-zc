/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-09-14
 * ================================
 */

#include "libzc.h"

static char *query = 0;
static char *finder_title = 0;

static void USAGE(void)
{
    printf("USAGE: %s -q query -m finder_title\n", zvar_progname);
    exit(1);
}

static void do_search(void)
{
    zfinder_t *finder;
    zbuf_t *result;
    int ret;

    if (finder_title == NULL || query == NULL) {
        USAGE();
    }
    finder = zfinder_create(finder_title);
    result = zbuf_create(1000);

    ret = zfinder_get(finder, query, result, 10 * 1000);
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
        finder_title = argv[1];

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

int zfinder_main(int argc, char **argv)
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

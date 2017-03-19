/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-09-14
 * ================================
 */

#include "zc.h"

static char *query = 0;
static char *finder_title = 0;

static void ___usage(char *parameter)
{
    printf("USAGE: %s -q query -t finder_title\n", zvar_progname);
    exit(1);
}

static void do_search(void)
{
    zfinder_t *finder;
    zbuf_t *result;
    int ret;

    if (finder_title == NULL || query == NULL) {
        ___usage(0);
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

int zfinder_main(int argc, char **argv)
{
    zvar_progname = argv[0];

    ZPARAMETER_BEGIN() {
        if (optval == 0) {
            ___usage(0);
        }
        if (!strcmp(optname, "-t")) {
            finder_title = optval;
            opti += 2;
            continue;
        }
        if (!strcmp(optname, "-q")) {
            query = optval;
            opti += 2;
            continue;
        }
    } ZPARAMETER_END;

    do_search();

    return 0;
}

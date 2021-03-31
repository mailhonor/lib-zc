/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2021-03-31
 * ================================
 */

#include "zc.h"

static void usage()
{
    printf("USAGE: %s keyword_list_file/keyword_db_file\n", zvar_progname);
    exit(0);
}

int main(int argc, char **argv)
{
    zmain_argument_run(argc, argv, 0);
    if (zvar_main_redundant_argc != 1) {
        usage();
    }
    char *listfn = zvar_main_redundant_argv[0];

    zmsearch_t *ms = 0;
    char buf[9];
    FILE *fp = fopen(listfn, "r");
    if (!fp) {
        printf("ERR open %s(%m)\n", listfn);
        exit(1);
    }
    if (fread(buf, 1, 8, fp) != 8) {
        buf[0] = 0;
    }
    fclose(fp);
    if (!memcmp(buf, "ZMSH", 4)) {
        ms = zmsearch_create_from_pathname(listfn);
        if (!ms) {
            printf("ERR open %s\n", listfn);
            exit(1);
        }
    } else {
        ms = zmsearch_create();
        if (zmsearch_add_token_from_pathname(ms, listfn) < 1) {
            printf("ERR open %s\n", listfn);
            exit(1);
        }
        zmsearch_add_over(ms);
    }

    zmsearch_walker_t *walker = zmsearch_walker_create(ms);
    char *token;
    int tlen;
    while (zmsearch_walker_walk(walker, (void **)&token, &tlen) > 0) {
        fwrite(token, 1, tlen, stdout);
        printf("\n");
    }
    zmsearch_walker_free(walker);

    zmsearch_free(ms);

    return 0;
}


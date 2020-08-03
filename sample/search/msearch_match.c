/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2020-07-29
 * ================================
 */

#include "zc.h"

static void usage()
{
    printf("USAGE: %s keyword_list_file/keyword_db_file file_for_search\n", zvar_progname);
    exit(0);
}

int main(int argc, char **argv)
{
    zmain_argument_run(argc, argv, 0);
    if (zvar_main_redundant_argc != 2) {
        usage();
    }
    char *listfn = zvar_main_redundant_argv[0];
    char *textfn = zvar_main_redundant_argv[1];

    zmsearch_t *ms = 0;
    char buf[9];
    FILE *fp = fopen(listfn, "r");
    if (!fp) {
        printf("ERR open %s(%m)\n", listfn);
        exit(1);
    }
    if (fread(buf, 1, 8, fp) != 8) {
        printf("ERR read %s(%m)\n", listfn);
        exit(1);
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

    zbuf_t *con = zbuf_create(10240);
    zfile_get_contents_sample(textfn, con);

    int offset;
    int len = zmsearch_match(ms, zbuf_data(con), zbuf_len(con), &offset);
    if (len < 1) {
        printf("NOT FOUND\n");
    } else {
        char *found = zmemdupnull(zbuf_data(con) + offset, len);
        printf("FOUND:\n");
        puts(found);
        zfree(found);
    }

    zbuf_free(con);
    zmsearch_free(ms);

    return 0;
}


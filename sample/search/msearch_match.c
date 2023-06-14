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
    zmain_argument_run(argc, argv);
    if (zvar_main_redundant_argc != 2) {
        usage();
    }
    char *listfn = zvar_main_redundant_argv[0];
    char *textfn = zvar_main_redundant_argv[1];

    zmsearch_t *ms = 0;
    char buf[9];
    FILE *fp = fopen(listfn, "rb");
    if (!fp) {
        printf("ERROR open %s(%m)\n", listfn);
        exit(1);
    }
    if (fread(buf, 1, 8, fp) != 8) {
        buf[0] = 0;
    }
    fclose(fp);
    if (!memcmp(buf, "ZMSH", 4)) {
        ms = zmsearch_create_from_pathname(listfn);
        if (!ms) {
            printf("ERROR open %s\n", listfn);
            exit(1);
        }
    } else {
        ms = zmsearch_create();
        if (zmsearch_add_token_from_pathname(ms, listfn) < 1) {
            printf("ERROR open %s\n", listfn);
            exit(1);
        }
        zmsearch_add_over(ms);
    }

    zbuf_t *con = zbuf_create(10240);
    zfile_get_contents_sample(textfn, con);

    int offset;
    const char *result;
    int len = zmsearch_match(ms, zbuf_data(con), zbuf_len(con), &result, &offset);
    if (len < 1) {
        printf("NOT FOUND\n");
    } else {
        printf("FOUND:\n");
        fwrite(result, 1, offset, stdout);
        printf("\n");
    }

    zbuf_free(con);
    zmsearch_free(ms);

    return 0;
}


/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2017-08-02
 * ================================
 */

#ifdef _LIB_ZC_SQLITE3_

#include "zc.h"

static void usage()
{
    printf("ERR USAGE %s -server server -query/exec/log sql_sentense\n", zvar_progname);
    exit(1);
}

int main(int argc, char **argv)
{
    zmain_argument_run(argc, argv);
    char *server = zconfig_get_str(zvar_default_config, "server", 0);
    char *sentense = zconfig_get_str(zvar_default_config, "query", 0);
    int ret, op =  'q';
    if (!sentense) {
        sentense = zconfig_get_str(zvar_default_config, "exec", 0);
        op = 'e';
    }
    if (!sentense) {
        sentense = zconfig_get_str(zvar_default_config, "log", 0);
        op = 'l';
    }
    if (zempty(server) || zempty(sentense)) {
        usage();
    }

    zsqlite3_proxy_client_t *pr = zsqlite3_proxy_client_connect(server);
    if (pr == 0) {
        printf("ERR can not open %s(%m)\n", server);
        exit(1);
    }
    zsqlite3_proxy_client_set_auto_reconnect(pr, 1);
    if (op == 'q') {
        ret = zsqlite3_proxy_client_query(pr, sentense, strlen(sentense));
        if (ret < 1) {
            printf("ERR %s\n", zsqlite3_proxy_client_get_error_msg(pr));
            exit(1);
        }

        zbuf_t **row;
        int column = zsqlite3_proxy_client_get_column(pr);
        while (1) {
            int r = zsqlite3_proxy_client_get_row(pr, &row);
            if (r < 0) {
                printf("ERR %s\n", zsqlite3_proxy_client_get_error_msg(pr));
                exit(1);
            }
            if (r == 0) {
                break;
            }
            for (int idx = 0; idx < column; idx++) {
                printf("%s ",  zbuf_data(row[idx]));
            }
            printf("\n");
        }
    } else if (op == 'e') {
        ret = zsqlite3_proxy_client_exec(pr, sentense, strlen(sentense));
        if (ret < 1) {
            printf("ERR %s\n", zsqlite3_proxy_client_get_error_msg(pr));
            exit(1);
        }
        printf("ok\n");
    } else if (op == 'l') {
        ret = zsqlite3_proxy_client_log(pr, sentense, strlen(sentense));
        if (ret < 1) {
            printf("ERR %s\n", zsqlite3_proxy_client_get_error_msg(pr));
            exit(1);
        }
        printf("ok\n");
    }
    zsqlite3_proxy_client_close(pr);
}
#else
#include <stdio.h>
int main(int argc, char **argv)
{
    printf("need defined _LIB_ZC_SQLITE3_\n");
    printf("cat makefiles/defined.include\n");
    printf("\n");
    printf("EXTRA_CFLAGS = -D_LIB_ZC_SQLITE3_\n");
    return 0;
}
#endif

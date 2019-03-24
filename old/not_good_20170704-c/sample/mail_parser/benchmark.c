/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-11
 * ================================
 */

#include "zc.h"
#include <ctype.h>

static void ___usage(char *parameter)
{
    printf("USAGE: %s -f eml_filename [ -loop 1000 ] [ -mpool ] \n", zvar_progname);
    exit(1);
}

static char hunman_buf[100];
static char *hunman_size2(long a)
{
    char buf[300], *p = buf, ch;
    int len, m, i;
    int tl = 0;

    hunman_buf[0] = 0;
    sprintf(buf, "%ld", a);
    len = strlen(buf);
    m = len % 3;

    while (1) {
        for (i = 0; i < m; i++) {
            ch = *p++;
            if (ch == '\0') {
                goto over;
            }
            hunman_buf[tl++] = ch;
        }
        hunman_buf[tl++] = ',';
        m = 3;
    }

  over:
    hunman_buf[tl] = 0;
    len = strlen(hunman_buf);
    if (len > 0) {
        if (hunman_buf[len - 1] == ',') {
            hunman_buf[len - 1] = 0;
        }
    }
    if (hunman_buf[0] == ',') {
        return hunman_buf + 1;
    }

    return hunman_buf;
}

int main(int argc, char **argv)
{
    zmpool_t *mpool = 0;
    zmail_t *parser;
    char *eml_fn = 0, *eml_data;
    int times = 1000, i, eml_size;
    zbuf_t *eml_data_buf;
    long t;

    int enable_mpool = 0;
    ZPARAMETER_BEGIN() {
        if (!strcmp(optname, "-mpool")) {
            enable_mpool = 1;
            opti+=1;
            continue;
        }
        if (!optval) {
            ___usage(0);
        }
        if (!strcmp(optname, "-loop")) {
            times = atoi(optval);
            opti+=2;
            continue;
        }
        if (!strcmp(optname, "-f")) {
            eml_fn = optval;
            opti+=2;
            continue;
        }
    } ZPARAMETER_END;

    if (!eml_fn) {
        ___usage(0);
    }

    eml_data_buf = zbuf_create(1024000);
    zfile_get_contents_sample(eml_fn, eml_data_buf);
    eml_data = ZBUF_DATA(eml_data_buf);
    eml_size = ZBUF_LEN(eml_data_buf);
    if (enable_mpool) {
        mpool = zmpool_create_greedy_pool();
    }

    printf("eml     : %s\n", eml_fn);
    printf("size    : %d(byte)\n", eml_size);
    printf("loop    : %d\n", times);
    printf("total   : %s(byte)\n", hunman_size2((long)eml_size * times));

    t = ztimeout_set(0);
    for (i = 0; i < times; i++) {
        parser = zmail_parser_create_MPOOL(mpool, eml_data, eml_size);
        zmail_parser_run(parser);
        zmail_parser_free(parser);
        if (mpool) {
            zmpool_reset(mpool);
        }
    }
    t = ztimeout_set(0) - t;

    printf("elapse  : %ld.%03ld(second)\n", t / 1000, t % 1000);
    printf("%%second : %s(byte)\n", hunman_size2((long)(((long)eml_size * times) / ((1.0 * t) / 1000))));


    if (mpool) {
        zfree(mpool);
    }
    zbuf_free(eml_data_buf);

    return 0;
}

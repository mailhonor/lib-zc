/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2023-12-17
 * ================================
 */

#include "zc.h"

static void usage()
{
    fprintf(stderr, "USAGE: %s rf822_date_string\n", zvar_progname);
    exit(1);
}

int main(int argc, char **argv)
{
    zmain_argument_run(argc, argv);
    char *ds;
    if (zvar_main_redundant_argc==0) {
        usage();
    }
    ds = zvar_main_redundant_argv[0];
    ssize_t u = zmime_header_line_decode_date(ds);
    zprintf("unix time: %zd\n", u);
    char buf[zvar_rfc1123_date_string_size + 1];
    zbuild_rfc822_date_string(u, buf);
    zprintf("date: %s\n", buf);
    return 0;
}

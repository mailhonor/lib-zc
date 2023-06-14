/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2023-06-13
 * ================================
 */

#include "zc.h"
#include <errno.h>

void usage(void)
{
    printf("USAGE: %s config_pathname\n", zvar_progname);
    exit(0);
}

int main(int argc, char **argv)
{
    const char *fn = "123.txt";
    const char *fn2 = "456.txt";
    zbuf_t *tmpbf = zbuf_create(1024);

    zmain_argument_run(argc, argv);

    zinfo("file exists 123.txt ? ");
    if (zfile_exists(fn) < 0)
    {
        zinfo("    error: %d", errno);
    }
    if (zfile_exists(fn) == 0)
    {
        zinfo("    none");
    }

    zinfo("create file 123.txt");
    if (zfile_put_contents(fn, "abc", 3) < 0)
    {
        zinfo("    error: %d", errno);
    }

    zinfo("read file 123.txt");
    if (zfile_get_contents(fn, tmpbf) < 0)
    {
        zinfo("    error: %d", errno);
    }
    zinfo("123.txt content: %s", zbuf_data(tmpbf));

    zinfo("link 123.txt 456.txt");
    if (zlink(fn, fn2) < 0)
    {
        zinfo("    error: %d", errno);
    }

    zinfo("link 123.txt 456.txt, force");
    if (zlink_force(fn, fn2, "./") < 0)
    {
        zinfo("    error: %d", errno);
    }

    return 0;
}

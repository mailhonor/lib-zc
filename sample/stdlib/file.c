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
    zprintf("USAGE: %s config_pathname [ first_filename ] [ second_filename ]\n", zvar_progname);
    exit(0);
}

int main(int argc, char **argv)
{
    const char *fn = "123.txt";
    const char *fn2 = "456.txt";
    zbuf_t *tmpbf = zbuf_create(1024);

    zmain_argument_run(argc, argv);
    if (zvar_main_redundant_argc > 0)
    {
        fn = zvar_main_redundant_argv[0];
    }
    if (zvar_main_redundant_argc > 1)
    {
        fn2 = zvar_main_redundant_argv[1];
    }

#ifdef _WIN32
    char fn_buf[10240 + 1];
    char fn2_buf[10240 + 1];
    zMultiByteToUTF8_any(fn, -1, fn_buf, 10240);
    zMultiByteToUTF8_any(fn2, -1, fn2_buf, 10240);
    fn = fn_buf;
    fn2 = fn2_buf;
#endif

    zinfo("\nfile exists %s ?", fn);
    if (zfile_exists(fn) < 0)
    {
        zinfo("    error: %d", errno);
    }
    if (zfile_exists(fn) == 0)
    {
        zinfo("    none");
    }
    if (zfile_exists(fn) == 1)
    {
        zinfo("    exists");
    }

    zinfo("\ncreate file %s", fn);
    if (zfile_put_contents(fn, "abc", 3) < 0)
    {
        zinfo("    error: %d", errno);
    }
    else
    {
        zinfo("    success");
    }

    zinfo("\nread file %s", fn);
    if (zfile_get_contents(fn, tmpbf) < 0)
    {
        zinfo("    error: %d", errno);
    }
    else
    {
        zinfo("    content: %s", zbuf_data(tmpbf));
    }

    zinfo("\nfile exists %s ?", fn2);
    if (zfile_exists(fn2) == 1)
    {
        zinfo("    exists");
        zinfo("\nunlink %s", fn2);
        if (zunlink(fn2) < 0)
        {
            zinfo("    error: %d", errno);
        }
        else
        {
            zinfo("    success");
        }
    }

    zinfo("\nfirst link %s => %s", fn, fn2);
    if (zlink(fn, fn2) < 0)
    {
        zinfo("    error: %d", errno);
    }
    else
    {
        zinfo("    success");
    }

    zinfo("\nsecond link %s => %s", fn, fn2);
    if (zlink(fn, fn2) < 0)
    {
        zinfo("    error: %d", errno);
    }
    else
    {
        zinfo("    success");
    }

    zinfo("\nthird force link %s => %s", fn, fn2);
    if (zlink_force(fn, fn2, "./") < 0)
    {
        zinfo("    error: %d", errno);
    }
    else
    {
        zinfo("    success");
    }

    return 0;
}

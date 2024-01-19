/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2023-06-13
 * ================================
 */

#include "zc.h"
#include <errno.h>

static char fn_buf[10240 + 1];
static char fn2_buf[10240 + 1];
static const char *fn = "一二三.txt";
static const char *fn2 = "四五六.txt";

static void test_utf8()
{
    if (zvar_main_redundant_argc > 0)
    {
        fn = zvar_main_redundant_argv[0];
#ifdef _WIN32
        zMultiByteToUTF8(fn, -1, fn_buf, 10240);
        fn = fn_buf;
#endif
    }
    if (zvar_main_redundant_argc > 1)
    {
        fn2 = zvar_main_redundant_argv[1];
#ifdef _WIN32
        char fn2_buf[10240 + 1];
        zMultiByteToUTF8(fn2, -1, fn2_buf, 10240);
        fn2 = fn2_buf;
#endif
    }

    zbuf_t *tmpbf = zbuf_create(1024);

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
}

static void test_sys()
{
    if (zvar_main_redundant_argc > 0)
    {
        fn = zvar_main_redundant_argv[0];
    }
    else
    {
#ifdef _WIN32
        zUTF8ToMultiByte(fn, -1, fn_buf, 10240);
        fn = fn_buf;
#endif
    }
    if (zvar_main_redundant_argc > 1)
    {
        fn2 = zvar_main_redundant_argv[1];
    }
    else
    {
#ifdef _WIN32
        zUTF8ToMultiByte(fn2, -1, fn2_buf, 10240);
        fn2 = fn2_buf;
#endif
    }

    zbuf_t *tmpbf = zbuf_create(1024);
    zinfo("\nfile exists %s ?", fn);
    if (zsys_file_exists(fn) < 0)
    {
        zinfo("    error: %d", errno);
    }
    if (zsys_file_exists(fn) == 0)
    {
        zinfo("    none");
    }
    if (zsys_file_exists(fn) == 1)
    {
        zinfo("    exists");
    }

    zinfo("\ncreate file %s", fn);
    if (zsys_file_put_contents(fn, "abc", 3) < 0)
    {
        zinfo("    error: %d", errno);
    }
    else
    {
        zinfo("    success");
    }

    zinfo("\nread file %s", fn);
    if (zsys_file_get_contents(fn, tmpbf) < 0)
    {
        zinfo("    error: %d", errno);
    }
    else
    {
        zinfo("    content: %s", zbuf_data(tmpbf));
    }

    zinfo("\nfile exists %s ?", fn2);
    if (zsys_file_exists(fn2) == 1)
    {
        zinfo("    exists");
        zinfo("\nunlink %s", fn2);
        if (zsys_unlink(fn2) < 0)
        {
            zinfo("    error: %d", errno);
        }
        else
        {
            zinfo("    success");
        }
    }

    zinfo("\nfirst link %s => %s", fn, fn2);
    if (zsys_link(fn, fn2) < 0)
    {
        zinfo("    error: %d", errno);
    }
    else
    {
        zinfo("    success");
    }

    zinfo("\nsecond link %s => %s", fn, fn2);
    if (zsys_link(fn, fn2) < 0)
    {
        zinfo("    error: %d", errno);
    }
    else
    {
        zinfo("    success");
    }

    zinfo("\nthird force link %s => %s", fn, fn2);
    if (zsys_link_force(fn, fn2, "./") < 0)
    {
        zinfo("    error: %d", errno);
    }
    else
    {
        zinfo("    success");
    }
}

int main(int argc, char **argv)
{
    zmain_argument_run(argc, argv);
    zprintf("USAGE: %s [ --sys ] [ first_filename ] [ second_filename ]\n", zvar_progname);
    if (zconfig_get_bool(zvar_default_config, "sys", 0))
    {
        test_sys();
    }
    else
    {
        test_utf8();
    }
    return 0;
}

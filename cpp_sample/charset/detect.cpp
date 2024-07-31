/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2017-01-06
 * ================================
 */

#include "zcc/zcc_charset.h"

static bool windows_1252_flag = false;
static void ___usage()
{
    zcc_error_and_exit("USAGE: %s [ --uconv ] [ --windows_1252 ] filename1 [filename2 ...]", zcc::progname);
}

static void dorun(const char *fn)
{
    std::string content = zcc::file_get_contents(fn);
    std::string charset;

    if (windows_1252_flag)
    {
        charset = zcc::charset::detect_1252(content.c_str(), (int)content.size());
        if (charset.empty())
        {
            zcc_info("%-30s: not found, windows-1252", fn);
        }
        else
        {
            zcc_info("%-30s: %s\n", fn, charset.c_str());
        }
    }
    else
    {
        charset = zcc::charset::detect_cjk(content.c_str(), (int)content.size());
        if (charset.empty())
        {
            zcc_info("%-30s: not found", fn);
        }
        else
        {
            zcc_info("%-30s: %s\n", fn, charset.c_str());
        }
    }
}

int main(int argc, char **argv)
{
    zcc::charset::var_debug_mode = true;
    zcc::main_argument::run(argc, argv);
    if (zcc::var_main_config.get_bool("uconv"))
    {
        zcc::charset::use_uconv();
    }
    windows_1252_flag = zcc::var_main_config.get_bool("windows_1252");

    if (zcc::main_argument::var_parameters.size() == 0)
    {
        ___usage();
    }
    for (const char *fn : zcc::main_argument::var_parameters)
    {
        dorun(fn);
    }

    return 0;
}

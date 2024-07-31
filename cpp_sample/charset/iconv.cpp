/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2016-12-09
 * ================================
 */

#include "zcc/zcc_charset.h"

static void ___usage()
{
    zcc_fatal("USAGE: %s -f from_charset -t to_charset [ --c ] [ --uconv ] [ file ] < input", zcc::progname);
}

int main(int argc, char **argv)
{
    const char *from_charset = 0;
    const char *to_charset = 0;
    int ignore_bytes = 0;
    int converted_len = 0;
    zcc::main_argument::run(argc, argv);
    ignore_bytes = zcc::var_main_config.get_int("c", 0);
    from_charset = zcc::var_main_config.get_cstring("f", "");
    to_charset = zcc::var_main_config.get_cstring("t", "");
    if (zcc::var_main_config.get_bool("uconv"))
    {
        zcc::charset::use_uconv();
    }

    if (zcc::empty(from_charset) || zcc::empty(to_charset))
    {
        ___usage();
    }

    std::string content;
    if (zcc::main_argument::var_parameters.size() > 0) {
        content = zcc::file_get_contents_sample(zcc::main_argument::var_parameters[0]);
    } else {
        content =zcc::stdin_get_contents();
    }

    std::string result;

    if ((zcc::charset::convert(from_charset, content.c_str(), content.size(),
                               to_charset, result, &converted_len, ignore_bytes, 0)) < 0)
    {
        zcc_error("can not convert");
    }
    else if (converted_len < (int)content.size())
    {
        if (ignore_bytes == 0)
        {
            zcc_error("illegal char at %d", converted_len + 1);
        }
        else if (ignore_bytes == -1)
        {
            zcc_error("unknown");
        }
        else
        {
            zcc_error("illegal char too much > %d", ignore_bytes);
        }
    }
    else
    {
        if (result.size())
        {
            fwrite(result.c_str(), 1, result.size(), stdout);
        }
    }

    return 0;
}

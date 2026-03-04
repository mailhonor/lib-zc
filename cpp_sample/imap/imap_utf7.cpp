/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2025-12-04
 * ================================
 */

#include "zcc/zcc_imap.h"

static void do_one(bool decode_mode, const std::string &token)
{
    std::string r;
    if (decode_mode)
    {
        r = zcc::imap_client::imap_utf7_to_utf8(token);
    }
    else
    {
        r = zcc::imap_client::utf8_to_imap_utf7(token);
    }
    printf("%s\n", r.c_str());
}

int main(int argc, char **argv)
{
    zcc::main_argument::run(argc, argv);
    const char *token = zcc::main_argument::var_parameter_argv[0];
    if (zcc::empty(token))
    {
        fprintf(stderr, "USAGE: %s --d/--e name-string/filename\n", zcc::progname);
        return 1;
    }
    bool decode_mode = false;
    if (zcc::var_main_config.get_bool("d"))
    {
        decode_mode = true;
    }
    else if (zcc::var_main_config.get_bool("e"))
    {
        decode_mode = false;
    }
    else
    {
        fprintf(stderr, "USAGE: %s --d/--e name-string/filename\n", zcc::progname);
        return 1;
    }
    int ret = zcc::file_exists(token);
    if (ret < 0)
    {
        zcc_fatal("system eorro: file not exists, %s", token);
        return 1;
    }
    if (ret == 0)
    {
        do_one(decode_mode, token);
        return 0;
    }
    FILE *fp = fopen(token, "r");
    if (fp == NULL)
    {
        zcc_fatal("file open error: %s", token);
        return 1;
    }
    char line_buf[8192 + 1];
    while (fgets(line_buf, sizeof(line_buf), fp) != NULL)
    {
        line_buf[sizeof(line_buf) - 1] = 0;
        std::string token = line_buf;
        zcc::trim_line_end_rn(token);
        do_one(decode_mode, token);
    }
    fclose(fp);

    return 0;
}

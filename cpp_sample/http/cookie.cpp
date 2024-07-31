/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2017-07-23
 * ================================
 */

#include "zcc/zcc_http.h"

int main(int argc, char **argv)
{
    zcc::main_argument::run(argc, argv);
    auto &args = zcc::main_argument::var_parameters;

    if (args.size() < 2)
    {
        zcc_info("USAGE: %s parse \"kk=vv;kkk=vvv\"", zcc::progname);
        zcc_info("USAGE: %s build name [ value ] [ -expires 12345678910 ] [ -path xxx ] [ -domain xx ] [ --secure ] [ --httponly]", zcc::progname);
        zcc::exit(1);
    }
    const char *cmd = args[0];
    if (!strcmp(cmd, "parse"))
    {
        auto r = zcc::http_cookie_parse(args[1]);
        zcc::debug_show(r);
    }
    else if (!strcmp(cmd, "build"))
    {
        const char *name = args[1];
        const char *value = "";
        if (args.size() > 2)
        {
            value = args[2];
        }
        int64_t expires = zcc::var_main_config.get_long("expires");
        const char *path = zcc::var_main_config.get_cstring("path");
        const char *domain = zcc::var_main_config.get_cstring("domain");
        bool secure = zcc::var_main_config.get_bool("secure");
        bool httponly = zcc::var_main_config.get_bool("httponly");
        std::string r = zcc::http_cookie_build_item(name, value, expires, path, domain, secure, httponly);
        zcc_info("%s", r.c_str());
    }

    return 0;
}

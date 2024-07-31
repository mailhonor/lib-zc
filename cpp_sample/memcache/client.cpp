/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2017-11-25
 * ================================
 */

#include "zcc/zcc_memcache.h"

int main(int argc, char **argv)
{
    zcc::memcache mc;
    std::string str;
    int64_t l;
    int flag;
    const char *server;

    zcc::main_argument::run(argc, argv);

    server = zcc::var_main_config.get_cstring("server", "127.0.0.1:11211");
    if (!mc.connect(server))
    {
        zcc_error("connect %s(%m)\n", server);
        goto over;
    }

    if (mc.cmd_set("iii", 0, 0, "123", 3) < 0)
    {
        zcc_error("set iii");
        goto over;
    }
    zcc_info("set iii 123");

    l = mc.cmd_incr("iii", 3);
    if (l < 0)
    {
        zcc_error("incr iii");
        goto over;
    }
    zcc_info("incr iii: 3, %zd", l);

    if (mc.cmd_get("iii", flag, str) < 0)
    {
        zcc_error("get iii\n");
        goto over;
    }
    zcc_info("get iii %s", str.c_str());

    if (mc.cmd_version(str) < 0)
    {
        zcc_error("version");
        goto over;
    }
    zcc_info("version %s", str.c_str());

    l = mc.cmd_incr("jjj", 1);
    if (l < 0)
    {
        zcc_error("incr jjj 1");
        mc.cmd_set("jjj",  0, 0, "1", 1);
        goto over;
    }
    zcc_info("incr jjj: 1, %zd", l);

over:
    zcc_info("USAGE: %s [ -server 127.0.0.1:11211 ]", zcc::progname);
    return 0;
}

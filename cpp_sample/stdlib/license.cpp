/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2017-05-19
 * ================================
 */

#include "zcc/zcc_stdlib.h"

int main(int argc, char **argv)
{
    const char *salt = 0;
    const char *mac = 0;
    const char *license = 0;

    zcc::main_argument::run(argc, argv);

    zcc_info("%s -salt salt_string -mac mac_address      #generate license", zcc::progname);
    zcc_info("%s -salt salt_string -license license      #check lincese", zcc::progname);

    salt = zcc::var_main_config.get_cstring("salt", "");
    mac = zcc::var_main_config.get_cstring("mac", "");
    license = zcc::var_main_config.get_cstring("license", "");

    if (zcc::empty(mac))
    {
        int ret = zcc::license_check(salt, license);
        if (ret == 1)
        {
            zcc_info("OK");
        }
        else if (ret == 0)
        {
            zcc_info("NO");
        }
        else if (ret < 0)
        {
            zcc_info("ERROR");
        }
        else
        {
            zcc_info("UNKNOWN");
        }
    }
    else
    {
        std::string r = zcc::license_build(salt, mac);
        zcc_info("%s", r.c_str());
    }

    return 0;
}

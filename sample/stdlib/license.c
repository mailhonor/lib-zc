/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2017-05-19
 * ================================
 */

#include "zc.h"

int main(int argc, char **argv)
{
    char *salt = 0;
    char *mac = 0;
    char *license = 0;

    zmain_argument_run(argc, argv);

    zprintf("%s -salt salt_string -mac mac_address      #generate license\n", zvar_progname);
    zprintf("%s -salt salt_string -license license      #check lincese\n", zvar_progname);

    salt = zconfig_get_str(zvar_default_config, "salt", 0);
    mac = zconfig_get_str(zvar_default_config, "mac", 0);
    license = zconfig_get_str(zvar_default_config, "license", 0);

    if (zempty(mac)) {
        int ret = zlicense_mac_check(salt, license);
        if (ret == 1) {
            zprintf("OK\n");
        } else if (ret == 0) {
            zprintf("NO\n");
        } else if (ret < 0) {
            zprintf("ERROR\n");
        } else {
            zprintf("UNKNOWN\n");
        }
    } else {
        zbuf_t *nlicense = zbuf_create(0);
        zlicense_mac_build(salt, mac, nlicense);
        zprintf("%s\n", zbuf_data(nlicense));
        zfree(nlicense);
    }

    return 0;
}

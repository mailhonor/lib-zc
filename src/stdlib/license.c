/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2016-09-09
 * ================================
 */

#include "zc.h"

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>


int zlicense_mac_check(const char *salt, const char *license)
{
    zbuf_t *license_c = zbuf_create(0);
    zargv_t *mac_list = zargv_create(0);

    if (ZEMPTY(salt) || ZEMPTY(license)) {
        return 0;
    }
    zget_mac_address(mac_list);
    ZARGV_WALK_BEGIN(mac_list, mac) {
        zbuf_reset(license_c);
        zlicense_mac_build(salt, mac, license_c);
        if (!strncasecmp(zbuf_data(license_c), license, 16)) {
            return 1;
        }
    } ZARGV_WALK_END;

    return 0;
}

void zlicense_mac_build(const char *salt, const char *_mac, zbuf_t *result)
{
    zbuf_t *builder = zbuf_create(128);
    zbuf_puts(builder, salt);
    zbuf_puts(builder, ",");
    int len = zbuf_len(builder);
    zbuf_puts(builder, _mac);
    zstr_tolower(zbuf_data(builder) + len);
    long crc = zcrc64(zbuf_data(builder), zbuf_len(builder), 0);
    zhex_encode(&crc, 8, result);
    zbuf_free(builder);
}

int zlicense_mac_check_from_config_filename(const char *salt, const char *config_file, const char *key)
{
    int ret = -1;
    zconfig_t *cf = zconfig_create();
    if (zconfig_load_from_filename(cf, config_file) < 0) {
        ret = -1;
        goto over;
    }
    char *license = zconfig_get_str(cf, key, "");
    if (ZEMPTY(license)) {
        ret = 0;
        goto over;
    }
    if (zlicense_mac_check(salt, license) == 0) {
        ret = 0;
        goto over;
    }
    ret = 1;
over:
    zconfig_free(cf);
    return ret;
}

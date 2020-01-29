/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
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
    if (ZEMPTY(salt) || ZEMPTY(license) || (strlen(license)!=16)) {
        return 0;
    }

    int ret = 0;

    zargv_t *mac_list = zargv_create(0);
    if (zget_mac_address(mac_list) < 0) {
        zargv_free(mac_list);
        return -1;
    }

    ZSTACK_BUF(license_c, 18);
    ZARGV_WALK_BEGIN(mac_list, mac) {
        zbuf_reset(license_c);
        zlicense_mac_build(salt, mac, license_c);
        if (!strcmp(zbuf_data(license_c), license)) {
            ret = 1;
            break;
        }
    } ZARGV_WALK_END;
    zargv_free(mac_list);

    return ret;
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
    char tmpbuf[18];
    sprintf(tmpbuf, "%016lX", crc);
    zbuf_strcat(result, tmpbuf);
    zbuf_free(builder);
}

int zlicense_mac_check_from_config_pathname(const char *salt, const char *config_file, const char *key)
{
    int ret = -1;
    zconfig_t *cf = zconfig_create();
    if (zconfig_load_from_pathname(cf, config_file) < 0) {
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

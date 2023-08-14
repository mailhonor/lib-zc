/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2016-09-09
 * ================================
 */

#include "zc.h"

typedef struct _license_cache_t _license_cache_t;
struct _license_cache_t {
    zargv_t *mac_argv;
};

char *zlicense_build(const char *salt, const char *target, char *new_license)
{
    zbuf_t *builder = zbuf_create(128);
    zbuf_puts(builder, salt);
    zbuf_puts(builder, ",");
    int len = zbuf_len(builder);
    zbuf_puts(builder, target);
    zstr_tolower(zbuf_data(builder) + len);
    size_t crc = zcrc64(zbuf_data(builder), zbuf_len(builder), 0);
    zbuf_free(builder);
    zsprintf(new_license, "%016zX", crc);
    return new_license;
}

static _license_cache_t *_license_cache_create()
{
    _license_cache_t *lc = (_license_cache_t *)zcalloc(1, sizeof(_license_cache_t));
    return lc;
}

static void _license_cache_free(_license_cache_t *lc)
{
    if (!lc) {
        return;
    }
    zargv_free(lc->mac_argv);
    zfree(lc);
}

/* -1: 系统错, 0: 不匹配, 1: 匹配 */
static int _license_check_inner(_license_cache_t *lc, const char *salt, const char *license)
{
    int ret = 0, tlen;
    char license_key[zvar_license_size + 1], test_license[zvar_license_size + 1];
    char target[32 + 1];
    const char *p = 0;

    license = (license?license:"");
    for (; *license; license++) {
        if (*license != ',') {
            break;
        }
    }
    p = strchr(license, ',');
    if (p) {
        strncpy(license_key, p + 1, zvar_license_size);
        tlen = (int)(p - license);
        if (tlen > 32) {
            tlen = 32;
        }
        strncpy(target, license, tlen);
        target[tlen] = 0;
    } else {
        strncpy(license_key, license, zvar_license_size);
        strcpy(target, "*MAC*");
    }
    license_key[zvar_license_size] = 0;

    if (!strcmp(target, "*MAC*")) {
        if (!(lc->mac_argv)) {
            lc->mac_argv = zargv_create(-1);
            if (zget_mac_address(lc->mac_argv) < 0) {
                ret = -1;
                goto over;
            }
        }
        ZARGV_WALK_BEGIN(lc->mac_argv, mac) {
            zlicense_build(salt, mac, test_license);
            if (!strcmp(license_key, test_license)) {
                ret = 1;
                goto over;
            }
        } ZARGV_WALK_END;
    } else {
        zlicense_build(salt, target, test_license);
        if (!strcmp(license_key, test_license)) {
            ret = 1;
            goto over;
        }
    }

    ret = 0;

over:
    return ret;
}

int zlicense_check(const char *salt, const char *license)
{
    _license_cache_t *lc = _license_cache_create();
    int ret = _license_check_inner(lc, salt, license);
    _license_cache_free(lc);
    return ret;
}

int zlicense_check_from_config_pathname(const char *salt, const char *config_file, const char *key)
{
    const char *cfs[2];
    cfs[0] = config_file;
    cfs[1] = 0;
    return zlicense_check_from_config_pathnames(salt, cfs, key);
}

int zlicense_check_from_config_pathnames(const char *salt, const char **config_files, const char *key)
{
    int ret = 0;
    zconfig_t *cf = zconfig_create();
    _license_cache_t *lc = _license_cache_create();
    for (const char **fn = config_files; *fn; fn ++) {
        zconfig_reset(cf);
        if (zconfig_load_from_pathname(cf, *fn) < 0) {
            continue;
        }
        char *license = zconfig_get_str(cf, key, "");
        if (ZEMPTY(license)) {
            ret = 0;
            continue;
        }
        if ((ret = _license_check_inner(lc, salt, license)) == 0) {
            continue;
        }
        break;
    }
    _license_cache_free(lc);
    zconfig_free(cf);
    return ret;
}

void zlicense_mac_build(const char *salt, const char *_mac, zbuf_t *result)
{
    char buf[zvar_license_size + 1];
    zlicense_build(salt, _mac, buf);
    zbuf_strcat(result, buf);
}


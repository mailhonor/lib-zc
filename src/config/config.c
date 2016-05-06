/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-10-12
 * ================================
 */

#include "libzc.h"

zconfig_t *zvar_config = 0;

void zvar_config_init(void)
{
    if (zvar_config == 0) {
        zvar_config = zconfig_create();
    }
}

/* ################################################################## */
void zconfig_show(zconfig_t * cf)
{
    char *key, *value;

    ZCONFIG_WALK_BEGIN(cf, key, value) {
        printf("%s = %s\n", key, value);
    }
    ZCONFIG_WALK_END;
}

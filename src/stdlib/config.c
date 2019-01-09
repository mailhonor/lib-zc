/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2015-10-12
 * ================================
 */

#include "zc.h"

zconfig_t *zvar_default_config = 0;
zconfig_t *zdefault_config_init(void)
{
    if (zvar_default_config == 0) {
        zvar_default_config = zconfig_create();
        zinner_atexit(zdefault_config_fini);
    }
    return zvar_default_config;
}

void zdefault_config_fini(void)
{
    if (zvar_default_config) {
        zconfig_free(zvar_default_config);
    }
    zvar_default_config = 0;
}

/* ################################################################## */
int zconfig_load_from_filename(zconfig_t * cf, const char *filename)
{
    FILE *fp;
    char *name, *value;
    char *line_buf;

    fp = fopen(filename, "r");
    if (!fp) {
        return -1;
    }
    line_buf = (char *)zmalloc(10240 + 10);

    while (fgets(line_buf, 10240, fp)) {
        name = ztrim_left(line_buf);
        if (ZEMPTY(name)|| (*name == '#')) {
            continue;
        }
        value = strchr(name, '=');
        if (value) {
            *value++ = 0;
        } else {
            value = "";
        }
        name = ztrim_right(name);
        if (!ZEMPTY(value)) {
            value = ztrim(value);
        }
        zconfig_update_string(cf, name, value, -1);
    }
    zfree(line_buf);
    fclose(fp);

    return 0;
}

void zconfig_load_annother(zconfig_t *cf, zconfig_t *another)
{
    ZCONFIG_WALK_BEGIN(another, k, v) {
        zconfig_update(cf, k, v);
    } ZCONFIG_WALK_END;
}

/* table ############################################################ */
void zconfig_get_str_table(zconfig_t * cf, zconfig_str_table_t * table)
{
    while (table->name) {
        table->target[0] = zconfig_get_str(cf, table->name, table->defval);
        table++;
    }
}

void zconfig_get_bool_table(zconfig_t * cf, zconfig_bool_table_t * table)
{
    while (table->name) {
        table->target[0] = zconfig_get_bool(cf, table->name, table->defval);
        table++;
    }
}

#define ___ZCONFIG_GET_TABLE(ttype) \
    void zconfig_get_## ttype ## _table(zconfig_t * cf, zconfig_ ## ttype ## _table_t * table) \
    { \
        while (table->name) \
        { \
            table->target[0] = zconfig_get_ ## ttype(cf, table->name, table->defval, table->min, table->max); \
            table++; \
        } \
    }
___ZCONFIG_GET_TABLE(int);
___ZCONFIG_GET_TABLE(long);
___ZCONFIG_GET_TABLE(size);
___ZCONFIG_GET_TABLE(second);

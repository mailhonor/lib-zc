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
int zconfig_load(zconfig_t * cf, const char *filename)
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
        zconfig_add(cf, name, value);
    }
    zfree(line_buf);
    fclose(fp);

    return 0;
}

/* ################################################################## */

#define ___RANGE_VALUE(val, min, max) {if(val < min){val = min;}else if(val>max){val = max;}}
char *zconfig_get_str(zconfig_t * cf, const char *name, const char *def)
{
    char *value;

    zdict_lookup(cf, name, &value);
    if (!ZEMPTY(value)) {
        return value;
    }

    return (char *)def;
}

int zconfig_get_bool(zconfig_t * cf, const char *name, int def)
{
    char *value;

    value = zconfig_get_str(cf, name, 0);
    if (ZEMPTY(value)) {
        return def;
    }

    return zstr_to_bool(value, def);
}

int zconfig_get_int(zconfig_t * cf, const char *name, int def, int min, int max)
{
    char *value;

    value = zconfig_get_str(cf, name, 0);
    if (!ZEMPTY(value)) {
        def = atoi(value);
    }
    ___RANGE_VALUE(def, min, max);

    return def;
}

long zconfig_get_long(zconfig_t * cf, const char *name, long def, long min, long max)
{
    char *value;

    value = zconfig_get_str(cf, name, 0);
    if (!ZEMPTY(value)) {
        def = atoll(value);
    }
    ___RANGE_VALUE(def, min, max);

    return def;
}

long zconfig_get_second(zconfig_t * cf, const char *name, long def, long min, long max)
{
    char *value;

    value = zconfig_get_str(cf, name, 0);
    if (!ZEMPTY(value)) {
        def = zstr_to_second(value);
    }

    ___RANGE_VALUE(def, min, max);

    return def;
}

long zconfig_get_size(zconfig_t * cf, const char *name, long def, long min, long max)
{
    char *value;

    value = zconfig_get_str(cf, name, 0);
    if (!ZEMPTY(value)) {
        def = zstr_to_size(value);
    }

    ___RANGE_VALUE(def, min, max);

    return def;
}

/* ################################################################## */
/* table */

void zconfig_get_str_table(zconfig_t * cf, zconfig_str_table_t * table)
{
    while (table->name) {
        if (table->target[0]) {
            zfree(table->target[0]);
        }
        table->target[0] = zconfig_get_str(cf, table->name, table->defval);
    }
}

void zconfig_get_bool_table(zconfig_t * cf, zconfig_bool_table_t * table)
{
    while (table->name) {
        table->target[0] = zconfig_get_bool(cf, table->name, table->defval);
    }
}

#define ___ZCONFIG_GET_TABLE(ttype) \
    void zconfig_get_## ttype ## _table(zconfig_t * cf, zconfig_ ## ttype ## _table_t * table) \
    { \
        while (table->name) \
        { \
            table->target[0] = zconfig_get_ ## ttype(cf, table->name, table->defval, table->min, table->max); \
        } \
    }
___ZCONFIG_GET_TABLE(int);
___ZCONFIG_GET_TABLE(long);
___ZCONFIG_GET_TABLE(size);
___ZCONFIG_GET_TABLE(second);

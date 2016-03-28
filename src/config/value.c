/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-12-29
 * ================================
 */

#include "libzc.h"

char *zconfig_get_str(zconfig_t * cf, char *name, char *def)
{
    char *value;

    zdict_lookup(cf, name , &value);
    if (! ZEMPTY(value))
    {
        return value;
    }

    return def;
}

int zconfig_get_bool(zconfig_t * cf, char *name, int def)
{
    char *value;

    value = zconfig_get_str(cf, name, 0);
    if (ZEMPTY(value))
    {
        return def;
    }

    return zstr_to_bool(value, def);
}

int zconfig_get_int(zconfig_t * cf, char *name, int def)
{
    char *value;

    value = zconfig_get_str(cf, name, 0);
    if (ZEMPTY(value))
    {
        return def;
    }

    return atoi(value);
}

long zconfig_get_long(zconfig_t * cf, char *name, long def)
{
    char *value;

    value = zconfig_get_str(cf, name, 0);
    if (ZEMPTY(value))
    {
        return def;
    }

    return atoll(value);
}

long zconfig_get_second(zconfig_t * cf, char *name, long def)
{
    char *value;

    value = zconfig_get_str(cf, name, 0);
    if (ZEMPTY(value))
    {
        return def;
    }


    return zstr_to_second(value);
}

long zconfig_get_size(zconfig_t * cf, char *name, long def)
{
    char *value;

    value = zconfig_get_str(cf, name, 0);
    if (ZEMPTY(value))
    {
        return def;
    }

    return zstr_to_size(value);
}

/* ################################################################## */
/* table */

void zconfig_get_str_table(zconfig_t * cf, zconfig_str_table_t * table)
{
    while (table->name)
    {
        if (table->target[0])
        {
            zfree(table->target[0]);
        }
        table->target[0] = zstrdup(zconfig_get_str(cf, table->name, table->defval));
    }
}

#define ___ZCONFIG_GET_TABLE(ttype) \
    void zconfig_get_## ttype ## _table(zconfig_t * cf, zconfig_ ## ttype ## _table_t * table) \
    { \
        while (table->name) \
        { \
            table->target[0] = zconfig_get_ ## ttype(cf, table->name, table->defval); \
        } \
    }
___ZCONFIG_GET_TABLE(int);
___ZCONFIG_GET_TABLE(long);
___ZCONFIG_GET_TABLE(bool);
___ZCONFIG_GET_TABLE(size);
___ZCONFIG_GET_TABLE(second);

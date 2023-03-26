/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-11-12
 * ================================
 */

#include "zc.h"

namespace zcc
{

config var_default_config(zdefault_config_init());

config::config()
{
    cf_ = zconfig_create();
    new_flag_ = true;
}

config::config(zconfig_t *cf)
{
    cf_ = cf;
    new_flag_ = false;
}

config::~config()
{
    if (new_flag_)
    {
        zconfig_free(cf_);
    }
}

config &config::reset()
{
    zconfig_reset(cf_);
    return *this;
}

config &config::update(const char *key, const char *val, int vlen)
{
    zconfig_update_string(cf_, key, val, vlen);
    return *this;
}

config &config::update(const char *key, std::string &val)
{
    zconfig_update_string(cf_, key, val.c_str(), val.size());
    return *this;
}

config &config::remove(const char *key)
{
    zconfig_delete(cf_, key);
    return *this;
}

bool config::load_from_pathname(const char *pathname)
{
    return (zconfig_load_from_pathname(cf_, pathname) ? true : false);
}

config &config::load_another(zconfig_t *another)
{
    zconfig_load_another(cf_, another);
    return *this;
}

config &config::load_another(config &another)
{
    zconfig_load_another(cf_, another.cf_);
    return *this;
}

config &config::debug_show()
{
    zconfig_debug_show(cf_);
    return *this;
}

bool config::get_bool(const char *key, bool default_val)
{
    return (zconfig_get_bool(cf_, key, (default_val ? 1 : 0)) ? true : false);
}

const char *config::get_str(const char *key, const char *default_val)
{
    return zconfig_get_str(cf_, key, default_val);
}

int config::get_int(const char *key, int default_val)
{
    return zconfig_get_int(cf_, key, default_val);
}

long config::get_long(const char *key, long default_val)
{
    return zconfig_get_long(cf_, key, default_val);
}

long config::get_second(const char *key, long default_val)
{
    return zconfig_get_second(cf_, key, default_val);
}

long config::get_size(const char *key, long default_val)
{
    return zconfig_get_size(cf_, key, default_val);
}

} // namespace zcc

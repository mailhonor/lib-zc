/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-11-12
 * ================================
 */

#include "zc.h"

namespace zcc {

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
    if (new_flag_) {
        zconfig_free(cf_);
    }
}

config &config::update(const char *key, const char *val)
{
    zconfig_update_string(cf_, key, val, -1);
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
    return (zconfig_load_from_pathname(cf_, pathname)?true:false);
}

config &config::load_annother(zconfig_t *another)
{
    zconfig_load_annother(cf_, another);
    return *this;
}

config &config::load_annother(config &another)
{
    zconfig_load_annother(cf_, another.cf_);
    return *this;
}

config &config::debug_show()
{
    zconfig_debug_show(cf_);
    return *this;
}

bool config::get_bool(const char *key, bool default_val)
{
    return (zconfig_get_bool(cf_, key, (default_val?1:0))?1:0);
}

}

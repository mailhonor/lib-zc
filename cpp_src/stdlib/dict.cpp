/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-10-12
 * ================================
 */

#include <cstdarg>
#include <algorithm>
#include "zcc/zcc_stdlib.h"

zcc_namespace_begin;

void debug_show(const dict &dt)
{
    std::string s;
    for (auto it = dt.begin(); it != dt.end(); it++)
    {
        s.append(it->first).append(" = ").append(it->second).append("\r\n");
    }
    zcc_output("%s", s.c_str());
}

const char *get_cstring(const dict &dt, const char *key, const char *def_val)
{
    auto it = dt.find(key);
    if (it == dt.end())
    {
        return def_val;
    }
    return it->second.c_str();
}

const char *get_cstring(const dict &dt, const std::string &key, const char *def_val)
{
    auto it = dt.find(key);
    if (it == dt.end())
    {
        return def_val;
    }
    return it->second.c_str();
}

std::string get_string(const dict &dt, const char *key, const char *def_val)
{
    auto it = dt.find(key);
    if (it == dt.end())
    {
        return def_val;
    }
    return it->second;
}

std::string get_string(const dict &dt, const std::string &key, const char *def_val)
{
    auto it = dt.find(key);
    if (it == dt.end())
    {
        return def_val;
    }
    return it->second;
}

const std::string &get_string(const dict &dt, const std::string &key, const std::string &def_val)
{
    auto it = dt.find(key);
    if (it == dt.end())
    {
        return def_val;
    }
    return it->second;
}

bool get_bool(const dict &dt, const char *key, bool def_val)
{
    const char *val = get_cstring(dt, key, nullptr);
    if (!val)
    {
        return def_val;
    }
    return str_to_bool(val, def_val);
}

bool get_bool(const dict &dt, const std::string &key, bool def_val)
{
    const char *val = get_cstring(dt, key, nullptr);
    if (!val)
    {
        return def_val;
    }
    return str_to_bool(val, def_val);
}

int get_int(const dict &dt, const char *key, int def_val)
{
    const char *val = get_cstring(dt, key, nullptr);
    if (!val)
    {
        return def_val;
    }
    return std::atoi(val);
}

int get_int(const dict &dt, const std::string &key, int def_val)
{
    const char *val = get_cstring(dt, key, nullptr);
    if (!val)
    {
        return def_val;
    }
    return std::atoi(val);
}

int64_t get_long(const dict &dt, const char *key, int64_t def_val)
{
    const char *val = get_cstring(dt, key, nullptr);
    if (!val)
    {
        return def_val;
    }
    return std::atol(val);
}

int64_t get_long(const dict &dt, const std::string &key, int64_t def_val)
{
    const char *val = get_cstring(dt, key, nullptr);
    if (!val)
    {
        return def_val;
    }
    return std::atol(val);
}

int64_t get_second(const dict &dt, const char *key, int64_t def_val)
{
    const char *val = get_cstring(dt, key, nullptr);
    if (!val)
    {
        return def_val;
    }
    return str_to_second(val, def_val);
}

int64_t get_second(const dict &dt, const std::string &key, int64_t def_val)
{
    const char *val = get_cstring(dt, key, nullptr);
    if (!val)
    {
        return def_val;
    }
    return str_to_second(val, def_val);
}

int64_t get_size(const dict &dt, const char *key, int64_t def_val)
{
    const char *val = get_cstring(dt, key, nullptr);
    if (!val)
    {
        return def_val;
    }
    return str_to_size(val, def_val);
}

int64_t get_size(const dict &dt, const std::string &key, int64_t def_val)
{
    const char *val = get_cstring(dt, key, nullptr);
    if (!val)
    {
        return def_val;
    }
    return str_to_size(val, def_val);
}

zcc_namespace_end;

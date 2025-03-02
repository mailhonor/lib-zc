/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-11-12
 * ================================
 */

#include <fstream>
#include "zcc/zcc_stdlib.h"

zcc_namespace_begin;

config var_main_config;

config::config()
{
}

config::~config()
{
}

config &config::reset()
{
    clear();
    return *this;
}

config &config::update(const char *key, const char *val, int vlen)
{
    if (vlen < 0)
    {
        vlen = std::strlen(val);
    }
    (*this)[key] = std::string(val, vlen);
    afterUpdate();
    return *this;
}

config &config::update(const char *key, const std::string &val)
{
    (*this)[key] = val;
    afterUpdate();
    return *this;
}

config &config::update(const std::string &key, const std::string &val)
{
    (*this)[key] = val;
    afterUpdate();
    return *this;
}

config &config::remove(const char *key)
{
    auto it = find(key);
    if (it != end())
    {
        erase(it);
        afterUpdate();
    }
    return *this;
}

config &config::remove(const std::string &key)
{
    auto it = find(key);
    if (it != end())
    {
        erase(it);
        afterUpdate();
    }
    return *this;
}

bool config::load_from_file(const char *pathname)
{
    char blank[1] = "";
    char *name, *value;
    char *line_buf;
    std::ifstream file(pathname, std::ifstream::binary);

    if (!file)
    {
        return false;
    }
    const int limit = 1024000;
    line_buf = new char[limit + 11];
    while (!file.eof())
    {
        line_buf[0];
        file.getline(line_buf, limit);
        if (file.fail())
        {
            break;
        }
        name = trim_left(line_buf);
        if (zcc::empty(name) || (*name == '#'))
        {
            continue;
        }
        value = std::strchr(name, '=');
        if (value)
        {
            *value++ = 0;
        }
        else
        {
            value = blank;
        }
        name = trim_right(name);
        if (!zcc::empty(value))
        {
            value = trim(value);
        }
        update(name, value);
    }
    delete[] line_buf;

    return true;
}

config &config::load_another(config &another)
{
    for (auto it = another.begin(); it != another.end(); it++)
    {
        (*this)[it->first] = it->second;
    }
    return *this;
}

config &config::debug_show()
{
    std::string s;
    for (auto it = begin(); it != end(); it++)
    {
        s.append(it->first).append(" = ").append(it->second).append("\r\n");
    }
    std::fprintf(stderr, "%s\n", s.c_str());
    return *this;
}

std::string *config::get_value(const char *key)
{
    auto it = find(key);
    if (it == end())
    {
        return nullptr;
    }
    return &(it->second);
}

std::string *config::get_value(const std::string &key)
{
    auto it = find(key);
    if (it == end())
    {
        return nullptr;
    }
    return &(it->second);
}

const char *config::get_cstring(const char *key, const char *def_val)
{
    auto it = find(key);
    if (it == end())
    {
        return def_val;
    }
    return it->second.c_str();
}

const char *config::get_cstring(const std::string &key, const char *def_val)
{
    auto it = find(key);
    if (it == end())
    {
        return def_val;
    }
    return it->second.c_str();
}

std::string config::get_string(const char *key, const char *def_val)
{
    auto it = find(key);
    if (it == end())
    {
        return def_val;
    }
    return it->second;
}

std::string config::get_string(const std::string &key, const char *def_val)
{
    auto it = find(key);
    if (it == end())
    {
        return def_val;
    }
    return it->second;
}

const std::string &config::get_string(const std::string &key, const std::string &def_val)
{
    auto it = find(key);
    if (it == end())
    {
        return def_val;
    }
    return it->second;
}

bool config::get_bool(const char *key, bool def_val)
{
    std::string *val = get_value(key);
    if (!val)
    {
        return def_val;
    }
    return str_to_bool(val->c_str(), def_val);
}

bool config::get_bool(const std::string &key, bool def_val)
{
    std::string *val = get_value(key);
    if (!val)
    {
        return def_val;
    }
    return str_to_bool(val->c_str(), def_val);
}

int config::get_int(const char *key, int def_val)
{
    std::string *val = get_value(key);
    if (!val)
    {
        return def_val;
    }
    return std::atoi(val->c_str());
}

int config::get_int(const std::string &key, int def_val)
{
    std::string *val = get_value(key);
    if (!val)
    {
        return def_val;
    }
    return std::atoi(val->c_str());
}

int64_t config::get_long(const char *key, int64_t def_val)
{
    std::string *val = get_value(key);
    if (!val)
    {
        return def_val;
    }
    return std::atol(val->c_str());
}

int64_t config::get_long(const std::string &key, int64_t def_val)
{
    std::string *val = get_value(key);
    if (!val)
    {
        return def_val;
    }
    return std::atol(val->c_str());
}

int64_t config::get_second(const char *key, int64_t def_val)
{
    std::string *val = get_value(key);
    if (!val)
    {
        return def_val;
    }
    return str_to_second(val->c_str(), def_val);
}

int64_t config::get_second(const std::string &key, int64_t def_val)
{
    std::string *val = get_value(key);
    if (!val)
    {
        return def_val;
    }
    return str_to_second(val->c_str(), def_val);
}

int64_t config::get_size(const char *key, int64_t def_val)
{
    std::string *val = get_value(key);
    if (!val)
    {
        return def_val;
    }
    return str_to_size(val->c_str(), def_val);
}

int64_t config::get_size(const std::string &key, int64_t def_val)
{
    std::string *val = get_value(key);
    if (!val)
    {
        return def_val;
    }
    return str_to_size(val->c_str(), def_val);
}

zcc_namespace_end;

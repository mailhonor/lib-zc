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

/**
 * @brief 全局配置对象
 */
config var_main_config;

/**
 * @brief 配置类的构造函数
 *
 * 目前构造函数为空，不执行任何初始化操作
 */
config::config()
{
}

/**
 * @brief 配置类的析构函数
 *
 * 目前析构函数为空，不执行任何清理操作
 */
config::~config()
{
}

/**
 * @brief 重置配置对象，清除所有配置项
 *
 * @return config& 返回当前配置对象的引用，支持链式调用
 */
config &config::reset()
{
    // 调用 clear 方法清除所有配置项
    clear();
    return *this;
}

/**
 * @brief 使用 C 风格字符串更新配置项
 *
 * @param key 配置项的键
 * @param val 配置项的值
 * @param vlen 值的长度，如果为 -1，则自动计算字符串长度
 * @return config& 返回当前配置对象的引用，支持链式调用
 */
config &config::update(const char *key, const char *val, int vlen)
{
    // 如果未指定值的长度，则自动计算
    if (vlen < 0)
    {
        vlen = std::strlen(val);
    }
    // 将配置项添加或更新到配置对象中
    (*this)[key] = std::string(val, vlen);
    // 调用更新后的处理函数
    afterUpdate();
    return *this;
}

config &config::update(const std::string &key, const char *val, int vlen)
{
    // 如果未指定值的长度，则自动计算
    if (vlen < 0)
    {
        vlen = std::strlen(val);
    }
    // 将配置项添加或更新到配置对象中
    (*this)[key] = std::string(val, vlen);
    // 调用更新后的处理函数
    afterUpdate();
    return *this;
}

/**
 * @brief 使用 std::string 和 C 风格字符串更新配置项
 *
 * @param key 配置项的键
 * @param val 配置项的值
 * @return config& 返回当前配置对象的引用，支持链式调用
 */
config &config::update(const char *key, const std::string &val)
{
    // 将配置项添加或更新到配置对象中
    (*this)[key] = val;
    // 调用更新后的处理函数
    afterUpdate();
    return *this;
}

/**
 * @brief 使用两个 std::string 更新配置项
 *
 * @param key 配置项的键
 * @param val 配置项的值
 * @return config& 返回当前配置对象的引用，支持链式调用
 */
config &config::update(const std::string &key, const std::string &val)
{
    // 将配置项添加或更新到配置对象中
    (*this)[key] = val;
    // 调用更新后的处理函数
    afterUpdate();
    return *this;
}

/**
 * @brief 使用 C 风格字符串移除配置项
 *
 * @param key 要移除的配置项的键
 * @return config& 返回当前配置对象的引用，支持链式调用
 */
config &config::remove(const char *key)
{
    // 查找指定键的配置项
    auto it = find(key);
    // 如果找到该配置项
    if (it != end())
    {
        // 从配置对象中移除该配置项
        erase(it);
        // 调用更新后的处理函数
        afterUpdate();
    }
    return *this;
}

/**
 * @brief 使用 std::string 移除配置项
 *
 * @param key 要移除的配置项的键
 * @return config& 返回当前配置对象的引用，支持链式调用
 */
config &config::remove(const std::string &key)
{
    // 查找指定键的配置项
    auto it = find(key);
    // 如果找到该配置项
    if (it != end())
    {
        // 从配置对象中移除该配置项
        erase(it);
        // 调用更新后的处理函数
        afterUpdate();
    }
    return *this;
}

/**
 * @brief 从文件中加载配置项
 *
 * @param pathname 配置文件的路径
 * @return bool 加载成功返回 true，失败返回 false
 */
bool config::load_from_file(const char *pathname)
{
    // 空字符串，用于处理没有值的配置项
    char blank[1] = "";

    // 配置项的名称
    char *name;
    // 配置项的值
    char *value;
    // 用于存储文件读取的行
    char *line_buf;
    // 以二进制模式打开配置文件
    std::ifstream file(pathname, std::ifstream::binary);

    // 如果文件打开失败
    if (!file)
    {
        return false;
    }
    // 每行的最大长度限制
    const int limit = 1024000;
    // 分配内存用于存储每行内容
    line_buf = new char[limit + 11];
    // 逐行读取文件，直到文件结束
    while (!file.eof())
    {
        // 从文件中读取一行
        file.getline(line_buf, limit);
        // 如果读取失败
        if (file.fail())
        {
            break;
        }
        // 去除行首的空白字符
        name = trim_left(line_buf);
        // 如果行为空或者是注释行
        if (zcc::empty(name) || (*name == '#'))
        {
            continue;
        }
        // 查找等号的位置
        value = std::strchr(name, '=');
        if (value)
        {
            *value++ = 0;
        }
        else
        {
            // 如果没有等号，使用空字符串作为值
            value = blank;
        }
        // 去除名称的尾部空白字符
        name = trim_right(name);
        // 如果值不为空，去除值的首尾空白字符
        if (!zcc::empty(value))
        {
            value = trim(value);
        }
        // 更新配置项
        update(name, value);
    }
    // 释放分配的内存
    delete[] line_buf;

    return true;
}

/**
 * @brief 加载另一个配置对象的所有配置项
 *
 * @param another 另一个配置对象
 * @return config& 返回当前配置对象的引用，支持链式调用
 */
config &config::load_another(config &another)
{
    // 遍历另一个配置对象的所有配置项
    for (auto it = another.begin(); it != another.end(); it++)
    {
        // 将配置项添加或更新到当前配置对象中
        (*this)[it->first] = it->second;
    }
    return *this;
}

/**
 * @brief 调试输出所有配置项
 *
 * 将所有配置项以 "key = value" 的格式输出到标准错误流
 * @return config& 返回当前配置对象的引用，支持链式调用
 */
config &config::debug_show()
{
    // 用于存储调试输出的字符串
    std::string s;
    // 遍历所有配置项
    for (auto it = begin(); it != end(); it++)
    {
        // 拼接配置项信息
        s.append(it->first).append(" = ").append(it->second).append("\r\n");
    }
    // 输出调试信息到标准错误流
    std::fprintf(stderr, "%s\n", s.c_str());
    return *this;
}

/**
 * @brief 使用 C 风格字符串获取配置项的值
 *
 * @param key 配置项的键
 * @return std::string* 如果找到配置项，返回其值的指针；否则返回 nullptr
 */
std::string *config::get_value(const char *key)
{
    // 查找指定键的配置项
    auto it = find(key);
    // 如果未找到该配置项
    if (it == end())
    {
        return nullptr;
    }
    return &(it->second);
}

/**
 * @brief 使用 std::string 获取配置项的值
 *
 * @param key 配置项的键
 * @return std::string* 如果找到配置项，返回其值的指针；否则返回 nullptr
 */
std::string *config::get_value(const std::string &key)
{
    // 查找指定键的配置项
    auto it = find(key);
    // 如果未找到该配置项
    if (it == end())
    {
        return nullptr;
    }
    return &(it->second);
}

/**
 * @brief 使用 C 风格字符串获取配置项的 C 风格字符串值
 *
 * @param key 配置项的键
 * @param def_val 默认值，如果未找到配置项，返回该默认值
 * @return const char* 配置项的值或默认值
 */
const char *config::get_cstring(const char *key, const char *def_val)
{
    // 查找指定键的配置项
    auto it = find(key);
    // 如果未找到该配置项
    if (it == end())
    {
        return def_val;
    }
    return it->second.c_str();
}

/**
 * @brief 使用 std::string 获取配置项的 C 风格字符串值
 *
 * @param key 配置项的键
 * @param def_val 默认值，如果未找到配置项，返回该默认值
 * @return const char* 配置项的值或默认值
 */
const char *config::get_cstring(const std::string &key, const char *def_val)
{
    // 查找指定键的配置项
    auto it = find(key);
    // 如果未找到该配置项
    if (it == end())
    {
        return def_val;
    }
    return it->second.c_str();
}

/**
 * @brief 使用 C 风格字符串获取配置项的 std::string 值
 *
 * @param key 配置项的键
 * @param def_val 默认值，如果未找到配置项，返回该默认值
 * @return std::string 配置项的值或默认值
 */
std::string config::get_string(const char *key, const char *def_val)
{
    // 查找指定键的配置项
    auto it = find(key);
    // 如果未找到该配置项
    if (it == end())
    {
        return def_val;
    }
    return it->second;
}

/**
 * @brief 使用 std::string 获取配置项的 std::string 值
 *
 * @param key 配置项的键
 * @param def_val 默认值，如果未找到配置项，返回该默认值
 * @return std::string 配置项的值或默认值
 */
std::string config::get_string(const std::string &key, const char *def_val)
{
    // 查找指定键的配置项
    auto it = find(key);
    // 如果未找到该配置项
    if (it == end())
    {
        return def_val;
    }
    return it->second;
}

/**
 * @brief 使用 std::string 获取配置项的 std::string 引用值
 *
 * @param key 配置项的键
 * @param def_val 默认值，如果未找到配置项，返回该默认值的引用
 * @return const std::string& 配置项的值或默认值的引用
 */
const std::string &config::get_string(const std::string &key, const std::string &def_val)
{
    // 查找指定键的配置项
    auto it = find(key);
    // 如果未找到该配置项
    if (it == end())
    {
        return def_val;
    }
    return it->second;
}

/**
 * @brief 使用 C 风格字符串获取配置项的布尔值
 *
 * @param key 配置项的键
 * @param def_val 默认值，如果未找到配置项，返回该默认值
 * @return bool 配置项的布尔值或默认值
 */
bool config::get_bool(const char *key, bool def_val)
{
    // 获取配置项的值
    std::string *val = get_value(key);
    // 如果未找到该配置项
    if (!val)
    {
        return def_val;
    }
    // 将字符串转换为布尔值
    return str_to_bool(val->c_str(), def_val);
}

/**
 * @brief 使用 std::string 获取配置项的布尔值
 *
 * @param key 配置项的键
 * @param def_val 默认值，如果未找到配置项，返回该默认值
 * @return bool 配置项的布尔值或默认值
 */
bool config::get_bool(const std::string &key, bool def_val)
{
    // 获取配置项的值
    std::string *val = get_value(key);
    // 如果未找到该配置项
    if (!val)
    {
        return def_val;
    }
    // 将字符串转换为布尔值
    return str_to_bool(val->c_str(), def_val);
}

/**
 * @brief 使用 C 风格字符串获取配置项的整数值
 *
 * @param key 配置项的键
 * @param def_val 默认值，如果未找到配置项，返回该默认值
 * @return int 配置项的整数值或默认值
 */
int config::get_int(const char *key, int def_val)
{
    // 获取配置项的值
    std::string *val = get_value(key);
    // 如果未找到该配置项
    if (!val)
    {
        return def_val;
    }
    // 将字符串转换为整数
    return std::atoi(val->c_str());
}

/**
 * @brief 使用 std::string 获取配置项的整数值
 *
 * @param key 配置项的键
 * @param def_val 默认值，如果未找到配置项，返回该默认值
 * @return int 配置项的整数值或默认值
 */
int config::get_int(const std::string &key, int def_val)
{
    // 获取配置项的值
    std::string *val = get_value(key);
    // 如果未找到该配置项
    if (!val)
    {
        return def_val;
    }
    // 将字符串转换为整数
    return std::atoi(val->c_str());
}

/**
 * @brief 使用 C 风格字符串获取配置项的长整数值
 *
 * @param key 配置项的键
 * @param def_val 默认值，如果未找到配置项，返回该默认值
 * @return int64_t 配置项的长整数值或默认值
 */
int64_t config::get_long(const char *key, int64_t def_val)
{
    // 获取配置项的值
    std::string *val = get_value(key);
    // 如果未找到该配置项
    if (!val)
    {
        return def_val;
    }
    // 将字符串转换为长整数
    return std::atol(val->c_str());
}

/**
 * @brief 使用 std::string 获取配置项的长整数值
 *
 * @param key 配置项的键
 * @param def_val 默认值，如果未找到配置项，返回该默认值
 * @return int64_t 配置项的长整数值或默认值
 */
int64_t config::get_long(const std::string &key, int64_t def_val)
{
    // 获取配置项的值
    std::string *val = get_value(key);
    // 如果未找到该配置项
    if (!val)
    {
        return def_val;
    }
    // 将字符串转换为长整数
    return std::atol(val->c_str());
}

/**
 * @brief 使用 C 风格字符串获取配置项的秒数
 *
 * @param key 配置项的键
 * @param def_val 默认值，如果未找到配置项，返回该默认值
 * @return int64_t 配置项的秒数或默认值
 */
int64_t config::get_second(const char *key, int64_t def_val)
{
    // 获取配置项的值
    std::string *val = get_value(key);
    // 如果未找到该配置项
    if (!val)
    {
        return def_val;
    }
    // 将字符串转换为秒数
    return str_to_second(val->c_str(), def_val);
}

/**
 * @brief 使用 std::string 获取配置项的秒数
 *
 * @param key 配置项的键
 * @param def_val 默认值，如果未找到配置项，返回该默认值
 * @return int64_t 配置项的秒数或默认值
 */
int64_t config::get_second(const std::string &key, int64_t def_val)
{
    // 获取配置项的值
    std::string *val = get_value(key);
    // 如果未找到该配置项
    if (!val)
    {
        return def_val;
    }
    // 将字符串转换为秒数
    return str_to_second(val->c_str(), def_val);
}

/**
 * @brief 使用 C 风格字符串获取配置项的大小值
 *
 * @param key 配置项的键
 * @param def_val 默认值，如果未找到配置项，返回该默认值
 * @return int64_t 配置项的大小值或默认值
 */
int64_t config::get_size(const char *key, int64_t def_val)
{
    // 获取配置项的值
    std::string *val = get_value(key);
    // 如果未找到该配置项
    if (!val)
    {
        return def_val;
    }
    // 将字符串转换为大小值
    return str_to_size(val->c_str(), def_val);
}

/**
 * @brief 使用 std::string 获取配置项的大小值
 *
 * @param key 配置项的键
 * @param def_val 默认值，如果未找到配置项，返回该默认值
 * @return int64_t 配置项的大小值或默认值
 */
int64_t config::get_size(const std::string &key, int64_t def_val)
{
    // 获取配置项的值
    std::string *val = get_value(key);
    // 如果未找到该配置项
    if (!val)
    {
        return def_val;
    }
    // 将字符串转换为大小值
    return str_to_size(val->c_str(), def_val);
}

zcc_namespace_end;

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

/**
 * @brief 调试输出字典内容
 * 
 * 遍历字典中的所有键值对，将其格式化为 "key = value" 的形式，
 * 并将结果输出到标准错误流中。
 * 
 * @param dt 要调试输出的字典对象
 */
void debug_show(const dict &dt)
{
    // 用于存储格式化后的字典内容
    std::string s;
    // 遍历字典中的所有键值对
    for (auto it = dt.begin(); it != dt.end(); it++)
    {
        // 拼接键值对信息
        s.append(it->first).append(" = ").append(it->second).append("\r\n");
    }
    // 将格式化后的内容输出到标准错误流
    std::fprintf(stderr, "%s\n", s.c_str());
}

/**
 * @brief 从字典中获取 C 风格字符串值
 * 
 * 如果字典中存在指定的键，则返回对应的值；否则返回默认值。
 * 
 * @param dt 要查询的字典对象
 * @param key 要查询的键，使用 C 风格字符串表示
 * @param def_val 默认值，当字典中不存在指定键时返回
 * @return const char* 字典中对应键的值或默认值
 */
const char *get_cstring(const dict &dt, const char *key, const char *def_val)
{
    // 在字典中查找指定的键
    auto it = dt.find(key);
    // 如果未找到该键
    if (it == dt.end())
    {
        return def_val;
    }
    // 返回对应的值
    return it->second.c_str();
}

/**
 * @brief 从字典中获取 C 风格字符串值
 * 
 * 如果字典中存在指定的键，则返回对应的值；否则返回默认值。
 * 
 * @param dt 要查询的字典对象
 * @param key 要查询的键，使用 std::string 表示
 * @param def_val 默认值，当字典中不存在指定键时返回
 * @return const char* 字典中对应键的值或默认值
 */
const char *get_cstring(const dict &dt, const std::string &key, const char *def_val)
{
    // 在字典中查找指定的键
    auto it = dt.find(key);
    // 如果未找到该键
    if (it == dt.end())
    {
        return def_val;
    }
    // 返回对应的值
    return it->second.c_str();
}

/**
 * @brief 从字典中获取 std::string 类型的值
 * 
 * 如果字典中存在指定的键，则返回对应的值；否则返回默认值。
 * 
 * @param dt 要查询的字典对象
 * @param key 要查询的键，使用 C 风格字符串表示
 * @param def_val 默认值，当字典中不存在指定键时返回
 * @return std::string 字典中对应键的值或默认值
 */
std::string get_string(const dict &dt, const char *key, const char *def_val)
{
    // 在字典中查找指定的键
    auto it = dt.find(key);
    // 如果未找到该键
    if (it == dt.end())
    {
        return def_val;
    }
    // 返回对应的值
    return it->second;
}

/**
 * @brief 从字典中获取 std::string 类型的值的引用
 * 
 * 如果字典中存在指定的键，则返回对应的值的引用；否则返回默认值的引用。
 * 
 * @param dt 要查询的字典对象
 * @param key 要查询的键，使用 std::string 表示
 * @param def_val 默认值，当字典中不存在指定键时返回
 * @return const std::string& 字典中对应键的值或默认值的引用
 */
const std::string &get_string(const dict &dt, const std::string &key, const std::string &def_val)
{
    // 在字典中查找指定的键
    auto it = dt.find(key);
    // 如果未找到该键
    if (it == dt.end())
    {
        return def_val;
    }
    // 返回对应的值的引用
    return it->second;
}

/**
 * @brief 从字典中获取布尔类型的值
 * 
 * 如果字典中存在指定的键，则将对应的值转换为布尔值；否则返回默认值。
 * 
 * @param dt 要查询的字典对象
 * @param key 要查询的键，使用 C 风格字符串表示
 * @param def_val 默认值，当字典中不存在指定键时返回
 * @return bool 转换后的布尔值或默认值
 */
bool get_bool(const dict &dt, const char *key, bool def_val)
{
    // 从字典中获取 C 风格字符串值
    const char *val = get_cstring(dt, key, nullptr);
    // 如果未获取到值
    if (!val)
    {
        return def_val;
    }
    // 将字符串转换为布尔值
    return str_to_bool(val, def_val);
}

/**
 * @brief 从字典中获取布尔类型的值
 * 
 * 如果字典中存在指定的键，则将对应的值转换为布尔值；否则返回默认值。
 * 
 * @param dt 要查询的字典对象
 * @param key 要查询的键，使用 std::string 表示
 * @param def_val 默认值，当字典中不存在指定键时返回
 * @return bool 转换后的布尔值或默认值
 */
bool get_bool(const dict &dt, const std::string &key, bool def_val)
{
    // 从字典中获取 C 风格字符串值
    const char *val = get_cstring(dt, key, nullptr);
    // 如果未获取到值
    if (!val)
    {
        return def_val;
    }
    // 将字符串转换为布尔值
    return str_to_bool(val, def_val);
}

/**
 * @brief 从字典中获取整数类型的值
 * 
 * 如果字典中存在指定的键，则将对应的值转换为整数；否则返回默认值。
 * 
 * @param dt 要查询的字典对象
 * @param key 要查询的键，使用 C 风格字符串表示
 * @param def_val 默认值，当字典中不存在指定键时返回
 * @return int 转换后的整数值或默认值
 */
int get_int(const dict &dt, const char *key, int def_val)
{
    // 从字典中获取 C 风格字符串值
    const char *val = get_cstring(dt, key, nullptr);
    // 如果未获取到值
    if (!val)
    {
        return def_val;
    }
    // 将字符串转换为整数
    return std::atoi(val);
}

/**
 * @brief 从字典中获取整数类型的值
 * 
 * 如果字典中存在指定的键，则将对应的值转换为整数；否则返回默认值。
 * 
 * @param dt 要查询的字典对象
 * @param key 要查询的键，使用 std::string 表示
 * @param def_val 默认值，当字典中不存在指定键时返回
 * @return int 转换后的整数值或默认值
 */
int get_int(const dict &dt, const std::string &key, int def_val)
{
    // 从字典中获取 C 风格字符串值
    const char *val = get_cstring(dt, key, nullptr);
    // 如果未获取到值
    if (!val)
    {
        return def_val;
    }
    // 将字符串转换为整数
    return std::atoi(val);
}

/**
 * @brief 从字典中获取长整数类型的值
 * 
 * 如果字典中存在指定的键，则将对应的值转换为长整数；否则返回默认值。
 * 
 * @param dt 要查询的字典对象
 * @param key 要查询的键，使用 C 风格字符串表示
 * @param def_val 默认值，当字典中不存在指定键时返回
 * @return int64_t 转换后的长整数值或默认值
 */
int64_t get_long(const dict &dt, const char *key, int64_t def_val)
{
    // 从字典中获取 C 风格字符串值
    const char *val = get_cstring(dt, key, nullptr);
    // 如果未获取到值
    if (!val)
    {
        return def_val;
    }
    // 将字符串转换为长整数
    return std::atol(val);
}

/**
 * @brief 从字典中获取长整数类型的值
 * 
 * 如果字典中存在指定的键，则将对应的值转换为长整数；否则返回默认值。
 * 
 * @param dt 要查询的字典对象
 * @param key 要查询的键，使用 std::string 表示
 * @param def_val 默认值，当字典中不存在指定键时返回
 * @return int64_t 转换后的长整数值或默认值
 */
int64_t get_long(const dict &dt, const std::string &key, int64_t def_val)
{
    // 从字典中获取 C 风格字符串值
    const char *val = get_cstring(dt, key, nullptr);
    // 如果未获取到值
    if (!val)
    {
        return def_val;
    }
    // 将字符串转换为长整数
    return std::atol(val);
}

/**
 * @brief 从字典中获取秒数类型的值
 * 
 * 如果字典中存在指定的键，则将对应的值转换为秒数；否则返回默认值。
 * 
 * @param dt 要查询的字典对象
 * @param key 要查询的键，使用 C 风格字符串表示
 * @param def_val 默认值，当字典中不存在指定键时返回
 * @return int64_t 转换后的秒数值或默认值
 */
int64_t get_second(const dict &dt, const char *key, int64_t def_val)
{
    // 从字典中获取 C 风格字符串值
    const char *val = get_cstring(dt, key, nullptr);
    // 如果未获取到值
    if (!val)
    {
        return def_val;
    }
    // 将字符串转换为秒数
    return str_to_second(val, def_val);
}

/**
 * @brief 从字典中获取秒数类型的值
 * 
 * 如果字典中存在指定的键，则将对应的值转换为秒数；否则返回默认值。
 * 
 * @param dt 要查询的字典对象
 * @param key 要查询的键，使用 std::string 表示
 * @param def_val 默认值，当字典中不存在指定键时返回
 * @return int64_t 转换后的秒数值或默认值
 */
int64_t get_second(const dict &dt, const std::string &key, int64_t def_val)
{
    // 从字典中获取 C 风格字符串值
    const char *val = get_cstring(dt, key, nullptr);
    // 如果未获取到值
    if (!val)
    {
        return def_val;
    }
    // 将字符串转换为秒数
    return str_to_second(val, def_val);
}

/**
 * @brief 从字典中获取大小类型的值
 * 
 * 如果字典中存在指定的键，则将对应的值转换为大小值；否则返回默认值。
 * 
 * @param dt 要查询的字典对象
 * @param key 要查询的键，使用 C 风格字符串表示
 * @param def_val 默认值，当字典中不存在指定键时返回
 * @return int64_t 转换后的大小值或默认值
 */
int64_t get_size(const dict &dt, const char *key, int64_t def_val)
{
    // 从字典中获取 C 风格字符串值
    const char *val = get_cstring(dt, key, nullptr);
    // 如果未获取到值
    if (!val)
    {
        return def_val;
    }
    // 将字符串转换为大小值
    return str_to_size(val, def_val);
}

/**
 * @brief 从字典中获取大小类型的值
 * 
 * 如果字典中存在指定的键，则将对应的值转换为大小值；否则返回默认值。
 * 
 * @param dt 要查询的字典对象
 * @param key 要查询的键，使用 std::string 表示
 * @param def_val 默认值，当字典中不存在指定键时返回
 * @return int64_t 转换后的大小值或默认值
 */
int64_t get_size(const dict &dt, const std::string &key, int64_t def_val)
{
    // 从字典中获取 C 风格字符串值
    const char *val = get_cstring(dt, key, nullptr);
    // 如果未获取到值
    if (!val)
    {
        return def_val;
    }
    // 将字符串转换为大小值
    return str_to_size(val, def_val);
}

zcc_namespace_end;

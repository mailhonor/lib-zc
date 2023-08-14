/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2017-08-11
 * ================================
 */

#pragma once

#ifndef ___ZC_LIB_INCLUDE_JSON___
#define ___ZC_LIB_INCLUDE_JSON___

#include <string>
#include <vector>
#include <map>

#ifndef zinline
#define zinline inline __attribute__((always_inline))
#endif

#define _namespace_begin \
    namespace zcc           \
    {
#define _namespace_end }

_namespace_begin;
#pragma pack(push, 1)

/* 宏 */
#define ZCC_JSON_ARRAY_WALK_BEGIN(js, child_js_p)                                                                            \
    {                                                                                                                        \
        auto const TMP_JS_0531_VECTOR = (js).get_array_value();                                                              \
        for (auto TMP_JS_0531_it = TMP_JS_0531_VECTOR.begin(); TMP_JS_0531_it != TMP_JS_0531_VECTOR.end(); TMP_JS_0531_it++) \
        {                                                                                                                    \
            auto &child_js_p = *TMP_JS_0531_it;                                                                              \
            {
#define ZCC_JSON_ARRAY_WALK_END \
    }                           \
    }                           \
    }

class json;
const unsigned char json_type_null = 0;
const unsigned char json_type_string = 1;
const unsigned char json_type_long = 2;
const unsigned char json_type_double = 3;
const unsigned char json_type_object = 4;
const unsigned char json_type_array = 5;
const unsigned char json_type_bool = 6;
const unsigned char json_type_unknown = 7;
const int json_serialize_strict = 0X01;
const int json_serialize_pretty = 0X02;
class json
{
public:
    json();
    json(const std::string &val);
    json(const char *val, int len = -1);
    json(ssize_t val);
#ifdef _WIN32
    json(long val);
#endif // _WIN32
    json(double val);
    json(bool val);
    json(const unsigned char type);
    ~json();

    /* 深度递归复制 */
    json *deep_copy();

    /* 首先重置本json为 null, 然后从文件加载json */
    bool load_from_pathname(const char *pathname);

    /* 首先重置本json为 null, 然后从jstr反序列化为json */
    bool unserialize(const char *jstr, int jsize = -1);
    zinline bool unserialize(const std::string &jstr) { return unserialize(jstr.c_str(), (int)(jstr.size())); }

    /* 序列化 */
    json *serialize(std::string &result, int flags = 0 /* json_serialize_XXX */);

    /* 类型 */
    zinline int get_type() { return type_; }
    zinline bool is_string() { return type_ == json_type_string; }
    zinline bool is_long() { return type_ == json_type_long; }
    zinline bool is_double() { return type_ == json_type_double; }
    zinline bool is_object() { return type_ == json_type_object; }
    zinline bool is_array() { return type_ == json_type_array; }
    zinline bool is_bool() { return type_ == json_type_bool; }
    zinline bool is_null() { return type_ == json_type_null; }

    /* 重置为 null */
    json *reset();

    /* 改变类型为 bool, 其他类似 */
    json *used_for_bool();
    json *used_for_long();
    json *used_for_double();
    json *used_for_string();
    json *used_for_array();
    json *used_for_object();

    /* 获取 string 值; 如果不是 string 类型, 则首先转换为 string 类型, 其值:
     * 原来是 null, 则为 def
     * 原来是 bool, 则为 "0" 或  "1"
     * 原来是 long, 则为 打印结果
     * 原来是 double, 则为 打印结果
     * 原来是 array, 则为 def
     * 原来是 object, 则为 def
     * */
    std::string &get_string_value(const char *def = "");

    /* 获取 long 值; 如果不是 long 类型, 则首先转换为 long 类型, 其值:
     * 原来是 null, 则为 def
     * 原来是 string, 则为 atol(val)
     * 原来是 bool, 则为 0 或  1
     * 原来是 double, 则为 long(val)
     * 原来是 array, 则为 def
     * 原来是 object, 则为 def
     * */
    ssize_t &get_long_value(ssize_t def = 0);

    /* 获取 double 值; 如果不是 double 类型, 则首先转换为 double 类型, 其值:
     * 原来是 null, 则为 def
     * 原来是 string, 则为 atof(val)
     * 原来是 bool, 则为 0.0 或  1.0
     * 原来是 long, 则为 打印结果
     * 原来是 array, 则为 def
     * 原来是 object, 则为 def
     * */
    double &get_double_value(double def = 0.0);

    /* 获取 bool 值; 如果不是 bool 类型, 则首先转换为 bool 类型, 其值:
     * 原来是 null, 则为 def
     * 原来是 string, 则:
     *         "t*" => true
     *         "T*" => true
     *         "y*" => true
     *         "Y*" => true
     *         "f*" => false
     *         "F*" => false
     *         "n*" => false
     *         "N*" => false
     *         "1" => true
     *         "0" => false
     *         其他 => def
     * 原来是 long/double, 则, 0 为 false, 否则 true
     * 原来是 array, 则为 def
     * 原来是 object, 则为 def
     * */
    bool &get_bool_value(bool def = false);

    /* 如果想操作下面的vector/map, 记得给新 json 设置 set_parent */

    /* 获取 array 值; 如果不是 array 类型, 则首先转换为 array 类型, 默认为 [] */
    const std::vector<json *> &get_array_value();

    /* 获取 object 值; 如果不是 object 类型, 则首先转换为 object 类型, 默认为 {} */
    const std::map<std::string, json *> &get_object_value();

    /* 设置值 */
    json *set_string_value(const char *val, int len = -1);
    zinline json *set_string_value(const std::string &val)
    {
        get_string_value() = val;
        return this;
    }
    zinline json *set_long_value(ssize_t val)
    {
        get_long_value() = val;
        return this;
    }
    zinline json *set_double_value(double val)
    {
        get_double_value() = val;
        return this;
    }
    zinline json *set_bool_value(bool val)
    {
        get_bool_value() = val;
        return this;
    }

    zinline json *set_value(const char *val, int len = -1) { return set_string_value(val, len); }
    zinline json *set_value(const std::string &val)
    {
        get_string_value() = val;
        return this;
    }
    zinline json *set_value(ssize_t val)
    {
        get_long_value() = val;
        return this;
    }
#ifdef _WIN32
    zinline json *set_value(long val)
    {
        get_long_value() = val;
        return this;
    }
#endif // _WIN32
    zinline json *set_value(double val)
    {
        get_double_value() = val;
        return this;
    }
    zinline json *set_value(bool val)
    {
        get_bool_value() = val;
        return this;
    }

    /* 获取下标为 idx 的 json 节点, 如果不是 array 类型, 则先转为 array 类型 */
    json *array_get(int idx);
    json &array_get_or_create(int idx);

    /* 获取 array 节点的个数, 如上 */
    int array_size();

    /* 在 idx 前, 插入 j */
    json *array_insert(int idx, json *j, bool return_child = false);

    /* 在最前面插入 j */
    zinline json *array_unshift(json *j, bool return_child = false)
    {
        return array_insert(0, j, return_child);
    }

    /* 在尾部追加节点 j */
    zinline json *array_add(json *j, bool return_child = false)
    {
        return array_insert(-1, j, return_child);
    }
    zinline json *array_push(json *j, bool return_child = false) { return array_add(j, return_child); }
    zinline json *array_add(const char *val, int len) { return array_add(new json(val, len)); }
    zinline json *array_push(const char *val, int len) { return array_add(new json(val, len)); }

    /* 设置下表为 idx 的成员 j, 如果键idx存在则, 则把idx对应的json赋给 *old, 如果old为0, 则销毁 */
    json *array_update(int idx, json *j, json **old, bool return_child = false);
    zinline json *array_update(int idx, json *j, bool return_child = false)
    {
        return array_update(idx, j, 0, return_child);
    }
    zinline json *array_update(size_t idx, const char *val, int len, bool return_child = false)
    {
        return array_update(idx, new json(val, len), return_child);
    }
    zinline json *array_update(size_t idx, const char *val, int len, json **old, bool return_child = false)
    {
        return array_update(idx, new json(val, len), old, return_child);
    }

    /* 删除下标为 idx的节点, 存在则返回true, 赋值给 *old, 如果 old为 0, 则销毁 */
    bool array_delete(int idx, json **old = 0);

    /* 弹出第一个节点, 存在则返回true, 赋值给 *old, 如果 old为 0, 则销毁 */
    zinline bool array_shift(json **old = 0) { return array_delete(0, old); }

    /* 弹出最后一个节点, 存在则返回true, 赋值给 *old, 如果 old为 0, 则销毁 */
    zinline bool array_pop(json **old = 0) { return array_delete(-1, old); }

    /* 获取键为 key 的json节点, 如果不是 object 类型, 则先转为 object 类型 */
    json *object_get(const char *key);
    json *object_get(const std::string &key);
    json &object_get_or_create(const char *key);
    json &object_get_or_create(const std::string &key);

    /* 获取 object 节点的个数, 如上 */
    int object_size();

    /* 更新键为key的节点, 旧值赋值给 *old, 如果 old为 0, 则销毁 */
    json *object_update(const std::string &key, json *j, json **old, bool return_child = false);
    json *object_add(const std::string &key, json *j, json **old, bool return_child = false)
    {
        return object_update(key, j, old, return_child);
    }

    zinline json *object_update(const std::string &key, json *j, bool return_child = false)
    {
        return object_update(key, j, 0, return_child);
    }
    zinline json *object_add(const std::string &key, json *j, bool return_child = false)
    {
        return object_update(key, j, 0, return_child);
    }
    zinline json *object_update(const std::string &key, const char *val, int len, json **old, bool return_child = false)
    {
        return object_update(key, new json(val, len), old, return_child);
    }
    zinline json *object_add(const std::string &key, const char *val, int len, json **old, bool return_child = false)
    {
        return object_update(key, new json(val, len), old, return_child);
    }
    zinline json *object_update(const std::string &key, const char *val, int len, bool return_child = false)
    {
        return object_update(key, new json(val, len), return_child);
    }
    zinline json *object_add(const std::string &key, const char *val, int len, bool return_child = false)
    {
        return object_update(key, new json(val, len), return_child);
    }

    /* 删除键为key的节点, 存在则返回true, 赋值给 *old, 如果 old为 0, 则销毁 */
    bool object_delete(const char *key, json **old);

#define ___zcc_json_update(TTT)                                                                         \
    json *array_insert(int idx, TTT val, bool return_child = false)                                     \
    {                                                                                                   \
        return array_insert(idx, new json(val), return_child);                                          \
    }                                                                                                   \
    zinline json *array_unshift(TTT val, bool return_child = false)                                     \
    {                                                                                                   \
        return array_insert(0, new json(val), return_child);                                            \
    }                                                                                                   \
    zinline json *array_add(TTT val, bool return_child = false)                                         \
    {                                                                                                   \
        return array_insert(-1, new json(val), return_child);                                           \
    }                                                                                                   \
    zinline json *array_push(TTT val, bool return_child = false)                                        \
    {                                                                                                   \
        return array_insert(-1, new json(val), return_child);                                           \
    }                                                                                                   \
    zinline json *array_update(int idx, TTT val, bool return_child = false)                             \
    {                                                                                                   \
        return array_update(idx, new json(val), return_child);                                          \
    }                                                                                                   \
    zinline json *array_update(int idx, TTT val, json **old, bool return_child = false)                 \
    {                                                                                                   \
        return array_update(idx, new json(val), old, return_child);                                     \
    }                                                                                                   \
    zinline json *object_update(const std::string &key, TTT val, bool return_child = false)             \
    {                                                                                                   \
        return object_update(key, new json(val), return_child);                                         \
    }                                                                                                   \
    zinline json *object_update(const std::string &key, TTT val, json **old, bool return_child = false) \
    {                                                                                                   \
        return object_update(key, new json(val), old, return_child);                                    \
    }                                                                                                   \
    zinline json *object_add(const std::string &key, TTT val, bool return_child = false)                \
    {                                                                                                   \
        return object_update(key, new json(val), 0, return_child);                                      \
    }                                                                                                   \
    zinline json *object_add(const std::string &key, TTT val, json **old, bool return_child = false)    \
    {                                                                                                   \
        return object_update(key, new json(val), old, return_child);                                    \
    }                                                                                                   \
    zinline void operator=(TTT val) { set_value(val); }

    /* 这几组方法类似 array_update, object_update, 只不过参数不同 */
    ___zcc_json_update(const std::string &);
    ___zcc_json_update(const char *);
    ___zcc_json_update(ssize_t);
#ifdef _WIN32
    ___zcc_json_update(long);
#endif // _WIN32
    ___zcc_json_update(double);
    ___zcc_json_update(bool);
#undef ___zcc_json_update

    /* */
    json *mv_value(json *val);
    json *mv_value(json &val);
    zinline void operator=(json *val) { mv_value(val); }
    zinline void operator=(json val) { mv_value(val); }

    /* */
    zinline json &operator[](const std::string &key) { return object_get_or_create(key); }
    zinline json &operator[](int idx) { return array_get_or_create(idx); }

    /* @get_by_path 得到路径path对应的j son, 并返回, 如:
     * 已知 json {group:{linux:[{}, {}, {me: {age:18, sex:"male"}}}}, 则
     * get_by_path("group/linux/2/me") 返回的 应该是 {age:18, sex:"male"} */
    json *get_by_path(const char *path);

    /* 如: get_by_path_vec("group", "linux", "2", "me", 0); */
    json *get_by_path_vec(const char *path0, ...);

    /* 获取 路径 pathname 对应的节点 的 值 */
    bool get_value_by_path(const char *pathname, std::string &value);
    bool get_value_by_path(const char *pathname, ssize_t *value);

    /* */
    zinline json *get_parent() { return parent_; }
    json *get_top();
    json *set_parent(json *js);
    json *debug_show();

    /* 扩展 */
    /* 如果不是 object 类型, 则先转为 object 类型 */
    std::string object_get_string_value(const std::string &key, const char *def = "");
    ssize_t object_get_long_value(const std::string &key, ssize_t def = 0);
    bool object_get_bool_value(const std::string &key, bool def = false);

private:
    unsigned char type_;
    union
    {
        bool b;
        ssize_t l;
        double d;
        char s[sizeof(std::string)];
        std::vector<json *> *v;
        std::map<std::string, json *> *m;
    } val_;
    json *parent_;
};

#pragma pack(pop)
_namespace_end;
#endif /*___ZC_LIB_INCLUDE_JSON___ */


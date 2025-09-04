/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2017-02-18
 * ================================
 */

#pragma once

#ifndef ZCC_LIB_INCLUDE_JSON___
#define ZCC_LIB_INCLUDE_JSON___

#include <vector>
#include "./zcc_stdlib.h"

#ifdef __cplusplus
#pragma pack(push, 1)
zcc_namespace_begin;

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
    static inline json *create_object()
    {
        return new json(json_type_object);
    }
    static inline json *create_array()
    {
        return new json(json_type_array);
    }
    static inline json *create_string()
    {
        return new json(json_type_string);
    }
    static inline json *create_long()
    {
        return new json(json_type_long);
    }
    static inline json *create_double()
    {
        return new json(json_type_double);
    }
    static inline json *create_bool()
    {
        return new json(json_type_bool);
    }

public:
    json();
    json(const std::string &val);
    json(const char *val, int len = -1);
    json(int64_t val);
    json(uint64_t val);
#ifdef _WIN64
    json(long val);
    json(unsigned long val);
#endif // _WIN64
    json(int val);
    json(unsigned int val);
    json(double val);
    json(bool val);
    json(const unsigned char type);
    ~json();

    /* 深度递归复制 */
    json *deep_copy();

    /* 首先重置本json为 null, 然后从文件加载json */
    bool load_from_file(const char *pathname);

    /* 首先重置本json为 null, 然后从jstr反序列化为json */
    bool unserialize(const char *jstr, int jsize = -1);
    inline bool unserialize(const std::string &jstr) { return unserialize(jstr.c_str(), (int)(jstr.size())); }

    /* 序列化 */
    json *serialize(std::string &result, int flags = 0 /* json_serialize_XXX */);
    std::string serialize(int flags = 0 /* json_serialize_XXX */);

    /* 类型 */
    inline int get_type() { return type_; }
    inline bool is_string() { return type_ == json_type_string; }
    inline bool is_long() { return type_ == json_type_long; }
    inline bool is_double() { return type_ == json_type_double; }
    inline bool is_object() { return type_ == json_type_object; }
    inline bool is_array() { return type_ == json_type_array; }
    inline bool is_bool() { return type_ == json_type_bool; }
    inline bool is_null() { return type_ == json_type_null; }

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
    int64_t &get_long_value(int64_t def = 0);

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
    inline json *set_string_value(const std::string &val)
    {
        get_string_value() = val;
        return this;
    }
    inline json *set_long_value(int64_t val)
    {
        get_long_value() = val;
        return this;
    }

    inline json *set_double_value(double val)
    {
        get_double_value() = val;
        return this;
    }
    inline json *set_bool_value(bool val)
    {
        get_bool_value() = val;
        return this;
    }

    inline json *set_value(const char *val, int len = -1) { return set_string_value(val, len); }
    inline json *set_value(const std::string &val)
    {
        get_string_value() = val;
        return this;
    }
    inline json *set_value(int64_t val)
    {
        get_long_value() = val;
        return this;
    }
    inline json *set_value(uint64_t val)
    {
        get_long_value() = val;
        return this;
    }
#ifdef _WIN64
    inline json *set_value(long val)
    {
        get_long_value() = val;
        return this;
    }
    inline json *set_value(unsigned long val)
    {
        get_long_value() = val;
        return this;
    }
#endif // _WIN64
    inline json *set_value(int val)
    {
        get_long_value() = val;
        return this;
    }
    inline json *set_value(unsigned int val)
    {
        get_long_value() = val;
        return this;
    }
    inline json *set_value(double val)
    {
        get_double_value() = val;
        return this;
    }
    inline json *set_value(bool val)
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
    inline json *array_unshift(json *j, bool return_child = false)
    {
        return array_insert(0, j, return_child);
    }

    /* 在尾部追加节点 j */
    inline json *array_add(json *j, bool return_child = false)
    {
        return array_insert(-1, j, return_child);
    }
    inline json *array_add(const char *val, int len) { return array_add(new json(val, len)); }
    inline json *array_add(const unsigned char *val, int len) { return array_add(new json((const char *)val, len)); }
    inline json *array_push(const char *val, int len) { return array_add(new json(val, len)); }
    inline json *array_push(const unsigned char *val, int len) { return array_add(new json((const char *)val, len)); }
    inline json *array_push(json *j, bool return_child = false) { return array_add(j, return_child); }

    /* 设置下表为 idx 的成员 j, 如果键idx存在则, 则把idx对应的json赋给 *old, 如果old为0, 则销毁 */
    json *array_update(int idx, json *j, json **old, bool return_child = false);
    inline json *array_update(int idx, json *j, bool return_child = false)
    {
        return array_update(idx, j, 0, return_child);
    }
    inline json *array_update(uint64_t idx, const char *val, int len, bool return_child = false)
    {
        return array_update(idx, new json(val, len), return_child);
    }
    inline json *array_update(uint64_t idx, const char *val, int len, json **old, bool return_child = false)
    {
        return array_update(idx, new json(val, len), old, return_child);
    }

    /* 删除下标为 idx的节点, 存在则返回true, 赋值给 *old, 如果 old为 0, 则销毁 */
    bool array_delete(int idx, json **old = 0);

    /* 弹出第一个节点, 存在则返回true, 赋值给 *old, 如果 old为 0, 则销毁 */
    inline bool array_shift(json **old = 0) { return array_delete(0, old); }

    /* 弹出最后一个节点, 存在则返回true, 赋值给 *old, 如果 old为 0, 则销毁 */
    inline bool array_pop(json **old = 0) { return array_delete(-1, old); }

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

    inline json *object_update(const std::string &key, json *j, bool return_child = false)
    {
        return object_update(key, j, 0, return_child);
    }
    inline json *object_add(const std::string &key, json *j, bool return_child = false)
    {
        return object_update(key, j, 0, return_child);
    }
    inline json *object_update(const std::string &key, const char *val, int len, json **old, bool return_child = false)
    {
        return object_update(key, new json(val, len), old, return_child);
    }
    inline json *object_add(const std::string &key, const char *val, int len, json **old, bool return_child = false)
    {
        return object_update(key, new json(val, len), old, return_child);
    }
    inline json *object_update(const std::string &key, const char *val, int len, bool return_child = false)
    {
        return object_update(key, new json(val, len), return_child);
    }
    inline json *object_add(const std::string &key, const char *val, int len, bool return_child = false)
    {
        return object_update(key, new json(val, len), return_child);
    }

    inline json *object_update(const std::string &key, const unsigned char *val, int len, json **old, bool return_child = false)
    {
        return object_update(key, new json((const char *)val, len), old, return_child);
    }
    inline json *object_add(const std::string &key, const unsigned char *val, int len, json **old, bool return_child = false)
    {
        return object_update(key, new json((const char *)val, len), old, return_child);
    }
    inline json *object_update(const std::string &key, const unsigned char *val, int len, bool return_child = false)
    {
        return object_update(key, new json((const char *)val, len), return_child);
    }
    inline json *object_add(const std::string &key, const unsigned char *val, int len, bool return_child = false)
    {
        return object_update(key, new json((const char *)val, len), return_child);
    }

    /* 删除键为key的节点, 存在则返回true, 赋值给 *old, 如果 old为 0, 则销毁 */
    bool object_delete(const char *key, json **old = nullptr);
    inline bool object_delete(const unsigned char *key, json **old = nullptr)
    {
        return object_delete((const char *)key, old);
    }
    bool object_delete(const std::string &key, json **old = nullptr);
    inline json *object_detach(const std::string &key)
    {
        json *j = nullptr;
        object_delete(key, &j);
        return j;
    }

#define ___zcc_json_update(TTT)                                                                        \
    json *array_insert(int idx, TTT val, bool return_child = false)                                    \
    {                                                                                                  \
        return array_insert(idx, new json(val), return_child);                                         \
    }                                                                                                  \
    inline json *array_unshift(TTT val, bool return_child = false)                                     \
    {                                                                                                  \
        return array_insert(0, new json(val), return_child);                                           \
    }                                                                                                  \
    inline json *array_add(TTT val, bool return_child = false)                                         \
    {                                                                                                  \
        return array_insert(-1, new json(val), return_child);                                          \
    }                                                                                                  \
    inline json *array_push(TTT val, bool return_child = false)                                        \
    {                                                                                                  \
        return array_insert(-1, new json(val), return_child);                                          \
    }                                                                                                  \
    inline json *array_update(int idx, TTT val, bool return_child = false)                             \
    {                                                                                                  \
        return array_update(idx, new json(val), return_child);                                         \
    }                                                                                                  \
    inline json *array_update(int idx, TTT val, json **old, bool return_child = false)                 \
    {                                                                                                  \
        return array_update(idx, new json(val), old, return_child);                                    \
    }                                                                                                  \
    inline json *object_update(const std::string &key, TTT val, bool return_child = false)             \
    {                                                                                                  \
        return object_update(key, new json(val), return_child);                                        \
    }                                                                                                  \
    inline json *object_update(const std::string &key, TTT val, json **old, bool return_child = false) \
    {                                                                                                  \
        return object_update(key, new json(val), old, return_child);                                   \
    }                                                                                                  \
    inline json *object_add(const std::string &key, TTT val, bool return_child = false)                \
    {                                                                                                  \
        return object_update(key, new json(val), 0, return_child);                                     \
    }                                                                                                  \
    inline json *object_add(const std::string &key, TTT val, json **old, bool return_child = false)    \
    {                                                                                                  \
        return object_update(key, new json(val), old, return_child);                                   \
    }                                                                                                  \
    inline void operator=(TTT val) { set_value(val); }

    /* 这几组方法类似 array_update, object_update, 只不过参数不同 */
    ___zcc_json_update(const std::string &);
    ___zcc_json_update(const char *);
    ___zcc_json_update(const unsigned char *);
    ___zcc_json_update(int64_t);
    ___zcc_json_update(uint64_t);
#ifdef _WIN64
    ___zcc_json_update(long);
    ___zcc_json_update(unsigned long);
#endif // _WIN64
    ___zcc_json_update(int);
    ___zcc_json_update(unsigned int);
    ___zcc_json_update(double);
    ___zcc_json_update(bool);
#undef ___zcc_json_update

    /* */
    inline json &operator[](const std::string &key) { return object_get_or_create(key); }
    inline json &operator[](int idx) { return array_get_or_create(idx); }

    /* @get_by_path 得到路径path对应的j son, 并返回, 如:
     * 已知 json {group:{linux:[{}, {}, {me: {age:18, sex:"male"}}}}, 则
     * get_by_path("group/linux/2/me") 返回的 应该是 {age:18, sex:"male"} */
    json *get_by_path(const char *path);

    /* 如: get_by_path_vec("group", "linux", "2", "me", 0); */
    json *get_by_path_vec(const char *path0, ...);

    /* 获取 路径 pathname 对应的节点 的 值 */
    bool get_value_by_path(const char *pathname, std::string &value);
    bool get_value_by_path(const char *pathname, int64_t *value);

    /* */
    inline json *get_parent() { return parent_; }
    json *get_top();
    json *set_parent(json *js);
    json *debug_show();

    /* 扩展 */
    /* 如果不是 object 类型, 则先转为 object 类型 */
    std::string object_get_string_value(const std::string &key, const char *def = "");
    int64_t object_get_long_value(const std::string &key, int64_t def = 0);
    bool object_get_bool_value(const std::string &key, bool def = false);

private:
    inline void operator=(json *val) {}
    inline void operator=(json val) {}

private:
    unsigned char type_;
    union
    {
        bool b;
        int64_t l;
        double d;
        char s[sizeof(std::string)];
        std::vector<json *> *v;
        std::map<std::string, json *> *m;
    } val_;
    json *parent_;
};

zcc_namespace_end;
#pragma pack(pop)
#endif // __cplusplus

#endif // ZCC_LIB_INCLUDE_JSON___

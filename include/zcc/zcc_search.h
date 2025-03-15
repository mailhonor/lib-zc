/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2016-01-15
 * ================================
 */

#pragma once

#ifndef ZCC_LIB_INCLUDE_SEARCH___
#define ZCC_LIB_INCLUDE_SEARCH___

#include "./zcc_stdlib.h"

#ifdef __cplusplus
#pragma pack(push, 4)
zcc_namespace_begin;

// 多关键字匹配

extern bool var_msearch_debug_mode;

struct zcc_msearch_reader_t;
struct zcc_msearch_builder_t;
struct zcc_msearch_walker_t;

// 构造器
class msearch_builder
{
public:
    msearch_builder();
    virtual ~msearch_builder();
    // 增加条目
    void add_token(const void *word, int len = -1);
    inline void add_token(const std::string &word)
    {
        add_token(word.c_str(), word.size());
    }
    // 从文件加载条目
    // 忽略空行, 忽略 ### 开头的行
    // trim掉两头空字符
    int add_token_from_file(const char *pathname);
    inline int add_token_from_file(const std::string &pathname)
    {
        return add_token_from_file(pathname.c_str());
    }
    // 增加条目完毕, 既开始编译
    virtual void add_over();

    // 编译后的数据地址
    const void *get_compiled_data();
    // 编译后的数据长度
    int64_t get_compiled_size();

protected:
    zcc_msearch_builder_t *builder_engine_{nullptr};
};

// 搜索
class msearch_reader : public msearch_builder
{
    friend msearch_walker;

public:
    // 是不是本数据格式
    static bool is_my_data(const void *data, int64_t size);
    static bool is_my_file(const char *filename);
    inline static bool is_my_file(const std::string &filename)
    {
        return is_my_file(filename.c_str());
    }
    // 从数据加载, 不会复制 data, 请保证 data 一直 可读
    static msearch_reader *create_from_data(const void *data);
    // 从文件加载, 不会复制文件和数据, 请保证不修改这个文件
    static msearch_reader *create_from_file(const char *filename);
    inline static msearch_reader *create_from_file(const std::string &filename)
    {
        return create_from_file(filename.c_str());
    }

public:
    msearch_reader();
    ~msearch_reader();
    // 从数据加载, 不会复制 data, 请保证 data 一直 可读
    void load_from_data(const void *data);
    // 从文件加载, 不会复制文件和数据, 请保证不修改这个文件
    int load_from_file(const char *filename);
    inline int load_from_file(const std::string &filename)
    {
        return load_from_file(filename.c_str());
    }
    // 匹配 str, 返回 -1: 出错了, 0: 没找到, 1: 找到了
    // matched_ptr: 找到的关键字地址
    // matched_len: 知道的关键字长度
    int match(const char *str, int len, const char **matched_ptr, int *matched_len);
    // 注意: 本class 基于 msearch_builder, 可以向 msearch_builder 一样手动添加条目
    //  所以, 在此逻辑下, add_over 后, 就可以使用 match 了
    void add_over();

protected:
    zcc_msearch_reader_t *reader_engine_{nullptr};
    mmap_reader reader_;
    int reader_type_{0}; // 1: data, 2: mmap
};

// 遍历器
class msearch_walker
{
public:
    msearch_walker(msearch_reader &reader);
    ~msearch_walker();
    // -1:出错, 0: 没有了, 1: 找到了
    // 非线程安全
    int walk(const char **token, int *tlen);
    // 重置
    void reset();

protected:
    msearch_reader &reader_;
    zcc_msearch_walker_t *walker_engine_{nullptr};
};

zcc_namespace_end;
#pragma pack(pop)
#endif // __cplusplus

#endif // ZCC_LIB_INCLUDE_SEARCH___

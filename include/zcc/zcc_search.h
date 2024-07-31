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

extern bool var_msearch_debug_mode;

struct zcc_msearch_reader_t;
struct zcc_msearch_builder_t;
struct zcc_msearch_walker_t;

class msearch_builder
{
public:
    msearch_builder();
    virtual ~msearch_builder();
    void add_token(const void *word, int len = -1);
    inline void add_token(const std::string &word)
    {
        add_token(word.c_str(), word.size());
    }
    int add_token_from_file(const char *pathname);
    inline int add_token_from_file(const std::string &pathname)
    {
        return add_token_from_file(pathname.c_str());
    }
    virtual void add_over();
    const void *get_compiled_data();
    int64_t get_compiled_size();

protected:
    zcc_msearch_builder_t *builder_engine_{nullptr};
};

class msearch_reader : public msearch_builder
{
    friend msearch_walker;

public:
    static bool is_my_data(const void *data, int64_t size);
    static bool is_my_file(const char *filename);
    inline static bool is_my_file(const std::string &filename)
    {
        return is_my_file(filename.c_str());
    }
    static msearch_reader *create_from_data(const void *data);
    static msearch_reader *create_from_file(const char *filename);
    inline static msearch_reader *create_from_file(const std::string &filename)
    {
        return create_from_file(filename.c_str());
    }

public:
    msearch_reader();
    ~msearch_reader();
    void load_from_data(const void *data);
    int load_from_file(const char *filename);
    inline int load_from_file(const std::string &filename)
    {
        return load_from_file(filename.c_str());
    }
    int match(const char *str, int len, const char **matched_ptr, int *matched_len);
    void add_over();

protected:
    zcc_msearch_reader_t *reader_engine_{nullptr};
    mmap_reader reader_;
    int reader_type_{0}; // 1: data, 2: mmap
};

class msearch_walker
{
public:
    msearch_walker(msearch_reader &reader);
    ~msearch_walker();
    // -1:出错, 0: 没找到, 1: 找到
    // 非线程安全
    int walk(const char **token, int *tlen);
    void reset();

protected:
    msearch_reader &reader_;
    zcc_msearch_walker_t *walker_engine_{nullptr};
};

zcc_namespace_end;
#pragma pack(pop)
#endif // __cplusplus

#endif // ZCC_LIB_INCLUDE_SEARCH___

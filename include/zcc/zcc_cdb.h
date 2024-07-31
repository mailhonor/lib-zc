/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2016-01-15
 * ================================
 */

#pragma once

#ifndef ZCC_LIB_INCLUDE_CDB___
#define ZCC_LIB_INCLUDE_CDB___

#include "./zcc_stdlib.h"

#ifdef __cplusplus
#pragma pack(push, 4)
zcc_namespace_begin;

extern const char *cdb_code_version;

class cdb_reader
{
public:
    static bool is_my_data(const void *data, int64_t size);
    static bool is_my_file(const char *filename);
    inline static bool is_my_file(const std::string &filename)
    {
        return is_my_file(filename.c_str());
    }
    static cdb_reader *create_from_file(const char *pathname);
    static cdb_reader *create_from_file(const std::string &pathname)
    {
        return create_from_file(pathname.c_str());
    }
    static cdb_reader *create_from_data(const void *data);

public:
    friend class cdb_walker;
    cdb_reader();
    ~cdb_reader();
    bool open_file(const char *pathname);
    inline bool open_file(const std::string &pathname)
    {
        return open_file(pathname.c_str());
    }
    bool open_data(const void *data);
    void close();
    int get_count();
    /* -1:出错, 0: 没找到, 1: 找到, 线程安全 */
    int find(const void *key, int klen, void **val, int *vlen);

protected:
    bool open_data_do(const void *data);
    mmap_reader file_reader_;
    const char *data_{0};
    int max_key_length_{0};
    int val_length_{0};
    int count_{0};
    bool used_flag_{false};
    bool file_flag_{false};
};

class cdb_walker
{
public:
    cdb_walker(cdb_reader &reader);
    cdb_walker();
    ~cdb_walker();
    void set_reader(cdb_reader &reader);
    void close();
    /* -1:出错, 0: 没找到, 1: 找到, 非线程安全 */
    int walk(void **key, int *klen, void **val, int *vlen);
    void reset();

protected:
    cdb_reader *reader_{0};
    int klen_{-1};
    char *hash_vector_data_{0};
    int hash_vector_size_{0};
    int hash_vector_i_{0};
    char *ndata_{0}; /* n means node */
    int ncount_{0};
    int ni_{0};
};

struct cdb_builder_engine_t;
class cdb_builder
{
public:
    cdb_builder();
    ~cdb_builder();
    /* 更新值 */
    void update(const void *key, int klen, const void *val, int vlen);
    inline void update(const std::string &key, const std::string &val)
    {
        update(key.c_str(), key.size(), val.c_str(), val.size());
    }
    bool compile();
    const void *get_compiled_data();
    int get_compiled_data_len();
    int dump(const char *dest_pathname);

public:
    cdb_builder_engine_t *engine_{0};
};

zcc_namespace_end;
#pragma pack(pop)
#endif // __cplusplus

#endif // ZCC_LIB_INCLUDE_CDB___

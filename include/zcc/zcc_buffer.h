/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2016-01-15
 * ================================
 */

#pragma once

#ifndef ZCC_LIB_INCLUDE_BUFFER___
#define ZCC_LIB_INCLUDE_BUFFER___

#include "./zcc_stdlib.h"

#ifdef __cplusplus
#pragma pack(push, 4)
zcc_namespace_begin;

// 不推荐使用
class buffer
{
    static int static_magic;

public:
    buffer();
    buffer(int64_t capability);
    buffer(void *data, int64_t size, int static_magic = 0);
    ~buffer();
    inline char *get_data() { return data_; };
    inline char *c_str() { return data_; };
    inline int64_t get_len() { return len_; }
    inline int64_t size() { return len_; }
    // 调整容量,使剩余容量大于 need_extra_space, 返回实际空余容量
    int64_t need_space(int64_t need_extra_space);
    buffer &reset()
    {
        len_ = 0;
        return *this;
    }
    buffer &clear()
    {
        len_ = 0;
        return *this;
    }
    inline buffer &putc(int ch)
    {
        ((len_ < capability_) ? ((int)(((unsigned char *)(data_))[len_++] = (unsigned char)(ch))) : ((static_mode_ ? 0 : putc_do(ch))));
        return *this;
    }
    inline buffer &put(int ch)
    {
        return putc(ch);
    }
    inline buffer &push_back(int ch)
    {
        return putc(ch);
    }
    inline buffer &terminate()
    {
        data_[len_] = 0;
        return *this;
    }
    inline buffer &truncate(int64_t new_len)
    {
        if ((len_ > new_len) && (new_len > -1))
        {
            len_ = new_len;
            data_[len_] = 0;
        }
        return *this;
    }
    buffer &resize(int64_t new_size, int default_value = -1);
    buffer &strcpy(const char *src, int64_t len = -1);
    buffer &strcat(const char *src, int64_t len = -1);
    buffer &memcpy(const void *src, int64_t size);
    buffer &memcat(const void *src, int64_t size);
    inline buffer &append(buffer &b)
    {
        return memcat(b.get_data(), b.get_len());
    }
    inline buffer &append(const std::string &s)
    {
        return memcat(s.c_str(), s.size());
    }
    inline buffer &append(const char *s, int len = -1)
    {
        return memcat(s, len);
    }
    inline buffer &append(int ch)
    {
        return putc(ch);
    }
    inline buffer &puts(buffer &b)
    {
        return append(b);
    }
    inline buffer &puts(const std::string &s)
    {
        return append(s);
    }
    inline buffer &puts(const char *s)
    {
        return strcat(s);
    }
    inline buffer &tolower()
    {
        zcc::tolower(data_);
        return *this;
    }
    inline buffer &toupper()
    {
        zcc::toupper(data_);
        return *this;
    }
#ifdef _WIN64
    buffer &printf_1024(const char *format, ...);
#else  // _WIN64
    buffer &__attribute__((format(gnu_printf, 2, 3))) printf_1024(const char *format, ...);
#endif // _WIN64
    buffer &vprintf_1024(const char *format, va_list ap);
    buffer &trim_right_rn();
    buffer &clear_null();

protected:
    int putc_do(int ch);

public:
    char *data_{0};
    int64_t len_{0};
    int64_t capability_{0};
    bool static_mode_{false};
};

zcc_namespace_end;
#pragma pack(pop)
#endif // __cplusplus

#endif // ZCC_LIB_INCLUDE_BUFFER___

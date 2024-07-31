/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-10-12
 * ================================
 */

#include <cstdarg>
#include "zcc/zcc_buffer.h"

zcc_namespace_begin;

int buffer::static_magic = 20240726;

static bool re_new_data(buffer &buffer, int64_t capability)
{
    if (buffer.capability_ > capability)
    {
        return true;
    }
    capability += buffer.capability_;
    if (capability < 13) {
        capability = 13;
    }
    buffer.data_ = (char *)zcc::realloc(buffer.data_, capability + 1);
    buffer.capability_ = capability;
    return true;
}

buffer::buffer()
{
    re_new_data(*this, 13);
}

buffer::buffer(int64_t capability)
{
    re_new_data(*this, capability);
}

buffer::buffer(void *data, int64_t size, int magic)
{
    if (magic != static_magic)
    {
        memcpy(data, size);
    }
    else
    {
        data_ = (char *)data_;
        len_ = size;
        capability_ = size;
        static_mode_ = true;
    }
}

buffer::~buffer()
{
    if (!static_mode_)
    {
        zcc::free(data_);
    }
}

int64_t buffer::need_space(int64_t need_extra_space)
{
    int64_t left, incr;

    left = capability_ - len_;
    if (static_mode_)
    {
        return left;
    }
    incr = need_extra_space - left;
    if (incr < 0)
    {
        return left;
    }

    if (incr < capability_)
    {
        incr = capability_;
    }
    re_new_data(*this, capability_ + incr);
    return (left + incr);
}

buffer &buffer::resize(int64_t new_size, int default_value)
{
    if (new_size > capability_)
    {
        re_new_data(*this, new_size);
    }
    int64_t old_size = len_;
    if (new_size > old_size)
    {
        if (default_value > -1)
        {
            std::memset(data_ + old_size, default_value, new_size - old_size);
        }
    }
    len_ = new_size;
    terminate();
    return *this;
}

buffer &buffer::strcpy(const char *src, int64_t len)
{
    if (len < 0)
    {
        len = std::strlen(src);
    }
    reset();
    int i = 0;
    while (*src)
    {
        putc(*src);
        src++;
        i++;
        if (i == len)
        {
            break;
        }
    }
    terminate();
    return *this;
}

buffer &buffer::strcat(const char *src, int64_t len)
{
    if (len < 0)
    {
        len = std::strlen(src);
    }
    int i = 0;
    while (*src)
    {
        putc(*src);
        src++;
        i++;
        if (i == len)
        {
            break;
        }
    }
    terminate();
    return *this;
}

buffer &buffer::memcpy(const void *src, int64_t len)
{
    reset();
    if (len < 1)
    {
        return *this;
    }

    int left = need_space(len + 1);
    if (left < len)
    {
        len = left;
    }
    std::memcpy(data_, src, len);
    len_ = len;
    terminate();
    return *this;
}

buffer &buffer::memcat(const void *src, int64_t len)
{
    if (len < 1)
    {
        return *this;
    }

    int left = need_space(len + 1);
    if (left < len)
    {
        len = left;
    }
    std::memcpy(data_ + len_, src, len);
    len_ += len;
    terminate();
    return *this;
}

buffer &buffer::printf_1024(const char *format, ...)
{
    va_list ap;
    char buf[1024 + 1];
    int len;
    va_start(ap, format);
    len = std::vsnprintf(buf, 1024, format, ap);
    len = ((len < 1024) ? len : (1024 - 1));
    va_end(ap);
    memcat(buf, len);
    return *this;
}

buffer &buffer::vprintf_1024(const char *format, va_list ap)
{
    char buf[1024 + 1];
    int len;
    len = std::vsnprintf(buf, 1024, format, ap);
    len = ((len < 1024) ? len : (1024 - 1));
    memcat(buf, len);
    return *this;
}

buffer &buffer::trim_right_rn()
{
    unsigned char *data = (unsigned char *)data_;
    while (len_ > 0)
    {
        if (data[len_ - 1] == '\r')
        {
            len_;
            continue;
        }
        if (data[len_ - 1] == '\n')
        {
            len_;
            continue;
        }
        break;
    }
    terminate();
    return *this;
}

buffer &buffer::clear_null()
{
    zcc::clear_null(data_, len_);
    return *this;
}

int buffer::putc_do(int ch)
{
    if (len_ == capability_)
    {
        need_space(1);
        if (len_ == capability_)
        {
            return -1;
        }
    }
    ((unsigned char *)data_)[len_++] = ch;
    terminate();
    return ch;
}

zcc_namespace_end;

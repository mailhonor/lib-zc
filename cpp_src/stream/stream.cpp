/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2021-04-15
 * ================================
 */

#include <cstdarg>
#include "zcc/zcc_stream.h"

zcc_namespace_begin;

stream::stream()
{
    engine_ = (stream_engine *)zcc::malloc(sizeof(stream_engine) + 1);
    std::memset(engine_, 0, 32);
    engine_->wait_timeout = -1;
}

stream::~stream()
{
    zcc::free(engine_);
    engine_ = 0;
}

stream &stream::reset()
{
    std::memset(engine_, 0, 32);
    return *this;
}

stream &stream::set_timeout(int wait_timeout)
{
    engine_->wait_timeout = wait_timeout;
    return *this;
}

int stream::getc_do()
{
    if (engine_->read_buf_p1 < engine_->read_buf_p2)
    {
        return engine_->read_buf[engine_->read_buf_p1++];
    }

    if (engine_->error || engine_->eof)
    {
        return -1;
    }

    if (engine_->write_buf_len > 0)
    {
        if (flush() == -1)
        {
            return -1;
        }
    }

    engine_->read_buf_p1 = engine_->read_buf_p2 = 0;

    int ret = engine_read(engine_->read_buf, rbuf_size);

    if (ret == 0)
    {
        engine_->eof = 1;
        return -1;
    }
    if (ret < 0)
    {
        engine_->error = 1;
        return -1;
    }
    engine_->read_buf_p1 = 1;
    engine_->read_buf_p2 = ret;

    return engine_->read_buf[0];
}

stream &stream::ungetc()
{
    if (engine_->read_buf_p1 > 0)
    {
        engine_->read_buf_p1--;
    }
    else
    {
        zcc_fatal("stream ungetc too much");
    }
    return *this;
}

int stream::read(void *mem, int max_len)
{
    if (max_len < 1)
    {
        return 0;
    }

    char *ps = (char *)mem;
    int left_len = max_len, ret_len = 0;
    int ch;
    int have_len = engine_->read_buf_p2 - engine_->read_buf_p1;
    if (have_len == 0)
    {
        ch = getc_do();
        if (ch == -1)
        {
            if (engine_->eof)
            {
                return 0;
            }
            return -1;
        }
        *ps++ = ch;
        have_len = engine_->read_buf_p2 - engine_->read_buf_p1;
        left_len--;
        ret_len++;
    }

    if (left_len > have_len)
    {
        left_len = have_len;
    }
    ret_len += left_len;
    std::memcpy(ps, engine_->read_buf + engine_->read_buf_p1, left_len);
    engine_->read_buf_p1 += left_len;
    return ret_len;
}

int stream::read(std::string &str, int max_len)
{
    if (max_len < 1)
    {
        return 0;
    }

    str.clear();
    int left_len = max_len, ret_len = 0;
    int ch;
    int have_len = engine_->read_buf_p2 - engine_->read_buf_p1;
    if (have_len == 0)
    {
        ch = getc_do();
        if (ch == -1)
        {
            if (engine_->eof)
            {
                return 0;
            }
            return -1;
        }
        str.push_back(ch);
        have_len = engine_->read_buf_p2 - engine_->read_buf_p1;
        left_len--;
        ret_len++;
    }

    if (left_len > have_len)
    {
        left_len = have_len;
    }
    ret_len += left_len;
    str.append((char *)(engine_->read_buf) + engine_->read_buf_p1, left_len);
    engine_->read_buf_p1 += left_len;
    return ret_len;
}

int stream::readn(void *mem, int strict_len)
{
    if (strict_len < 1)
    {
        return 0;
    }

    char *ps = (char *)mem;
    int left_len = strict_len;
    int ch;

    if (mem)
    {
        while (left_len > 0)
        {
            ch = getc();
            if (ch == -1)
            {
                break;
            }
            *ps++ = ch;
            left_len--;
        }
    }
    else
    {
        while (left_len > 0)
        {
            ch = getc();
            if (ch == -1)
            {
                break;
            }
            left_len--;
        }
    }

    if (strict_len > left_len)
    {
        return strict_len - left_len;
    }
    if (engine_->eof)
    {
        return 0;
    }
    return -1;
}

int stream::readn(std::string &str, int strict_len)
{
    if (strict_len < 1)
    {
        return 0;
    }

    str.clear();
    int left_len = strict_len;
    int ch;

    while (left_len > 0)
    {
        ch = getc();
        if (ch == -1)
        {
            break;
        }
        str.push_back(ch);
        left_len--;
    }

    if (strict_len > left_len)
    {
        return strict_len - left_len;
    }
    if (engine_->eof)
    {
        return 0;
    }
    return -1;
}

int stream::read_delimiter(void *mem, int delimiter, int max_len)
{
    if (max_len < 1)
    {
        return 0;
    }

    char *ps = (char *)mem;
    int left_len = max_len;
    int ch;

    if (ps)
    {
        while (left_len > 0)
        {
            ch = getc();
            if (ch == -1)
            {
                break;
            }
            *ps++ = ch;
            left_len--;
            if (ch == delimiter)
            {
                break;
            }
        }
    }
    else
    {
        while (left_len > 0)
        {
            ch = getc();
            if (ch == -1)
            {
                break;
            }
            left_len--;
            if (ch == delimiter)
            {
                break;
            }
        }
    }

    *ps = 0;
    if (max_len > left_len)
    {
        return max_len - left_len;
    }
    if (engine_->eof)
    {
        return 0;
    }
    return -1;
}

int stream::read_delimiter(std::string &str, int delimiter, int max_len)
{
    if (max_len < 1)
    {
        return 0;
    }

    str.clear();
    int left_len = max_len;
    int ch;

    while (left_len > 0)
    {
        ch = getc();
        if (ch == -1)
        {
            break;
        }
        str.push_back(ch);
        left_len--;
        if (ch == delimiter)
        {
            break;
        }
    }

    if (max_len > left_len)
    {
        return max_len - left_len;
    }
    if (engine_->eof)
    {
        return 0;
    }
    return -1;
}

int stream::write(const void *buf, int len)
{
    if (len < 0)
    {
        len = std::strlen((const char *)buf);
    }
    if (len < 1)
    {
        return 0;
    }
    int left_len = len;
    int ch;
    const char *str = (const char *)buf;
    if (str == 0)
    {
        str = "";
    }
    while (left_len--)
    {
        ch = *str++;
        putc(ch);
        if (engine_->error || engine_->eof)
        {
            return -1;
        }
    }
    return len;
}

int stream::printf_1024(const char *format, ...)
{
    va_list ap;
    char buf[1024 + 1];
    int len;

    if (format == 0)
    {
        format = "";
    }
    va_start(ap, format);
    len = std::vsnprintf(buf, 1024, format, ap);
    len = ((len < 1024) ? len : (1024 - 1));
    va_end(ap);

    return write(buf, len);
}

int stream::flush()
{
    if (engine_->error)
    {
        return -1;
    }
    int ret, left = engine_->write_buf_len;
    char *data = (char *)(engine_->write_buf);

    while (left > 0)
    {
        if ((ret = engine_write(data, left)) < 0)
        {
            break;
        }
        left -= ret;
        data += ret;
    }

    engine_->write_buf_len = 0;
    if (left > 0)
    {
        engine_->error = 1;
        return -1;
    }
    return 1;
}

int stream::putc_do(int ch)
{
    int ret;

    if (engine_->error)
    {
        return -1;
    }

    if (engine_->write_buf_len >= rbuf_size)
    {
        ret = flush();
        if (ret == -1)
        {
            return -1;
        }
    }
    engine_->write_buf[engine_->write_buf_len++] = ch;
    return ch;
}

int stream::timed_read_wait(int wait_timeout)
{
    return 1;
}

int stream::timed_write_wait(int wait_timeout)
{
    return 1;
}

int stream::get_cint()
{
    int ch, size = 0, shift = 0;
    while (1)
    {
        ch = getc();
        if (ch == -1)
        {
            return -1;
        }
        size |= ((ch & 0177) << shift);
        if (ch & 0200)
        {
            break;
        }
        shift += 7;
    }
    return size;
}

/* write */
int stream::write_cint(int size)
{
    int ch, left = size, len = 0;
    do
    {
        ch = left & 0177;
        left >>= 7;
        if (!left)
        {
            ch |= 0200;
        }
        putc(ch);
    } while (left);
    return len;
}

int stream::write_cint_and_data(const void *buf, int len)
{
    if (len < 0)
    {
        len = std::strlen((char *)(void *)buf);
    }
    write_cint(len);
    write(buf, len);
    return len;
}

int stream::write_cint_and_int(int i)
{
    char buf[32];
    int len;
    len = std::sprintf(buf, "%d", i);
    write_cint(len);
    write(buf, len);
    return len;
}

int stream::write_cint_and_long(int64_t i)
{
    char buf[64];
    int len;
    len = std::sprintf(buf, "%zd", i);
    write_cint_and_data(buf, len);
    return len;
}

int stream::write_cint_and_pp(const char **pp, int size)
{
    for (int i = 0; i < size; i++)
    {
        write_cint_and_data(pp[i], -1);
    }
    return 1;
}

zcc_namespace_end;

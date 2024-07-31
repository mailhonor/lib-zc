/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2016-02-23
 * ================================
 */

#include "zcc/zcc_stdlib.h"
#include "zcc/zcc_buffer.h"

zcc_namespace_begin;

int cint_data_unescape(const void *src_data, int src_size, void **result_data, int *result_len)
{
    int i = 0;
    unsigned char *buf = (unsigned char *)src_data;
    int ch, len = 0, shift = 0;
    while (1)
    {
        ch = ((i++ == src_size) ? -1 : *buf++);
        if (ch == -1)
        {
            return -1;
        }
        len |= ((ch & 0177) << shift);
        if (ch & 0200)
        {
            break;
        }
        shift += 7;
    }
    if (i + len > src_size)
    {
        return -1;
    }
    *result_data = buf;
    *result_len = len;
    return i + len;
}

int cint_data_unescape_all(const void *src_data, int src_size, size_data *vec, int vec_size)
{
    int count = 0;
    size_data *sd;
    unsigned char *buf = (unsigned char *)src_data;
    while (1)
    {
        if (count >= vec_size)
        {
            return count;
        }
        int i = 0;
        int ch, len = 0, shift = 0;
        while (1)
        {
            ch = ((i++ == src_size) ? -1 : *buf++);
            if (ch == -1)
            {
                return count;
            }
            len |= ((ch & 0177) << shift);
            if (ch & 0200)
            {
                break;
            }
            shift += 7;
        }
        if (i + len > src_size)
        {
            return count;
        }
        sd = vec + count++;
        sd->data = (char *)buf;
        sd->size = len;
        buf += len;
    }
    return count;
}

void cint_data_escape(std::string &bf, const void *data, int len)
{
    int ch, left = len;

    if (len < 0)
    {
        len = std::strlen((const char *)data);
    }
    do
    {
        ch = left & 0177;
        left >>= 7;
        if (!left)
        {
            ch |= 0200;
        }
        bf.push_back(ch);
    } while (left);
    if (len > 0)
    {
        bf.append((const char *)data, len);
    }
}

void cint_data_escape_int(std::string &bf, int i)
{
    char buf[32];
    int len;
    len = std::sprintf(buf, "%d", i);
    cint_data_escape(bf, buf, len);
}

void cint_data_escape_long(std::string &bf, int64_t i)
{
    char buf[64];
    int len;
    len = std::sprintf(buf, "%zd", i);
    cint_data_escape(bf, buf, len);
}

void cint_data_escape_pp(std::string &bf, const char **pp, int size)
{
    for (int i = 0; i < size; i++)
    {
        cint_data_escape(bf, pp[i], -1);
    }
}

int cint_put(int size, char *buf)
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
        ((unsigned char *)buf)[len++] = ch;
    } while (left);
    return len;
}

zcc_namespace_end;

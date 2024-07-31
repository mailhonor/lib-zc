/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2021-04-15
 * ================================
 */

#include "zcc/zcc_stream.h"
#ifndef _WIN64
#include <unistd.h>
#endif // _WIN64

zcc_namespace_begin;

fstream::fstream()
{
    close(true);
}

#ifdef _WIN64
fstream::fstream(HANDLE fd)
{
    open(fd);
}

fstream &fstream::open(HANDLE fd)
{
    if (fd_ != fd)
    {
        close(true);
        fd_ = fd;
    }
    return *this;
}
#else  // _WIN64
fstream::fstream(int fd)
{
    open(fd);
}

fstream &fstream::open(int fd)
{
    if (fd_ != fd)
    {
        close(true);
        fd_ = fd;
    }
    return *this;
}
#endif // _WIN64

fstream::~fstream()
{
    close(true);
}

int fstream::close(bool close_fd_or_release_ssl)
{
    if (!close_fd_or_release_ssl)
    {
        return 1;
    }

    if (close_fd_or_release_ssl)
    {
#ifdef _WIN64
        bool ret = true;
        if (fd_ != INVALID_HANDLE_VALUE)
        {
            ret = CloseHandle(fd_);
            fd_ = INVALID_HANDLE_VALUE;
        }
        return (ret ? 1 : -1);
#else  // _WIN64
        int ret = 1;
        if (fd_ > -1)
        {
            ret = ::close(fd_);
        }
        fd_ = -1;
        return ret;
#endif // _WIN64
    }
    return 1;
}

int fstream::engine_read(void *buf, int len)
{
#ifdef _WIN64
    DWORD rlen;
    if (ReadFile(fd_, buf, len, &rlen, 0))
    {
        return rlen;
    }
    else
    {
        return -1;
    }
#else  // _WIN64
    return ::read(fd_, buf, len);
#endif // _WIN64
}

int fstream::engine_write(const void *buf, int len)
{
#ifdef _WIN64
    DWORD rlen;
    if (WriteFile(fd_, buf, len, &rlen, 0))
    {
        return rlen;
    }
    else
    {
        return -1;
    }
#else  // _WIN64
    return ::write(fd_, buf, len);
#endif // _WIN64
}

zcc_namespace_end;

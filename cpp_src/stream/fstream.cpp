/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2021-04-15
 * ================================
 */

#include "zcc/zcc_stream.h"
#ifdef _WIN64
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#else // _WIN64
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#endif // _WIN64

zcc_namespace_begin;

fstream::fstream()
{
    close(true);
}

fstream::fstream(int fd)
{
    open(fd);
}

bool fstream::open(const char *fn)
{
    close(true);
    fd_ = zcc::open(fn, O_RDWR | O_CREAT, 0644);
    if (fd_ < 0)
    {
        return false;
    }
    open(fd_);
    return true;
}

fstream &fstream::open(int fd)
{
    if (fd_ != fd)
    {
        close(true);
        fd_ = fd;
    }
#ifdef _WIN64
    if (fd_ > -1)
    {
        handle_ = (HANDLE)_get_osfhandle(fd_);
    }
#endif
    return *this;
}

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
        int ret = 1;
        if (fd_ > -1)
        {
            ret = close_fd(fd_);
        }
        fd_ = -1;
        return ret;
    }
    return 1;
}

int fstream::engine_read(void *buf, int len)
{
#ifdef _WIN64
    DWORD rlen;
    if (ReadFile(handle_, buf, len, &rlen, 0))
    {
        return (int)rlen;
    }
    else
    {
        return -1;
    }
#else  // _WIN64
    return (int)::read(fd_, buf, len);
#endif // _WIN64
}

int fstream::engine_write(const void *buf, int len)
{
#ifdef _WIN64
    DWORD rlen;
    if (WriteFile(handle_, buf, len, &rlen, 0))
    {
        return (int)rlen;
    }
    else
    {
        return -1;
    }
#else  // _WIN64
    return (int)::write(fd_, buf, len);
#endif // _WIN64
}

zcc_namespace_end;

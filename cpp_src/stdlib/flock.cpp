/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2025-06-09
 * ================================
 */

#include "zcc/zcc_win64.h"
#include "zcc/zcc_errno.h"
#include <stdio.h>
#ifdef _WIN64
#include <winbase.h>
#include <fileapi.h>
#include <shellapi.h>
#include <shlobj.h>
#include <wchar.h>
#include <sys/utime.h>
#include <direct.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#else // _WIN64
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <utime.h>
#include <stdarg.h>
#include <dirent.h>
#endif // _WIN64

zcc_namespace_begin;

// 下面是夸平台的flock实现方案

#ifdef _WIN64

static inline int __flock(int fd, bool exclusive, DWORD action)
{
    HANDLE hd = (HANDLE)_get_osfhandle(fd);
    if (hd == INVALID_HANDLE_VALUE)
    {
        return -1;
    }
    if (LockFileEx(hd, action, 0, 1, 0, NULL))
    {
        return 1;
    }
    auto err = GetLastError();
    if (ERROR_LOCK_VIOLATION == err || ERROR_SHARING_VIOLATION == err)
    {
        return 0;
    }
    return -1;
}
int flock(int fd, bool exclusive)
{
    DWORD action = exclusive ? LOCKFILE_EXCLUSIVE_LOCK : 0;
    return __flock(fd, exclusive, action);
}

int try_flock(int fd, bool exclusive)
{
    DWORD action = exclusive ? LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY : LOCKFILE_FAIL_IMMEDIATELY;
    return __flock(fd, exclusive, action);
}

bool funlock(int fd)
{
    HANDLE hd = (HANDLE)_get_osfhandle(fd);
    if (hd == INVALID_HANDLE_VALUE)
    {
        return false;
    }
    if (UnlockFileEx(hd, 0, 1, 0, NULL))
    {
        return true;
    }
    return false;
}
#else  // _WIN64
int flock(int fd, bool exclusive)
{
    while (1)
    {
        if (::flock(fd, exclusive ? LOCK_EX : LOCK_SH) == 0)
        {
            return 1;
        }
        if (errno == EINTR)
        {
            continue;
        }
        return -1;
    }
    return 0;
}

int try_flock(int fd, bool exclusive)
{
    while (1)
    {
        if (::flock(fd, exclusive ? LOCK_EX | LOCK_NB : LOCK_SH | LOCK_NB) == 0)
        {
            return 1;
        }
        if (errno == EINTR)
        {
            continue;
        }
        if (errno == EWOULDBLOCK)
        {
            return 0;
        }
        return -1;
    }
    return 0;
}

bool funlock(int fd)
{
    return ::flock(fd, LOCK_UN) == 0;
}
#endif // _WIN64

flocker::flocker()
{
}

flocker::~flocker()
{
    unlock();
}

int flocker::__lock(const std::string &pathname, bool exclusive, bool try_mode)
{
    if (pathname != pathname_)
    {
        unlock();
    }
    if (fd_ == -1)
    {
        fd_ = open(pathname.c_str(), O_RDONLY | O_CREAT, 0666);
        if (fd_ == -1)
        {
            return -1;
        }
    }
    int r;
    if (try_mode)
    {
        r = try_flock(fd_, exclusive);
    }
    else
    {
        r = flock(fd_, exclusive);
    }
    if (r == 1)
    {
        pathname_ = pathname;
        return 1;
    }
    return r;
}

int flocker::unlock()
{
    pathname_.clear();
    if (fd_ != -1)
    {
#ifdef _WIN64
        ::_close(fd_);
#else  // _WIN64
        ::close(fd_);
#endif // _WIN64
    }
    fd_ = -1;
    return 1;
}
zcc_namespace_end;

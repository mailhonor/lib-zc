/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-12-04
 * ================================
 */

#include "zcc/zcc_stdlib.h"
#include "zcc/zcc_win64.h"
#include "zcc/zcc_errno.h"
#ifdef _WIN64
#include <windows.h>
#include <wchar.h>
#include <direct.h>
#else // _WIN64
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#endif // _WIN64

#ifndef Z_MAX_PATH
#define Z_MAX_PATH 4096
#endif // Z_MAX_PATH

zcc_namespace_begin;

mmap_reader::mmap_reader()
{
}

mmap_reader::~mmap_reader()
{
    close();
}

int mmap_reader::open(const char *pathname)
{
#ifdef _WIN64
    HANDLE fd, fm;
    int64_t size;
    void *data;

    fd_ = INVALID_HANDLE_VALUE;
    data_ = nullptr;
    size_ = 0;

    wchar_t pathnamew[Z_MAX_PATH + 1];
    if (Utf8ToWideChar(pathname, -1, pathnamew, Z_MAX_PATH) < 1)
    {
        return -1;
    }
    fd = CreateFileW(pathnamew,
                     GENERIC_READ,
                     FILE_SHARE_READ,
                     NULL,
                     OPEN_EXISTING,
                     FILE_ATTRIBUTE_NORMAL,
                     NULL);
    if (fd == INVALID_HANDLE_VALUE)
    {
        return -1;
    }

    LARGE_INTEGER info;
    memset(&info, 0, sizeof(info));
    GetFileSizeEx(fd, &info);
    size = info.QuadPart;
    if (size < 0)
    {
        CloseHandle(fd);
        return -1;
    }

    fm = CreateFileMapping(
        fd,
        NULL,
        PAGE_READONLY,
        0,
        0,
        NULL);

    if (NULL == INVALID_HANDLE_VALUE)
    {
        CloseHandle(fd);
        return -1;
    }

    data = MapViewOfFile(
        fm,
        FILE_MAP_READ,
        0,
        0,
        0);

    if (NULL == data)
    {
        if (size == 0)
        {
            data = (void *)var_blank_buffer;
        }
        else
        {
            CloseHandle(fm);
            CloseHandle(fd);
            return -1;
        }
    }
    fd_ = fd;
    fm_ = fm;
    data_ = (const char *)data;
    size_ = size;
    return 1;
#else  // _WIN64
    int fd;
    int64_t size;
    void *data;
    struct stat st;
    int errno2;

    while (((fd = ::open(pathname, O_RDONLY)) == -1) && (get_errno() == ZCC_EINTR))
    {
        continue;
    }
    if (fd == -1)
    {
        return -1;
    }
    if (::fstat(fd, &st) == -1)
    {
        errno2 = get_errno();
        ::close(fd);
        set_errno(errno2);
        return -1;
    }
    size = st.st_size;
    data = mmap(NULL, size + 1, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED)
    {
        errno2 = get_errno();
        ::close(fd);
        set_errno(errno2);
        return -1;
    }

    fd_ = fd;
    size_ = size;
    data_ = (char *)data;

    return 1;
#endif // _WIN64
    return 1;
}

int mmap_reader::close()
{
#ifdef _WIN64
    if (fd_ == INVALID_HANDLE_VALUE)
    {
        return 1;
    }
    UnmapViewOfFile(data_);
    CloseHandle(fm_);
    CloseHandle(fd_);
    fd_ = INVALID_HANDLE_VALUE;
    return 1;
#else  // _WIN64
    if (fd_ < 1)
    {
        return 1;
    }
    munmap((void *)data_, size_ + 1);
    ::close(fd_);
    fd_ = -1;
    return 1;
#endif // _WIN64
}

zcc_namespace_end;

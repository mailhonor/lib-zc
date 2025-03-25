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

/**
 * @brief 构造函数，初始化 mmap_reader 对象
 */
mmap_reader::mmap_reader()
{
}

/**
 * @brief 析构函数，在对象销毁时关闭内存映射
 */
mmap_reader::~mmap_reader()
{
    close();
}

/**
 * @brief 打开指定路径的文件并进行内存映射
 * @param pathname 文件的路径
 * @return 成功返回 1，失败返回 -1
 */
int mmap_reader::open(const char *pathname)
{
#ifdef _WIN64
    // Windows 系统下的实现
    HANDLE fd, fm;  // 文件句柄和文件映射对象句柄
    int64_t size;   // 文件大小
    void *data;     // 内存映射的数据指针

    // 初始化成员变量
    fd_ = INVALID_HANDLE_VALUE;
    data_ = nullptr;
    size_ = 0;

    // 将 UTF-8 编码的路径转换为宽字符路径
    wchar_t pathnamew[Z_MAX_PATH + 1];
    if (Utf8ToWideChar(pathname, -1, pathnamew, Z_MAX_PATH) < 1)
    {
        // 转换失败，返回 -1
        return -1;
    }

    // 打开文件
    fd = CreateFileW(pathnamew,
                     GENERIC_READ,       // 以只读模式打开
                     FILE_SHARE_READ,    // 允许其他进程以只读模式共享
                     NULL,
                     OPEN_EXISTING,      // 打开已存在的文件
                     FILE_ATTRIBUTE_NORMAL,
                     NULL);
    if (fd == INVALID_HANDLE_VALUE)
    {
        // 打开文件失败，返回 -1
        return -1;
    }

    // 获取文件大小
    LARGE_INTEGER info;
    memset(&info, 0, sizeof(info));
    GetFileSizeEx(fd, &info);
    size = info.QuadPart;
    if (size < 0)
    {
        // 获取文件大小失败，关闭文件并返回 -1
        CloseHandle(fd);
        return -1;
    }

    // 创建文件映射对象
    fm = CreateFileMapping(
        fd,
        NULL,
        PAGE_READONLY,  // 只读映射
        0,
        0,
        NULL);

    if (fm == INVALID_HANDLE_VALUE)
    {
        // 创建文件映射对象失败，关闭文件并返回 -1
        CloseHandle(fd);
        return -1;
    }

    // 映射文件视图
    data = MapViewOfFile(
        fm,
        FILE_MAP_READ,  // 只读映射
        0,
        0,
        0);

    if (NULL == data)
    {
        if (size == 0)
        {
            // 文件大小为 0，使用空白缓冲区
            data = (void *)var_blank_buffer;
        }
        else
        {
            // 映射失败，关闭文件映射对象和文件并返回 -1
            CloseHandle(fm);
            CloseHandle(fd);
            return -1;
        }
    }

    // 更新成员变量
    fd_ = fd;
    fm_ = fm;
    data_ = (const char *)data;
    size_ = size;
    return 1;
#else  // _WIN64
    // Linux 等非 Windows 系统下的实现
    int fd;          // 文件描述符
    int64_t size;    // 文件大小
    void *data;      // 内存映射的数据指针
    struct stat st;  // 文件状态结构体
    int errno2;      // 保存错误码

    // 尝试打开文件，处理被信号中断的情况
    while (((fd = ::open(pathname, O_RDONLY)) == -1) && (get_errno() == ZCC_EINTR))
    {
        continue;
    }
    if (fd == -1)
    {
        // 打开文件失败，返回 -1
        return -1;
    }

    // 获取文件状态
    if (::fstat(fd, &st) == -1)
    {
        errno2 = get_errno();
        ::close(fd);
        set_errno(errno2);
        return -1;
    }

    size = st.st_size;

    // 进行内存映射
    data = mmap(NULL, size + 1, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED)
    {
        errno2 = get_errno();
        ::close(fd);
        set_errno(errno2);
        return -1;
    }

    // 更新成员变量
    fd_ = fd;
    size_ = size;
    data_ = (char *)data;

    return 1;
#endif // _WIN64
    return 1;
}

/**
 * @brief 关闭内存映射并释放资源
 * @return 成功返回 1
 */
int mmap_reader::close()
{
#ifdef _WIN64
    // Windows 系统下的实现
    if (fd_ == INVALID_HANDLE_VALUE)
    {
        // 文件句柄无效，直接返回 1
        return 1;
    }
    // 取消内存映射
    UnmapViewOfFile(data_);
    // 关闭文件映射对象句柄
    CloseHandle(fm_);
    // 关闭文件句柄
    CloseHandle(fd_);
    fd_ = INVALID_HANDLE_VALUE;
    return 1;
#else  // _WIN64
    // Linux 等非 Windows 系统下的实现
    if (fd_ < 0)
    {
        // 文件描述符无效，直接返回 1
        return 1;
    }
    // 取消内存映射
    munmap((void *)data_, size_ + 1);
    // 关闭文件描述符
    ::close(fd_);
    fd_ = -1;
    return 1;
#endif // _WIN64
}

zcc_namespace_end;

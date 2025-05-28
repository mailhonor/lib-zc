/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-12-04
 * ================================
 */

#include "zcc/zcc_win64.h"
#include "zcc/zcc_errno.h"
#include <stdio.h>
#ifdef _WIN64
#include <winbase.h>
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

// 如果未定义 Z_MAX_PATH，则定义其值为 10240
#ifndef Z_MAX_PATH
#define Z_MAX_PATH 10240
#endif // Z_MAX_PATH

#ifdef _WIN64
// 定义 Windows 64 位系统下的路径分隔符
#define path_splitor '\\'
#else // _WIN64
// 定义非 Windows 64 位系统下的路径分隔符
#define path_splitor '/'
#endif // _WIN64

/**
 * @brief 健壮地执行一个表达式，处理 EINTR 错误
 * @param exp 要执行的表达式
 * @return 表达式的返回值，如果出错则返回 -1
 */
#define ROBUST_DO(exp)        \
    int ret = -1;             \
    while (1)                 \
    {                         \
        ret = exp;            \
        if (ret > -1)         \
        {                     \
            return ret;       \
        }                     \
        int ec = get_errno(); \
        if (ec == ZCC_EINTR)  \
        {                     \
            continue;         \
        }                     \
        set_errno(ec);        \
        return ret;           \
    }                         \
    return ret;

/**
 * @brief 健壮地执行一个表达式，处理 EINTR 错误，并在遇到指定错误时返回 0
 * @param exp 要执行的表达式
 * @param E 指定的错误码
 * @return 表达式的返回值，如果出错则返回 -1，遇到指定错误返回 0
 */
#define ROBUST_DO_ONE_MORE(exp, E) \
    int ret = -1;                  \
    while (1)                      \
    {                              \
        ret = exp;                 \
        if (ret > -1)              \
        {                          \
            return ret;            \
        }                          \
        int ec = get_errno();      \
        if (ec == ZCC_EINTR)       \
        {                          \
            continue;              \
        }                          \
        if (ec == E)               \
        {                          \
            return 0;              \
        }                          \
        set_errno(ec);             \
        return ret;                \
    }                              \
    return ret;

zcc_namespace_begin;

/**
 * @brief 打开一个文件
 * @param pathname 要打开的文件路径
 * @param mode 打开文件的模式
 * @return 如果成功，返回文件指针；否则返回 0
 */
FILE *fopen(const char *pathname, const char *mode)
{
#ifdef _WIN64
    // 用于存储宽字符版本的文件路径
    wchar_t pathnamew[Z_MAX_PATH + 1];
    // 用于存储宽字符版本的打开模式
    wchar_t modew[64 + 1];
    // 计算打开模式的长度
    int mlen = std::strlen(mode);
    // 如果打开模式长度超过 10，返回 0
    if (mlen > 10)
    {
        return 0;
    }
    // 将 UTF-8 编码的文件路径转换为宽字符
    if (Utf8ToWideChar(pathname, -1, pathnamew, Z_MAX_PATH) < 1)
    {
        return 0;
    }
    // 将 UTF-8 编码的打开模式转换为宽字符
    if (Utf8ToWideChar(mode, mlen, modew, 64) < 1)
    {
        return 0;
    }
    // 调用 Windows 宽字符版本的 fopen 函数
    return ::_wfopen(pathnamew, modew);
#else  // _Win32
    // 调用标准的 fopen 函数
    return ::fopen(pathname, mode);
#endif // _Win32
}

#ifdef _WIN64
/**
 * @brief 从文件流中读取一行，直到遇到指定的分隔符
 * @param lineptr 指向存储读取内容的缓冲区指针
 * @param n 指向缓冲区大小的指针
 * @param delim 指定的分隔符
 * @param stream 文件流指针
 * @return 读取的字符数，如果出错则返回 -1
 */
int64_t getdelim(char **lineptr, int64_t *n, int delim, FILE *stream)
{
    // 检查参数是否为空
    if ((lineptr == 0) || (n == 0))
    {
        zcc_error("getdelim lineptr is NULL");
        return -1;
    }
    // 获取缓冲区大小
    int nn = *n, count = 0;
    // 如果缓冲区指针为空，分配初始缓冲区
    if (*lineptr == 0)
    {
        *n = 128;
        nn = *n;
        *lineptr = (char *)zcc::malloc(nn + 1);
    }
    while (1)
    {
        // 从文件流中读取一个字符
        int ch = ::fgetc(stream);
        // 如果到达文件末尾，退出循环
        if (ch == EOF)
        {
            break;
        }
        // 如果缓冲区空间不足，重新分配缓冲区
        if (count + 1 > nn)
        {
            nn *= 2;
            *lineptr = (char *)zcc::realloc(*lineptr, nn + 1);
        }
        // 将字符存储到缓冲区
        (*lineptr)[count] = ch;
        count++;
        // 如果遇到分隔符，退出循环
        if (ch == delim)
        {
            break;
        }
        continue;
    }
    // 在缓冲区末尾添加字符串结束符
    (*lineptr)[count] = 0;
    // 如果没有读取到任何字符，返回 -1
    if (count == 0)
    {
        return -1;
    }
    return count;
}
#endif // _WIN64

/**
 * @brief 获取文件的绝对路径
 * @param pathname 要获取绝对路径的文件路径
 * @return 绝对路径的字符串
 */
std::string realpath(const char *pathname)
{
    std::string r;
    // 检查路径是否为空
    if (empty(pathname))
    {
        return r;
    }
#ifdef _WIN64
    // 用于存储宽字符版本的文件路径
    wchar_t pathnamew[Z_MAX_PATH + 1];
    // 用于存储宽字符版本的绝对路径
    wchar_t result[Z_MAX_PATH + 1];
    // 将 UTF-8 编码的文件路径转换为宽字符
    if (Utf8ToWideChar(pathname, -1, pathnamew, Z_MAX_PATH) < 1)
    {
        return r;
    }
    // 获取文件的绝对路径
    int ret = GetFullPathNameW(pathnamew, Z_MAX_PATH, result, 0);
    if (ret < 1)
    {
        return r;
    }
    // 将宽字符版本的绝对路径转换为 UTF-8 编码
    r = WideCharToUTF8(result, ret);
    return r;
#else  // _Win32
    // 用于存储绝对路径的缓冲区
    char buf[PATH_MAX + 1];
    // 调用标准的 realpath 函数获取绝对路径
    char *p = ::realpath(pathname, buf);
    if (p)
    {
        r = p;
    }
    return r;
#endif // _Win32
}

#ifdef _WIN64
/**
 * @brief 获取文件的状态信息
 * @param pathname 要获取状态信息的文件路径
 * @param statbuf 指向存储状态信息的结构体指针
 * @return 如果成功，返回 0；否则返回 -1
 */
int stat(const char *pathname, struct _stat64i32 *statbuf)
{
    // 用于存储宽字符版本的文件路径
    wchar_t pathnamew[Z_MAX_PATH + 1];
    // 将 UTF-8 编码的文件路径转换为宽字符
    if (Utf8ToWideChar(pathname, -1, pathnamew, Z_MAX_PATH) < 1)
    {
        return 0;
    }
    // 调用 Windows 宽字符版本的 stat 函数
    return ::_wstat(pathnamew, statbuf);
}
#else  // _WIN64
/**
 * @brief 获取文件的状态信息
 * @param pathname 要获取状态信息的文件路径
 * @param statbuf 指向存储状态信息的结构体指针
 * @return 如果成功，返回 0；否则返回 -1
 */
int stat(const char *pathname, struct stat *statbuf)
{
    // 调用标准的 stat 函数
    return ::stat(pathname, statbuf);
}
#endif // _WIN64

/**
 * @brief 获取文件的大小
 * @param pathname 要获取大小的文件路径
 * @return 文件的大小，如果出错则返回 -1
 */
int64_t file_get_size(const char *pathname)
{
#ifdef _WIN64
    // 用于存储文件状态信息的结构体
    struct _stat64i32 st;
#else  // _WIN64
    // 用于存储文件状态信息的结构体
    struct stat st;
#endif // _WIN64
    // 调用 stat 函数获取文件状态信息
    if (zcc::stat(pathname, &st) == -1)
    {
        int ec = get_errno();
        // 如果文件不存在，返回 0
        if (ec == ZCC_ENOENT)
        {
            return 0;
        }
        return -1;
    }
    return st.st_size;
}

/**
 * @brief 检查文件是否存在
 * @param pathname 要检查的文件路径
 * @return 如果存在，返回 1；如果不存在，返回 0；如果出错，返回 -1
 */
int file_exists(const char *pathname)
{
#ifdef _WIN64
    // 用于存储文件状态信息的结构体
    struct _stat64i32 st;
#else  // _WIN64
    // 用于存储文件状态信息的结构体
    struct stat st;
#endif // _WIN64
    // 调用 stat 函数获取文件状态信息
    if (zcc::stat(pathname, &st) == -1)
    {
        int ec = get_errno();
        // 如果文件不存在，返回 0
        if (ec == ZCC_ENOENT)
        {
            return 0;
        }
        return -1;
    }
    return 1;
}

int file_is_regular(const char *pathname)
{
    zcc_stat st;
    int ret = zcc::stat(pathname, &st);
    if (ret == -1)
    {
        int ec = get_errno();
        // 如果文件不存在，返回 0
        if (ec == ZCC_ENOENT)
        {
            return 0;
        }
        return -1;
    }
#ifdef _WIN64
    if (st.st_mode & _S_IFREG)
    {
        return 1;
    }
#else  // _WIN64
    if (S_ISREG(st.st_mode))
    {
        return 1;
    }
#endif // _WIN64
    return 0;
}

int file_is_dir(const char *pathname)
{
    zcc_stat st;
    int ret = zcc::stat(pathname, &st);
    if (ret == -1)
    {
        int ec = get_errno();
        // 如果文件不存在，返回 0
        if (ec == ZCC_ENOENT)
        {
            return 0;
        }
        return -1;
    }
#ifdef _WIN64
    if (st.st_mode & _S_IFDIR)
    {
        return 1;
    }
#else  // _WIN64
    if (S_ISDIR(st.st_mode))
    {
        return 1;
    }
#endif // _WIN64
    return 0;
}

/* ################################################################## */
/* file get/put contents */

/**
 * @brief 将数据写入文件
 * @param pathname 要写入的文件路径
 * @param data 要写入的数据
 * @param len 要写入的数据长度
 * @return 如果成功，返回 1；否则返回 -1
 */
int file_put_contents(const char *pathname, const void *data, int len)
{
    int ret;
    FILE *fp;

    // 打开文件
    if (!(fp = fopen(pathname, "wb+")))
    {
        return -1;
    }
    // 将数据写入文件
    fwrite(data, 1, len, fp);
    // 刷新文件缓冲区
    ret = fflush(fp);
    // 关闭文件
    fclose(fp);
    // 如果刷新缓冲区失败，返回 -1
    if (ret == EOF)
    {
        return -1;
    }
    return 1;
}

/**
 * @brief 从文件中读取内容到字符串
 * @param pathname 要读取的文件路径
 * @param bf 用于存储读取内容的字符串
 * @return 读取的字符数，如果出错则返回 -1
 */
int64_t file_get_contents(const char *pathname, std::string &bf)
{
    FILE *fp = 0;

    // 清空字符串
    bf.clear();
    // 打开文件
    if (!(fp = fopen(pathname, "rb")))
    {
        return -1;
    }
    while (1)
    {
        // 从文件中读取一个字符
        int ch = fgetc(fp);
        // 如果到达文件末尾，退出循环
        if (ch == EOF)
        {
            break;
        }
        // 将字符添加到字符串中
        bf.push_back(ch);
    }
    // 检查文件是否出错
    int ret = ferror(fp);
    // 关闭文件
    fclose(fp);
    // 如果文件出错，返回 -1
    if (ret)
    {
        return -1;
    }
    return bf.size();
}

/**
 * @brief 从文件中读取内容并返回字符串
 * @param pathname 要读取的文件路径
 * @return 包含文件内容的字符串
 */
std::string file_get_contents(const char *pathname)
{
    std::string r;
    file_get_contents(pathname, r);
    return r;
}

/**
 * @brief 从文件中读取内容到字符串，如果出错则退出程序
 * @param pathname 要读取的文件路径
 * @param bf 用于存储读取内容的字符串
 * @return 读取的字符数
 */
int64_t file_get_contents_sample(const char *pathname, std::string &bf)
{
    int64_t ret = file_get_contents(pathname, bf);
    // 如果读取失败，输出错误信息并退出程序
    if (ret < 0)
    {
        zcc_error("load from %s", pathname);
        exit(1);
    }
    return ret;
}

/**
 * @brief 从文件中读取内容并返回字符串，如果出错则退出程序
 * @param pathname 要读取的文件路径
 * @return 包含文件内容的字符串
 */
std::string file_get_contents_sample(const char *pathname)
{
    std::string r;
    file_get_contents_sample(pathname, r);
    return r;
}

/**
 * @brief 从标准输入读取内容到字符串
 * @param bf 用于存储读取内容的字符串
 * @return 读取的字符数
 */
int stdin_get_contents(std::string &bf)
{
    // 清空字符串
    bf.clear();
    while (1)
    {
        // 从标准输入读取一个字符
        int ch = fgetc(stdin);
        // 如果到达文件末尾，退出循环
        if (ch == EOF)
        {
            break;
        }
        // 将字符添加到字符串中
        bf.push_back(ch);
    }
    return bf.size();
}

/**
 * @brief 从标准输入读取内容并返回字符串
 * @return 包含标准输入内容的字符串
 */
std::string stdin_get_contents()
{
    std::string r;
    stdin_get_contents(r);
    return r;
}

int file_copy(const char *sourcePathname, const char *destPathname)
{
    FILE *sourceFile, *destFile;
    char buffer[1024 + 1];
    size_t bytesRead;

    // 打开源文件（附件），以二进制读模式
    sourceFile = fopen(sourcePathname, "rb");
    if (sourceFile == NULL)
    {
        zcc_debug("无法打开源文件");
        return -1;
    }

    // 打开目标文件，以二进制写模式，如果文件不存在则创建
    destFile = fopen(destPathname, "wb");
    if (destFile == NULL)
    {
        zcc_debug("无法创建目标文件");
        fclose(sourceFile);
        return -1;
    }

    // 使用缓冲区读取和写入文件
    while ((bytesRead = fread(buffer, 1, 1024, sourceFile)) > 0)
    {
        fwrite(buffer, 1, bytesRead, destFile);
    }

    // 关闭文件
    fclose(sourceFile);
    fflush(destFile);
    int ret = ferror(destFile);
    fclose(destFile);
    if (ret)
    {
        return -1;
    }
    return 1;
}

#ifdef _WIN64
/**
 * @brief Windows 64 位系统下打开文件的辅助函数
 * @param pathname 要打开的文件路径
 * @param flags 打开文件的标志
 * @param mode 文件权限模式
 * @return 如果成功，返回文件描述符；否则返回 0
 */
static int _open_win64(const char *pathname, int flags, int mode)
{
    // 用于存储宽字符版本的文件路径
    wchar_t pathnamew[Z_MAX_PATH + 1];
    // 将 UTF-8 编码的文件路径转换为宽字符
    if (Utf8ToWideChar(pathname, -1, pathnamew, Z_MAX_PATH) < 1)
    {
        return 0;
    }
    // 调用 Windows 宽字符版本的 open 函数
    return ::_wopen(pathnamew, flags, mode);
}
#endif // _WIN64

/**
 * @brief 打开一个文件
 * @param pathname 要打开的文件路径
 * @param flags 打开文件的标志
 * @param mode 文件权限模式
 * @return 如果成功，返回文件描述符；否则返回 -1
 */
int open(const char *pathname, int flags, int mode)
{
#ifdef _WIN64
    // 健壮地调用 Windows 64 位系统下的打开文件函数
    ROBUST_DO(_open_win64(pathname, flags, mode));
#else  // _WIN64
    // 健壮地调用标准的 open 函数
    ROBUST_DO(::open(pathname, flags, mode));
#endif // _WIN64
}

/**
 * @brief 创建或更新文件的访问和修改时间
 * @param pathname 要操作的文件路径
 * @return 如果成功，返回 1；否则返回 -1
 */
int touch(const char *pathname)
{
    int fd = -1;
#ifdef _WIN64
    // 打开文件
    if ((fd = _open_win64(pathname, O_RDWR | O_CREAT, 0666)) < 0)
    {
        return -1;
    }
    // 关闭文件
    ::close(fd);
    return 1;
#else  // _WIN64
    // 打开文件
    if ((fd = open(pathname, O_RDWR | O_CREAT, 0666)) < 0)
    {
        return -1;
    }
    // 更新文件的访问和修改时间
    if (futimens(fd, 0) < 0)
    {
        // 关闭文件
        ::close(fd);
        return -1;
    }
    // 关闭文件
    ::close(fd);
    return 1;
#endif // _WIN64
}

/**
 * @brief 创建一个目录
 * @param pathname 要创建的目录路径
 * @param mode 目录权限模式
 * @return 如果成功，返回 0；否则返回 -1
 */
int mkdir(const char *pathname, int mode)
{
#ifdef _WIN64
    // 用于存储宽字符版本的目录路径
    wchar_t pathnamew[Z_MAX_PATH + 1];
    // 将 UTF-8 编码的目录路径转换为宽字符
    if (Utf8ToWideChar(pathname, -1, pathnamew, Z_MAX_PATH) < 1)
    {
        return 0;
    }
    // 调用 Windows 宽字符版本的 mkdir 函数
    return ::_wmkdir(pathnamew);
#else  // _WIN64
    // 调用标准的 mkdir 函数
    return ::mkdir(pathname, mode);
#endif // _WIN64
}

/**
 * @brief 根据路径列表创建多级目录
 * @param paths 路径列表
 * @param mode 目录权限模式
 * @return 如果成功，返回 1；否则返回 -1
 */
int mkdir(std::vector<std::string> paths, int mode)
{
    int r = -1, ret;
#ifdef _WIN64
    // 用于存储文件状态信息的结构体
    struct _stat64i32 st;
#else  // _WIN64
    // 用于存储文件状态信息的结构体
    struct stat st;
#endif // _WIN64
    std::string tmppath;
    char pathbuf[10240 + 1];
    char *path;
    unsigned char *ps, *p;
    int saved_ch;

    // 遍历路径列表，拼接路径
    for (auto it = paths.begin(); it != paths.end(); it++)
    {
        const char *path = it->c_str();
        if (!tmppath.empty())
        {
            if ((tmppath.back() != '/')
#ifdef _WIN64
                && (tmppath.back() != '\\')
#endif // _WIN64
            )
            {
                tmppath.push_back(path_splitor);
            }
        }

        if (empty(path))
        {
            break;
        }
        if ((path[0] == '/') && (!tmppath.empty()))
        {
            path++;
        }
#ifdef _WIN64
        if (path[0] == '\\')
        {
            path++;
        }
#endif // _WIN64
        tmppath.append(path);
    }

#ifdef _WIN64
    // 将路径中的斜杠替换为反斜杠
    for (uint64_t i = 0; i < tmppath.size(); i++)
    {
        if (tmppath[i] == '/')
        {
            tmppath[i] = '\\';
        }
    }
#endif // _WIN64
    // 检查路径长度是否超过限制
    if (tmppath.size() > 10240)
    {
        goto over;
    }

    // 检查目录是否存在
    if ((ret = zcc::stat(tmppath.c_str(), &st)) < 0)
    {
        if (get_errno() == ZCC_ENOTDIR)
        {
            goto over;
        }
    }
    else
    {
#ifdef _WIN64
        // 检查是否为目录
        if (!(st.st_mode & _S_IFDIR))
        {
            set_errno(ZCC_ENOTDIR);
            goto over;
        }
#else  // _WIN64
       // 检查是否为目录
        if (!S_ISDIR(st.st_mode))
        {
            set_errno(ZCC_ENOTDIR);
            goto over;
        }
#endif // _WIN64
        r = 1;
        goto over;
    }

    // 将路径复制到缓冲区
    std::memcpy(pathbuf, tmppath.c_str(), tmppath.size());
    pathbuf[tmppath.size()] = 0;
    path = pathbuf;
    ps = (unsigned char *)path;
    saved_ch = -1;
    // 遍历路径，逐级创建目录
    for (; ps;)
    {
        if (saved_ch > -1)
        {
            ps[0] = saved_ch;
        }
        p = (unsigned char *)std::strchr((char *)ps, path_splitor);
        if (p)
        {
            ps = p + 1;
            saved_ch = *ps;
            *ps = 0;
        }
        else
        {
            ps = 0;
        }
        // 检查目录是否存在
        if ((ret = zcc::stat(path, &st)) < 0)
        {
            if (get_errno() == ZCC_ENOTDIR)
            {
                goto over;
            }
        }
        else
        {
#ifdef _WIN64
            // 检查是否为目录
            if (!(st.st_mode & _S_IFDIR))
            {
                set_errno(ZCC_ENOTDIR);
                goto over;
            }
#else  // _WIN64
       // 检查是否为目录
            if (!S_ISDIR(st.st_mode))
            {
                set_errno(ZCC_ENOTDIR);
                goto over;
            }
#endif // _WIN64
            continue;
        }

        // 创建目录
        if ((ret = mkdir(path, mode)) < 0)
        {
            {
                int ec = get_errno();
                // 如果目录已存在，继续创建下一级目录
                if (ec != ZCC_EEXIST)
                {
                    goto over;
                }
                continue;
            }
        }
    }
    r = 1;
over:
    return r;
}

/**
 * @brief 根据可变参数列表创建多级目录
 * @param mode 目录权限模式
 * @param path1 第一个路径
 * @param ... 可变参数列表，包含其他路径
 * @return 如果成功，返回 1；否则返回 -1
 */
int mkdir(int mode, const char *path1, ...)
{
    char *path;
    std::vector<std::string> paths;
    // 将第一个路径添加到路径列表中
    paths.push_back(path1);
    va_list ap;
    // 初始化可变参数列表
    va_start(ap, path1);
    // 遍历可变参数列表，将路径添加到路径列表中
    while ((path = (char *)va_arg(ap, char *)))
    {
        paths.push_back(path);
    }
    // 结束可变参数列表
    va_end(ap);
    // 调用根据路径列表创建多级目录的函数
    return mkdir(paths, mode);
}

/**
 * @brief 重命名文件或目录
 * @param oldpath 旧的文件或目录路径
 * @param newpath 新的文件或目录路径
 * @return 如果成功，返回 0；否则返回 -1
 */
int rename(const char *oldpath, const char *newpath)
{
#ifdef _WIN64
    // 用于存储宽字符版本的旧路径
    wchar_t oldpathw[Z_MAX_PATH + 1];
    // 用于存储宽字符版本的新路径
    wchar_t newpathw[Z_MAX_PATH + 1];
    // 将 UTF-8 编码的旧路径和新路径转换为宽字符
    if ((Utf8ToWideChar(oldpath, -1, oldpathw, Z_MAX_PATH) < 1) || (Utf8ToWideChar(newpath, -1, newpathw, Z_MAX_PATH) < 1))
    {
        return -1;
    }
    // 调用 Windows 宽字符版本的重命名函数
    if (!MoveFileExW(oldpathw, newpathw, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED))
    {
        return -1;
    }
    return 0;
#else  // _WIN64
    // 健壮地调用标准的 rename 函数
    ROBUST_DO(::rename(oldpath, newpath));
#endif // _WIN64
}

/**
 * @brief 删除文件
 * @param pathname 要删除的文件路径
 * @return 如果成功，返回 0；如果文件不存在，返回 0；否则返回 -1
 */
int unlink(const char *pathname)
{
    if (empty(pathname))
    {
        return 0;
    }
#ifdef _WIN64
    // 用于存储宽字符版本的文件路径
    wchar_t pathnamew[Z_MAX_PATH + 1];
    // 将 UTF-8 编码的文件路径转换为宽字符
    if (Utf8ToWideChar(pathname, -1, pathnamew, Z_MAX_PATH) < 1)
    {
        return -1;
    }
    // 调用 Windows 宽字符版本的删除文件函数
    if (!DeleteFileW(pathnamew))
    {
        int ec = get_errno();
        // 如果文件不存在，返回 0
        if (ec == ZCC_ENOENT)
        {
            return 0;
        }
        return -1;
    }
    return 0;
#else  // _WIN64
    // 健壮地调用标准的 unlink 函数，处理文件不存在的情况
    ROBUST_DO_ONE_MORE(::unlink(pathname), ZCC_ENOENT);
#endif // _WIN64
}

/**
 * @brief 创建硬链接
 * @param oldpath 源文件路径
 * @param newpath 硬链接文件路径
 * @return 如果成功，返回 0；否则返回 -1
 */
int link(const char *oldpath, const char *newpath)
{
#ifdef _WIN64
    // 用于存储宽字符版本的源文件路径
    wchar_t oldpathw[Z_MAX_PATH + 1];
    // 用于存储宽字符版本的硬链接文件路径
    wchar_t newpathw[Z_MAX_PATH + 1];
    // 将 UTF-8 编码的源文件路径和硬链接文件路径转换为宽字符
    if ((Utf8ToWideChar(oldpath, -1, oldpathw, Z_MAX_PATH) < 1) || (Utf8ToWideChar(newpath, -1, newpathw, Z_MAX_PATH) < 1))
    {
        return -1;
    }
    // 调用 Windows 宽字符版本的创建硬链接函数
    if (!CreateHardLinkW(newpathw, oldpathw, NULL))
    {
        return -1;
    }
    return 0;
#else  // _WIN64
    // 健壮地调用标准的 link 函数
    ROBUST_DO(::link(oldpath, newpath));
#endif // _WIN64
}

/**
 * @brief 强制创建硬链接，如果目标文件已存在则先创建临时链接再替换
 * @param oldpath 源文件路径
 * @param newpath 硬链接文件路径
 * @param tmpdir 临时目录路径
 * @return 如果成功，返回 0；否则返回 -1
 */
int link_force(const char *oldpath, const char *newpath, const char *tmpdir)
{
    // 尝试创建硬链接
    int ret = link(oldpath, newpath);
    if (ret == 0)
    {
        return 0;
    }
    int ec = get_errno();
    // 如果目标文件不存在，返回 -1
    if (ec != ZCC_EEXIST)
    {
        return -1;
    }

    std::string tmppath;
    // 拼接临时文件路径
    tmppath.append(tmpdir).append("/");
    tmppath.append(build_unique_id());

    // 在临时路径创建硬链接
    ret = link(oldpath, tmppath.c_str());
    if (ret < 0)
    {
        return -1;
    }
    // 重命名临时文件为目标文件
    ret = rename(tmppath.c_str(), newpath);
    // 删除临时文件
    unlink(tmppath.c_str());
    if (ret < 0)
    {
        return -1;
    }
    return 0;
}

/**
 * @brief 创建符号链接
 * @param oldpath 源文件或目录路径
 * @param newpath 符号链接路径
 * @return 如果成功，返回 0；否则返回 -1
 */
int symlink(const char *oldpath, const char *newpath)
{
#ifdef _WIN64
    DWORD attr;
    BOOLEAN res;
    // 用于存储宽字符版本的源文件或目录路径
    wchar_t oldpathw[Z_MAX_PATH + 1];
    // 用于存储宽字符版本的符号链接路径
    wchar_t newpathw[Z_MAX_PATH + 1];
    // 将 UTF-8 编码的源文件或目录路径和符号链接路径转换为宽字符
    if ((Utf8ToWideChar(oldpath, -1, oldpathw, Z_MAX_PATH) < 1) || (Utf8ToWideChar(newpath, -1, newpathw, Z_MAX_PATH) < 1))
    {
        return -1;
    }
    // 获取源文件或目录的属性
    if ((attr = GetFileAttributesW(oldpathw)) == INVALID_FILE_ATTRIBUTES)
    {
        return -1;
    }
    // 调用 Windows 宽字符版本的创建符号链接函数
    res = CreateSymbolicLinkW(newpathw, oldpathw, (attr & FILE_ATTRIBUTE_DIRECTORY ? 1 : 0));
    if (!res)
    {
        return -1;
    }
    return 0;
#else  // _WIN64
    // 健壮地调用标准的 symlink 函数
    ROBUST_DO(::symlink(oldpath, newpath));
#endif // _WIN64
}

/**
 * @brief 强制创建符号链接，如果目标文件已存在则先创建临时链接再替换
 * @param oldpath 源文件或目录路径
 * @param newpath 符号链接路径
 * @param tmpdir 临时目录路径
 * @return 如果成功，返回 0；否则返回 -1
 */
int symlink_force(const char *oldpath, const char *newpath, const char *tmpdir)
{
    // 尝试创建符号链接
    int ret = symlink(oldpath, newpath);
    if (ret == 0)
    {
        return 0;
    }
    int ec = get_errno();
    // 如果目标文件不存在，返回 -1
    if (ec != ZCC_EEXIST)
    {
        return -1;
    }

    std::string tmppath;
    // 拼接临时文件路径
    tmppath.append(tmpdir).append("/");
    tmppath.append(build_unique_id());
    // 在临时路径创建符号链接
    ret = symlink(oldpath, tmppath.c_str());
    if (ret < 0)
    {
        return -1;
    }
    // 重命名临时文件为目标文件
    ret = rename(tmppath.c_str(), newpath);
    if (ret < 0)
    {
        // 删除临时文件
        unlink(tmppath.c_str());
        return -1;
    }
    return 0;
}

/**
 * @brief 实际执行删除目录的操作
 * @param pathname 要删除的目录路径
 * @return 如果成功，返回 0；如果目录不存在，返回 0；否则返回 -1
 */
static int _rmdir_true_do(const char *pathname)
{
#ifdef _WIN64
    // 用于存储宽字符版本的目录路径
    wchar_t pathnamew[Z_MAX_PATH + 1];
    // 将 UTF-8 编码的目录路径转换为宽字符
    if (Utf8ToWideChar(pathname, -1, pathnamew, Z_MAX_PATH) < 1)
    {
        return -1;
    }
    // 调用 Windows 宽字符版本的删除目录函数
    if (_wrmdir(pathnamew) < 0)
    {
        int ec = get_errno();
        // 如果目录不存在，返回 0
        if (ec == ZCC_ENOENT)
        {
            return 0;
        }
        set_errno(ec);
        return -1;
    }
    return 0;
#else  // _WIN64
    // 健壮地调用标准的 rmdir 函数，处理目录不存在的情况
    ROBUST_DO_ONE_MORE(::rmdir(pathname), ENOENT);
#endif // _WIN64
}

/**
 * @brief 删除目录，可以选择递归删除
 * @param pathname 要删除的目录路径
 * @param recurse_mode 是否递归删除
 * @return 如果成功，返回 1；否则返回 -1
 */
int rmdir(const char *pathname, bool recurse_mode)
{
    std::string path;
    std::string filename;
    std::vector<dir_item_info> items;
    int ret = -1;

    // 如果不递归删除，直接调用实际删除目录的函数
    if (!recurse_mode)
    {
        return _rmdir_true_do(pathname);
    }

    // 扫描目录
    if ((ret = scandir(pathname, items)) < 0)
    {
        goto over;
    }
    // 遍历目录中的文件和子目录
    for (auto it = items.begin(); it != items.end(); it++)
    {
        path.clear();
        path.append(filename).append("/").append(it->filename);
        if (it->dir)
        {
            // 如果是目录，递归删除
            if (_rmdir_true_do(path.c_str()) < 1)
            {
                goto over;
            }
        }
        else
        {
            // 如果是文件，删除文件
            if (unlink(path.c_str()) < 1)
            {
                goto over;
            }
        }
    }
    ret = 1;

over:
    return ret;
}

/**
 * @brief 扫描目录，获取目录中的文件和子目录信息
 * @param dirname 要扫描的目录路径
 * @param filenames 用于存储目录中的文件和子目录信息的向量
 * @return 如果成功，返回 1；否则返回 -1
 */
int scandir(const char *dirname, std::vector<dir_item_info> &filenames)
{
#ifdef _WIN64
    int ret = -1;
    WIN32_FIND_DATAW FindFileData;
    HANDLE hFind;

    std::string tmpdirname = dirname;
    // 处理目录路径末尾的斜杠
    if (!tmpdirname.empty())
    {
        if ((tmpdirname.back() == '\\') || (tmpdirname.back() == '/'))
        {
            tmpdirname.pop_back();
        }
        tmpdirname.append("\\*.*");
    }
    dirname = tmpdirname.c_str();

    // 将 UTF-8 编码的目录路径转换为宽字符
    std::wstring pw = Utf8ToWideChar(dirname);
    // 查找目录中的文件和子目录
    hFind = FindFirstFileW(pw.c_str(), &FindFileData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        return -1;
    }
    while (1)
    {
        // 将宽字符版本的文件名转换为 UTF-8 编码
        std::string filename = WideCharToUTF8((wchar_t *)FindFileData.cFileName);
        const char *fn = filename.c_str();
        // 跳过当前目录和上级目录
        if (!((fn[0] == '.') && ((fn[1] == '\0') || ((fn[1] == '.') && (fn[2] == '\0')))))
        {
            dir_item_info item;
            item.filename = filename;
            // 判断是否为目录
            if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                item.dir = true;
            }
            // 将文件或子目录信息添加到向量中
            filenames.push_back(item);
        }
        // 查找下一个文件或子目录
        if (FindNextFileW(hFind, &FindFileData) == 0)
        {
            if (GetLastError() == ERROR_NO_MORE_FILES)
            {
                break;
            }
            goto over;
        }
    }
    ret = 1;

over:
    // 关闭查找句柄
    FindClose(hFind);
    return ret;
#else  // _WIN64
    DIR *dir;
    struct dirent *ent_list;

    // 打开目录
    if (!(dir = opendir(dirname)))
    {
        if (get_errno() == ZCC_ENOENT)
        {
            return 0;
        }
        zcc_error("访问文件夹失败:%s(%m)", dirname);
        return -1;
    }

    // 现代 Linux 系统，readdir 是线程安全的
    while ((ent_list = readdir(dir)))
    {
        const char *fn = ent_list->d_name;
        // 跳过当前目录和上级目录
        if ((fn[0] == '.') && ((fn[1] == '\0') || ((fn[1] == '.') && (fn[2] == '\0'))))
        {
            continue;
        }
        dir_item_info item;
        item.filename = fn;
        // 根据文件类型设置标志位
        if (ent_list->d_type == DT_BLK)
        {
            item.dev = true;
        }
        else if (ent_list->d_type == DT_CHR)
        {
            item.dev = true;
        }
        else if (ent_list->d_type == DT_DIR)
        {
            item.dir = true;
        }
        else if (ent_list->d_type == DT_FIFO)
        {
            item.fifo = true;
        }
        else if (ent_list->d_type == DT_LNK)
        {
            item.link = true;
        }
        else if (ent_list->d_type == DT_REG)
        {
            item.regular = true;
        }
        else if (ent_list->d_type == DT_SOCK)
        {
            item.socket = true;
        }
        // 将文件或子目录信息添加到向量中
        filenames.push_back(item);
    }
    // 关闭目录
    closedir(dir);
    return 1;
#endif // _WIN64
}

/**
 * @brief 扫描目录，返回目录中的文件和子目录信息向量
 * @param dirname 要扫描的目录路径
 * @return 包含目录中的文件和子目录信息的向量
 */
std::vector<dir_item_info> scandir(const char *dirname)
{
    std::vector<dir_item_info> filenames;
    // 调用扫描目录的函数
    scandir(dirname, filenames);
    return filenames;
}

/**
 * @brief 格式化文件名，去除非法字符
 * @param filename 要格式化的文件名
 * @return 格式化后的文件名
 */
std::string format_filename(const char *filename)
{
    std::string path;
    const char *ignore = "/\\|";
#ifdef _WIN64
    // Windows 64 位系统下的非法字符
    ignore = "\":<>?/\\|*";
#endif
    // 遍历文件名，去除非法字符
    for (const unsigned char *p = (const unsigned char *)(void *)filename; *p; p++)
    {
        int ch = *p;
        if (ch < 127)
        {
            if (!::std::isprint(ch))
            {
                ch = '_';
            }
            else if (std::strchr(ignore, ch))
            {
                ch = '_';
            }
        }
        path.push_back(ch);
    }
    return path;
}

/**
 * @brief 查找符合条件的文件，调用另一个重载函数
 * @param dir_or_file 目录或文件路径列表
 * @param pathname_match 文件名匹配模式
 * @return 包含符合条件的文件路径的向量
 */
std::vector<std::string> find_file_sample(std::vector<const char *> dir_or_file, const char *pathname_match)
{
    return find_file_sample(dir_or_file.data(), (int)dir_or_file.size(), pathname_match);
}

/**
 * @brief 查找符合条件的文件
 * @param dir_or_file 目录或文件路径列表
 * @param item_count 路径列表的元素个数
 * @param pathname_match 文件名匹配模式
 * @return 包含符合条件的文件路径的向量
 */
std::vector<std::string> find_file_sample(const char **dir_or_file, int item_count, const char *pathname_match)
{
    std::vector<std::string> r;
#ifdef _WIN64
    return r;
#else  // _WIN64
    std::map<std::string, bool> rs;
    char buf[4096 + 1];
    // 遍历路径列表
    for (int i = 0; i < item_count; i++)
    {
        const char *pathname = dir_or_file[i];
        struct stat st;
        // 获取文件状态信息
        if (::stat(pathname, &st) == -1)
        {
            if (errno == ENOENT)
            {
                continue;
            }
            zcc_error_and_exit("open %s(%m)", pathname);
        }
        if (S_ISREG(st.st_mode))
        {
            // 如果是普通文件，添加到结果列表中
            if (rs.find(pathname) == rs.end())
            {
                r.push_back(pathname);
                rs[pathname] = true;
            }
            continue;
        }
        else if (!S_ISDIR(st.st_mode))
        {
            zcc_debug("WARNING file must be regular file or directory %s", pathname);
            continue;
        }
        if (zcc::empty(pathname_match))
        {
            // 拼接查找命令
            std::sprintf(buf, "find \"%s\" -type f", pathname);
        }
        else
        {
            // 拼接查找命令，包含文件名匹配模式
            std::sprintf(buf, "find \"%s\" -type f -name \"%s\"", pathname, pathname_match);
        }
        // 执行查找命令
        FILE *fp = popen(buf, "r");
        if (!fp)
        {
            zcc_debug("ERROR popen: find \"%s\" -type f", pathname);
        }
        while (fgets(buf, 4096, fp))
        {
            char *p = strchr(buf, '\n');
            if (p)
            {
                *p = 0;
            }
            p = strchr(buf, '\r');
            if (p)
            {
                *p = 0;
            }
            // 将符合条件的文件路径添加到结果列表中
            if (rs.find(buf) == rs.end())
            {
                r.push_back(buf);
                rs[buf] = true;
            }
        }
        // 关闭文件指针
        fclose(fp);
    }
    return r;
#endif // _WIN64
}

/**
 * @brief 创建快捷方式
 * @param from 源文件或目录路径
 * @param to 快捷方式路径
 * @return 如果成功，返回 true；否则返回 false
 */
bool create_shortcut_link(const char *from, const char *to)
{
#ifdef _WIN64
    // 初始化 COM 库
    if (FAILED(CoInitialize(NULL)))
    {
        return false;
    }

    HRESULT hres;
    IShellLinkW *psl;

    // 创建 Shell 链接对象
    hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (LPVOID *)&psl);
    if (!SUCCEEDED(hres))
    {
        return false;
    }
    // 将 UTF-8 编码的源文件或目录路径转换为宽字符
    std::wstring from_path = Utf8ToWideChar(std::string(from));
    // 将 UTF-8 编码的快捷方式路径转换为宽字符
    std::wstring to_path = Utf8ToWideChar(std::string(to));

    // 设置 Shell 链接对象的路径和工作目录
    psl->SetPath((LPCWSTR)from_path.c_str());
    psl->SetWorkingDirectory((LPCWSTR)from_path.c_str());

    IPersistFile *ppf;
    // 查询接口，获取 IPersistFile 接口指针
    hres = psl->QueryInterface(IID_IPersistFile, (LPVOID *)&ppf);

    if (SUCCEEDED(hres))
    {
        // 保存快捷方式
        hres = ppf->Save((LPCOLESTR)to_path.c_str(), TRUE);
        // 释放 IPersistFile 接口指针
        ppf->Release();
    }
    // 释放 Shell 链接对象指针
    psl->Release();
    return true;
#else  // _WIN64
    return false;
#endif // _WIN64
}

std::string path_concat(const char *path1, ...)
{
    std::string return_path = path1;
    char *path;
    va_list ap;
    va_start(ap, path1);
    while ((path = (char *)va_arg(ap, char *)))
    {
        if (!return_path.empty())
        {
            if ((return_path.back() != '/')
#ifdef _WIN64
                && (return_path.back() != '\\')
#endif // _WIN64
            )
            {
                return_path.push_back(path_splitor);
            }
        }
        if (empty(path))
        {
            break;
        }
        if ((path[0] == '/') && (!return_path.empty()))
        {
            path++;
        }
#ifdef _WIN64
        if (path[0] == '\\')
        {
            path++;
        }
#endif // _WIN64
        return_path.append(path);
    }
    va_end(ap);

    if ((!return_path.empty()) && (return_path.back() == path_splitor))
    {
        return_path.pop_back();
    }
    return return_path;
}

std::string get_dirname(const char *pathname)
{
    std::string path = pathname;
    size_t pos = path.rfind('/');
#ifdef _WIN64
    size_t pos2 = path.rfind('\\');
    if ((pos2 != std::string::npos) && (pos2 != std::string::npos) && (pos2 > pos))
    {
        pos = pos2;
    }
#endif // _WIN64
    if (pos != std::string::npos)
    {
        return path.substr(0, pos);
    }
    return "";
}

void get_dirname_and_filename(const char *pathname, std::string &dirname, std::string &filename)
{
    std::string path = pathname;
    size_t pos = path.rfind('/');
#ifdef _WIN64
    size_t pos2 = path.rfind('\\');
    if ((pos2 != std::string::npos) && (pos2 != std::string::npos) && (pos2 > pos))
    {
        pos = pos2;
    }
#endif // _WIN64
    if (pos != std::string::npos)
    {
        dirname = path.substr(0, pos);
        filename = path.substr(pos + 1);
    }
    else
    {
        dirname = "";
        filename = path;
    }
}

std::string get_pathname_for_dump(const char *dirname, const char *filename, int max_loop)
{
    if (max_loop < 1)
    {
        max_loop = 10000;
    }
    std::string tmp_pathname = dirname;
    tmp_pathname.push_back(path_splitor);
    tmp_pathname.append(filename);

    int ret = zcc::file_exists(tmp_pathname);
    if (ret < 0)
    {
        return "";
    }
    if (ret == 0)
    {
        return tmp_pathname;
    }
    const char *suffix = "";
    std::string prefix = dirname;
    prefix.push_back(path_splitor);
    const char *p = strrchr(filename, '.');
    if (p)
    {
        suffix = p;
        if (p > filename)
        {
            prefix.append(filename, p - filename);
        }
    }
    else
    {
        prefix.append(filename);
    }

    do
    {
        size_t size = prefix.size();
        if (size == 0)
        {
            break;
        }
        if (prefix[size - 1] != ')')
        {
            break;
        }
        size--;
        if (size == 0)
        {
            break;
        }
        while (size > 0)
        {
            int ch = prefix[size - 1];
            if (isdigit(ch))
            {
                size--;
                continue;
            }
            break;
        }
        if (size == 0)
        {
            break;
        }
        if (prefix[size - 1] != '(')
        {
            break;
        }
        size--;
        prefix.resize(size);
        break;
    } while (0);

    std::string r;
    int i = 0;
    for (i = 1; i < max_loop; i++)
    {
        r = prefix;
        r.append("(").append(std::to_string(i)).append(")").append(suffix);
        ret = zcc::file_exists(r.c_str());
        if (ret < 0)
        {
            return "";
        }
        if (ret > 0)
        {
            continue;
        }
        return r;
    }
    return "";
}

std::string get_pathname_for_dump(const char *pathname, int max_loop)
{
    std::string dirname;
    std::string filename;
    get_dirname_and_filename(pathname, dirname, filename);
    return get_pathname_for_dump(dirname.c_str(), filename.c_str(), max_loop);
}

zcc_namespace_end;

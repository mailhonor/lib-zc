/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2023-05-30
 * ================================
 */

#include "zcc/zcc_stdlib.h"
#include "zcc/zcc_errno.h"
#include "zcc/zcc_win64.h"
#ifdef _WIN64
#include <processthreadsapi.h>
#include <tlhelp32.h>
#else // _WIN64
#ifdef __linux__
#include <sys/resource.h>
#endif // __linux__
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif // _WIN64

zcc_namespace_begin;

/**
 * @brief 终止当前进程，调用标准库的 exit 函数。
 * @param status 进程退出状态码，通常 0 表示正常退出，非零表示异常退出。
 */
void exit(int status)
{
    // 调用标准库的 exit 函数终止进程
    ::exit(status);
}

/**
 * @brief 获取当前进程的进程 ID。
 * @return 当前进程的进程 ID，在不同操作系统下有不同的实现方式。
 */
int get_process_id()
{
#ifdef _WIN64
    // 在 Windows 系统下，调用 GetCurrentProcessId 函数获取当前进程的 ID
    return ::GetCurrentProcessId();
#else  // _WIN64
    // 在非 Windows 系统下，调用 getpid 函数获取当前进程的 ID
    return ::getpid();
#endif // _WIN64
}

/**
 * @brief 获取当前进程的父进程 ID。
 * @return 当前进程的父进程 ID，如果获取失败则返回 -1。
 */
int get_parent_process_id()
{
#ifdef _WIN64
    int parent_pid = -1;
    HANDLE handle;
    PROCESSENTRY32 pe;
    // 获取当前进程的 ID
    DWORD current_pid = ::GetCurrentProcessId();

    // 初始化 PROCESSENTRY32 结构体的大小
    pe.dwSize = sizeof(PROCESSENTRY32);
    // 创建进程快照
    handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (handle == INVALID_HANDLE_VALUE)
    {
        // Handle creation failed
        return -1;
    }

    if (Process32First(handle, &pe))
    {
        do
        {
            // 找到当前进程，记录其父进程 ID
            if (pe.th32ProcessID == current_pid)
            {
                parent_pid = pe.th32ParentProcessID;
                break;
            }
        } while (Process32Next(handle, &pe));
    }

    // 关闭快照句柄
    CloseHandle(handle);
    return parent_pid;
#else  // _WIN64
    // 在非 Windows 系统下，调用 getppid 函数获取父进程的 ID
    return ::getppid();
#endif // _WIN64
}

/**
 * @brief 获取当前线程的线程 ID。
 * @return 当前线程的线程 ID，如果获取失败则返回 -1。
 */
int get_thread_id()
{
#ifdef _WIN64
    // 在 Windows 系统下，调用 GetCurrentThreadId 函数获取当前线程的 ID
    return ::GetCurrentThreadId();
#else // _WIN64
#if (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 30)
    // 在特定版本的 glibc 下，调用 gettid 函数获取线程 ID
    return ::gettid();
#else
#if defined(SYS_gettid)
    // 调用系统调用获取线程 ID
    return (int)syscall(SYS_gettid);
#elif defined(__NR_gettid) || defined(_ANDROID)
    // 调用系统调用获取线程 ID
    return (int)syscall(__NR_gettid);
#elif defined(__APPLE__)
    uint64_t tid;
    // 在苹果系统下，调用 pthread_threadid_np 函数获取线程 ID
    pthread_threadid_np(NULL, &tid);
    return (int)(static_cast<int64_t>(tid));
#else
    // 无法获取线程 ID，返回 -1
    return -1;
#endif
#endif
#endif // _WIN64
}

/**
 * @brief 获取当前进程的可执行文件的完整路径。
 * @return 可执行文件的完整路径，如果获取失败则返回空字符串。
 */
std::string get_cmd_pathname()
{
#ifdef _WIN64
    char szFileName[4096 + 1];
    DWORD size = 4096, ret;
    // 获取当前模块的文件路径
    if ((ret = GetModuleFileName(NULL, szFileName, size)))
    {
        // 将多字节字符串转换为 UTF-8 编码
        return MultiByteToUTF8(szFileName, ret);
    }
    return "";
#else  // _WIN64
    char path[4096 + 1];
    // 读取 /proc/self/exe 符号链接获取可执行文件路径
    int64_t len = readlink("/proc/self/exe", path, 4096);
    if (len != -1)
    {
        path[len] = '\0';
        return path;
    }
    else
    {
        return "";
    }
#endif // _WIN64
}

/**
 * @brief 获取当前进程的可执行文件的名称。
 * @return 可执行文件的名称，如果获取失败则返回完整路径。
 */
std::string get_cmd_name()
{
    // 获取可执行文件的完整路径
    std::string r = get_cmd_pathname();
#ifdef _WIN64
    // 在 Windows 系统下，查找最后一个反斜杠的位置
    size_t pos = r.find_last_of('\\');
#else  // _WIN64
    // 在非 Windows 系统下，查找最后一个正斜杠的位置
    size_t pos = r.find_last_of('/');
#endif // _WIN64
    if (pos != std::string::npos)
    {
        // 截取文件名部分
        return r.substr(pos + 1);
    }
    return r;
}

#ifdef __linux__
/**
 * @brief 快速设置资源限制。
 * @param cmd 资源限制类型，如 RLIMIT_CORE。
 * @param cur_val 要设置的资源限制值。
 * @return 设置成功返回 true，失败返回 false。
 */
bool quick_setrlimit(int cmd, unsigned long cur_val)
{
    struct rlimit rlim;
    // 获取当前的资源限制
    if (getrlimit(RLIMIT_CORE, &rlim) == 0)
    {
        if (rlim.rlim_cur != cur_val)
        {
            // 更新资源限制值
            rlim.rlim_cur = cur_val;
            if (setrlimit(cmd, &rlim))
            {
                // 设置失败，输出警告信息
                zcc_warning("setrlimit(%m)");
                return false;
            }
        }
    }
    else
    {
        // 获取资源限制失败，输出警告信息
        zcc_warning("setrlimit(%m)");
    }
    return true;
}

/**
 * @brief 设置核心转储文件的大小限制。
 * @param megabyte 核心转储文件的大小限制，单位为兆字节。如果为 -1 则表示无限制。
 * @return 设置成功返回 true，失败返回 false。
 */
bool set_core_file_size(int megabyte)
{
    unsigned long l = RLIM_INFINITY;
    if (megabyte >= 0)
    {
        // 将兆字节转换为字节
        l = 1UL * 1024 * 1024 * megabyte;
    }
    // 调用 quick_setrlimit 函数设置资源限制
    return quick_setrlimit(RLIMIT_CORE, l);
}

/**
 * @brief 设置进程的最大内存限制。
 * @param megabyte 最大内存限制，单位为兆字节。如果为 -1 则表示无限制。
 * @return 设置成功返回 true，失败返回 false。
 */
bool set_max_mem(int megabyte)
{
    int ret = 1;
    unsigned long l = RLIM_INFINITY;
    if (megabyte >= 0)
    {
        // 将兆字节转换为字节
        l = 1UL * 1024 * 1024 * megabyte;
    }
#ifdef RLIMIT_AS
    // 设置地址空间限制
    ret = ret && quick_setrlimit(RLIMIT_AS, l);
#else
#ifdef RLIMIT_RSS
    // 设置常驻集大小限制
    ret = ret && _quick_setrlimit(RLIMIT_RSS, l);
#endif
#endif

#ifdef RLIMIT_DATA
    // 设置数据段大小限制
    ret = ret && quick_setrlimit(RLIMIT_DATA, l);
#endif
    return ret;
}

/**
 * @brief 将当前进程加入指定的 cgroup。
 * @param name cgroup 的名称。
 * @return 加入成功返回 true，失败返回 false。
 */
bool set_cgroup_name(const char *name)
{
    if (empty(name))
    {
        return false;
    }
    bool r = false;

    // 获取当前进程的 ID
    long pid = (long)getpid();
    char pbuf[128];
    // 将进程 ID 转换为字符串
    std::sprintf(pbuf, "%ld", pid);

    char fn[1024 + 1];
    // 构造 cgroup.procs 文件的路径
    std::snprintf(fn, 1024, "/sys/fs/cgroup/memory/%s/cgroup.procs", name);

    // 打开 cgroup.procs 文件
    int fd = ::open(fn, O_WRONLY, 0);
    if (fd == -1)
    {
        // 打开文件失败，输出警告信息
        zcc_warning("open %s(%m)", fn);
        return false;
    }
    while (1)
    {
        // 将进程 ID 写入 cgroup.procs 文件
        if (write(fd, pbuf, strlen(pbuf)) != -1)
        {
            if (get_errno() == ZCC_EINTR)
            {
                // 写入被信号中断，继续尝试
                continue;
            }
            break;
        }
        r = true;
        break;
    }
    // 关闭文件描述符
    ::close(fd);

    return r;
}

/**
 * @brief 获取系统可用内存的大小。
 * @return 系统可用内存的大小，单位为字节。如果获取失败则返回 -1。
 */
int64_t get_MemAvailable()
{
    int v = -1;
    char linebuf[1024 + 1];
    // 打开 /proc/meminfo 文件
    FILE *fp = ::fopen("/proc/meminfo", "r");
    if (!fp)
    {
        return v;
    }
    while (fgets(linebuf, 1024, fp))
    {
        // 查找 MemAvailable 行
        if (std::strncmp(linebuf, "MemAvailable:", 13))
        {
            continue;
        }
        // 将可用内存大小转换为字节
        v = atol(linebuf + 13) * 1024;
        break;
    }
    // 关闭文件
    fclose(fp);
    return v;
}

/**
 * @brief 获取系统的 CPU 核心数。
 * @return 系统的 CPU 核心数，如果获取失败则返回 1。
 */
int get_cpu_core_count()
{
    int count = 0;
    char linebuf[1024 + 1];
    // 打开 /proc/cpuinfo 文件
    FILE *fp = ::fopen("/proc/cpuinfo", "r");
    if (!fp)
    {
        return 1;
    }
    while (fgets(linebuf, 1024, fp))
    {
        // 查找 processor 行
        if (strncmp(linebuf, "processor", 9))
        {
            continue;
        }
        // 每找到一行 processor，CPU 核心数加 1
        count++;
    }
    // 关闭文件
    fclose(fp);
    return count;
}
#endif // __linux__

zcc_namespace_end;

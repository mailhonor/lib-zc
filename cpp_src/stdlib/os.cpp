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

void exit(int status)
{
    ::exit(status);
}

int get_process_id()
{
#ifdef _WIN64
    return ::GetCurrentProcessId();
#else  // _WIN64
    return ::getpid();
#endif // _WIN64
}

int get_parent_process_id()
{
#ifdef _WIN64
    int parent_pid = -1;
    HANDLE handle;
    PROCESSENTRY32 pe;
    DWORD current_pid = ::GetCurrentProcessId();

    pe.dwSize = sizeof(PROCESSENTRY32);
    handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (Process32First(handle, &pe))
    {
        do
        {
            if (pe.th32ProcessID == current_pid)
            {
                parent_pid = pe.th32ParentProcessID;
                break;
            }
        } while (Process32Next(handle, &pe));
    }

    CloseHandle(handle);
    return parent_pid;
#else  // _WIN64
    return ::getppid();
#endif // _WIN64
}

int get_thread_id()
{
#ifdef _WIN64
    return ::GetCurrentThreadId();
#else // _WIN64
#if (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 30)
    return ::gettid();
#else
#if defined(SYS_gettid)
    return (int)syscall(SYS_gettid);
#elif defined(__NR_gettid) || defined(_ANDROID)
    return (int)syscall(__NR_gettid);
#elif defined(__APPLE__)
    uint64_t tid;
    pthread_threadid_np(NULL, &tid);
    return (int)(static_cast<int64_t>(tid));
#else
    return -1;
#endif
#endif
#endif // _WIN64
}

std::string get_cmd_pathname()
{
#ifdef _WIN64
    char szFileName[4096 + 1];
    DWORD size = 4096, ret;
    // HMODULE hModule = GetModuleHandle(NULL);
    if ((ret = GetModuleFileName(NULL, szFileName, size)))
    {
        return MultiByteToUTF8(szFileName, ret);
    }
    return "";
#else  // _WIN64
    char path[4096 + 1];
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

std::string get_cmd_name()
{
    std::string r = get_cmd_pathname();
#ifdef _WIN64
    size_t pos = r.find_last_of('\\');
#else  // _WIN64
    size_t pos = r.find_last_of('/');
#endif // _WIN64
    if (pos != std::string::npos)
    {
        return r.substr(pos + 1);
    }
    return r;
}

#ifdef __linux__
bool quick_setrlimit(int cmd, unsigned long cur_val)
{
    struct rlimit rlim;
    if (getrlimit(RLIMIT_CORE, &rlim) == 0)
    {
        if (rlim.rlim_cur != cur_val)
        {
            rlim.rlim_cur = cur_val;
            if (setrlimit(cmd, &rlim))
            {
                zcc_warning("setrlimit(%m)");
                return false;
            }
        }
    }
    else
    {
        zcc_warning("setrlimit(%m)");
    }
    return true;
}

bool set_core_file_size(int megabyte)
{
    unsigned long l = RLIM_INFINITY;
    if (megabyte >= 0)
    {
        l = 1UL * 1024 * 1024 * megabyte;
    }
    return quick_setrlimit(RLIMIT_CORE, l);
}

bool set_max_mem(int megabyte)
{
    int ret = 1;
    unsigned long l = RLIM_INFINITY;
    if (megabyte >= 0)
    {
        l = 1UL * 1024 * 1024 * megabyte;
    }
#ifdef RLIMIT_AS
    ret = ret && quick_setrlimit(RLIMIT_AS, l);
#else
#ifdef RLIMIT_RSS
    ret = ret && _quick_setrlimit(RLIMIT_RSS, l);
#endif
#endif

#ifdef RLIMIT_DATA
    ret = ret && quick_setrlimit(RLIMIT_DATA, l);
#endif
    return ret;
}

bool set_cgroup_name(const char *name)
{
    if (!empty(name))
    {
        return false;
    }
    int r = false;

    long pid = (long)getpid();
    char pbuf[128];
    std::sprintf(pbuf, "%ld", pid);

    char fn[1024 + 1];
    std::snprintf(fn, 1024, "/sys/fs/cgroup/memory/%s/cgroup.procs", name);

    int fd = ::open(fn, O_RDONLY, 0);
    if (fd == -1)
    {
        zcc_warning("open %s(%m)", fn);
        return false;
    }
    while (1)
    {
        if (write(fd, pbuf, strlen(pbuf)) != -1)
        {
            if (get_errno() == ZCC_EINTR)
            {
                continue;
            }
            break;
        }
        r = true;
        break;
    }
    ::close(fd);

    return r;
}

int64_t get_MemAvailable()
{
    int v = -1;
    char linebuf[1024 + 1];
    FILE *fp = ::fopen("/proc/meminfo", "r");
    if (!fp)
    {
        return v;
    }
    while (fgets(linebuf, 1024, fp))
    {
        if (std::strncmp(linebuf, "MemAvailable:", 13))
        {
            continue;
        }
        v = atol(linebuf + 13) * 1024;
        break;
    }
    fclose(fp);
    return v;
}

int get_cpu_core_count()
{
    int count = 0;
    char linebuf[1024 + 1];
    FILE *fp = ::fopen("/proc/cpuinfo", "r");
    if (!fp)
    {
        return 1;
    }
    while (fgets(linebuf, 1024, fp))
    {
        if (strncmp(linebuf, "processor", 9))
        {
            continue;
        }
        count++;
    }
    fclose(fp);
    return count;
}
#endif // __linux__

zcc_namespace_end;

/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2018-12-02
 * ================================
 */

#include "zcc/zcc_thread.h"
#include "zcc/zcc_win64.h"
#include <thread>

#ifdef _WIN64
#include <processthreadsapi.h>
#else // _WIN64
#include <pthread.h>
#endif // _WIN64

zcc_namespace_begin;

static void set_thread_name_by_handle(std::thread::native_handle_type h, const char *name)
{
#ifdef _MSC_VER
    auto n = Utf8ToWideChar(name);
    SetThreadDescription(h, n.c_str());
#elif defined(__APPLE__)
    pthread_setname_np(name);
#else // _WIN64
    pthread_setname_np(h, name);
#endif // _WIN64
}

void set_thread_name(std::thread &t, const char *name)
{
    return set_thread_name_by_handle(t.native_handle(), name);
}

void set_thread_name(const char *name)
{
#ifdef _MSC_VER
    return set_thread_name_by_handle(GetCurrentThread(), name);
#else  // _WIN64
    return set_thread_name_by_handle(pthread_self(), name);
#endif // _WIN64
}

static std::string get_thread_name_by_handle(std::thread::native_handle_type h)
{
    std::string r;
#ifdef _MSC_VER
    PWSTR data;
    HRESULT hr = GetThreadDescription(h, &data);
    if (SUCCEEDED(hr))
    {
        r = WideCharToUTF8(data);
        LocalFree(data);
    }
    return r;
#else  // _WIN64
    char name[16 + 1];
    if (pthread_getname_np(h, name, 16) == 0)
    {
        r = name;
    }
    return r;
#endif // _WIN64
}

std::string get_thread_name(std::thread &t)
{
    return get_thread_name_by_handle(t.native_handle());
}

std::string get_thread_name()
{
#ifdef _MSC_VER
    return get_thread_name_by_handle(GetCurrentThread());
#else  // _WIN64
    return get_thread_name_by_handle(pthread_self());
#endif // _WIN64
}

zcc_namespace_end;

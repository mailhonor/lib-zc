/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2026-02-22
 * ================================
 */

#include "zcc/zcc_stdlib.h"
#include "zcc/zcc_win64.h"

#ifdef _WIN64
#include <shlwapi.h>
#include <tlhelp32.h>
#pragma comment(lib, "shlwapi.lib")
#else
#include <dlfcn.h>
#include <link.h>
#include <unistd.h>
#endif

zcc_namespace_begin;

void *dlopen_auto(const std::string &lib_pathname_or_name, const std::string &symbol_to_check)
{
    void *libhandle = nullptr;
    libhandle = dlopen("");
    if (libhandle)
    {
        if (dlsym(libhandle, symbol_to_check))
        {
            return libhandle;
        }
        dlclose(libhandle);
        libhandle = nullptr;
    }
    //
    if (lib_pathname_or_name.empty())
    {
        return nullptr;
    }
    //
    libhandle = dlopen(lib_pathname_or_name);
    if (libhandle)
    {
        return libhandle;
    }
    //
    std::string name;
#ifdef _WIN32
    name = lib_pathname_or_name + ".dll";
#else
    name = "lib" + lib_pathname_or_name + ".so";
#endif
    libhandle = dlopen(name);
    if (libhandle)
    {
        return libhandle;
    }
    //
    return nullptr;
}

#ifdef _WIN64
static HMODULE g_mainModule = nullptr;

// 获取主模块句柄，使用缓存并考虑多线程安全
static HMODULE getMainModule()
{
    if (g_mainModule)
    {
        return g_mainModule;
    }
    global_low_level_mutex_lock();
    if (g_mainModule == nullptr)
    {
        g_mainModule = GetModuleHandleA(nullptr);
    }
    global_low_level_mutex_unlock();
    return g_mainModule;
}
#endif // _WIN64

// 加载动态库，支持全局符号表
void *dlopen(const std::string &lib_name)
{
    if (lib_name.empty())
    {
#ifdef _WIN32
        // 返回一个特殊标记表示全局符号表
        return reinterpret_cast<void *>(-1);
#else
        return ::dlopen(nullptr, RTLD_GLOBAL | RTLD_LAZY);
#endif
    }
#ifdef _WIN32
    wchar_t lib_name_w[Z_MAX_PATH + 1];
    if (Utf8ToWideChar(lib_name, lib_name_w, Z_MAX_PATH) < 1)
    {
        return nullptr;
    }
    return LoadLibraryW(lib_name_w);
#else
    return ::dlopen(lib_name.c_str(), RTLD_GLOBAL | RTLD_LAZY);
#endif
}

// 获取符号地址，支持全局符号表
void *dlsym(void *handle, const std::string &symbol_name)
{
    if (!handle)
    {
        return nullptr;
    }
#ifdef _WIN32
    if (handle == reinterpret_cast<void *>(-1))
    {
        // 查找全局符号表
        HMODULE mainModule = getMainModule();
        void *addr = GetProcAddress(mainModule, symbol_name.c_str());
        if (addr != nullptr)
            return addr;

        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
        if (snapshot == INVALID_HANDLE_VALUE)
            return nullptr;

        MODULEENTRY32 modEntry;
        modEntry.dwSize = sizeof(MODULEENTRY32);
        if (Module32First(snapshot, &modEntry))
        {
            do
            {
                if (modEntry.hModule == mainModule)
                    continue;
                addr = GetProcAddress(modEntry.hModule, symbol_name.c_str());
                if (addr != nullptr)
                {
                    CloseHandle(snapshot);
                    return addr;
                }
            } while (Module32Next(snapshot, &modEntry));
        }
        CloseHandle(snapshot);
        return nullptr;
    }
    else
    {
        return reinterpret_cast<void *>(GetProcAddress(static_cast<HMODULE>(handle), symbol_name.c_str()));
    }
#else
    return ::dlsym(handle, symbol_name.c_str());
#endif
}

// 卸载动态库，处理全局符号表特殊情况
bool dlclose(void *handle)
{
    if (!handle)
    {
        return true;
    }
#ifdef _WIN32
    if (handle != reinterpret_cast<void *>(-1))
    {
        return FreeLibrary(static_cast<HMODULE>(handle)) != 0;
    }
    return true;
#else
    return ::dlclose(handle) == 0;
#endif
}

zcc_namespace_end;

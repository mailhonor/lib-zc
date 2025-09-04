/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2017-02-18
 * ================================
 */

#pragma once

#ifndef ZCC_LIB_INCLUDE_STDLIB___
#define ZCC_LIB_INCLUDE_STDLIB___

// 检查是否为64位Windows系统
#ifdef _WIN64
// 检查是否已经定义了 _WIN32_WINNT 宏，如果定义了则取消定义
#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
// 重新定义 _WIN32_WINNT 为 0x0A00，对应Windows 10系统
#define _WIN32_WINNT 0x0A00
#endif // _WIN64

// _AMD64_ 宏定义
// 如果是64位Windows系统，定义 _AMD64_ 宏
#ifdef _WIN64
#define _AMD64_
#endif // _WIN64

// 开发模式相关设置
// 检查是否开启了ZCC开发模式
#ifdef ZCC_DEV_MODE___
// 检查是否使用Microsoft Visual C++编译器
#ifdef _MSC_VER
// 禁用特定的编译器警告，4267: 从 'size_t' 转换到 '类型'，可能丢失数据；4244: 从 '类型1' 转换到 '类型2'，可能丢失数据；4996: 函数或变量已被标记为弃用
#pragma warning(disable : 4267 4244 4996)
#endif // _MSC_VER
// 检查是否已经定义了 NOMINMAX 宏，如果未定义则定义它，用于避免Windows头文件中定义的 min 和 max 宏与标准库冲突
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#endif // ZCC_DEV_MODE___

// 定义 ZCC_DEPRECATED 宏，用于标记弃用的函数或变量
// 检查是否使用Microsoft Visual C++编译器
#if defined(_MSC_VER)
// 如果是Microsoft Visual C++编译器，使用 __declspec(deprecated) 标记函数或变量为弃用
#define ZCC_DEPRECATED __declspec(deprecated)
// 检查是否使用GNU C++编译器
#elif defined(__GNUC__)
// 如果是GNU C++编译器，使用 __attribute__((deprecated)) 标记函数或变量为弃用
#define ZCC_DEPRECATED __attribute__((deprecated))
// 其他编译器情况
#else
// 对于其他编译器，定义 ZCC_DEPRECATED 为空
#define ZCC_DEPRECATED
#endif

//
#ifndef zcc_namespace_c_begin
#define zcc_namespace_c_begin \
    extern "C"                \
    {
#define zcc_namespace_c_end }

#define zcc_namespace_begin \
    namespace zcc           \
    {
#define zcc_namespace_end }

#define zcc_general_namespace_begin(ns) \
    namespace ns                        \
    {
#define zcc_general_namespace_end(ns) }

#define zcc_anonymous_namespace_begin() \
    namespace                           \
    {
#define zcc_anonymous_namespace_end() }

#endif

#ifndef ZCC__FILE__
#define ZCC__FILE__ __FILE__
#endif

/**
 * @brief 不区分大小写地比较两个字符串的前 n 个字符是否相等。
 * 首先比较两个字符串的第一个字符（转换为大写后），若相等则调用 zcc::strncasecmp 进行前 n 个字符的不区分大小写比较。
 * @param a 第一个字符串。
 * @param b 第二个字符串。
 * @param n 要比较的字符数量。
 * @return 如果前 n 个字符不区分大小写相等则返回 true，否则返回 false。
 */
#define ZCC_STR_N_CASE_EQ(a, b, n) ((zcc::toupper((int)a[0]) == zcc::toupper((int)b[0])) && (!zcc::strncasecmp(a, b, n)))

/**
 * @brief 不区分大小写地比较两个字符串是否相等。
 * 首先比较两个字符串的第一个字符（转换为大写后），若相等则调用 zcc::strcasecmp 进行完整字符串的不区分大小写比较。
 * @param a 第一个字符串。
 * @param b 第二个字符串。
 * @return 如果两个字符串不区分大小写相等则返回 true，否则返回 false。
 */
#define ZCC_STR_CASE_EQ(a, b) ((zcc::toupper(a[0]) == zcc::toupper(b[0])) && (!zcc::strcasecmp(a, b)))

/**
 * @brief 区分大小写地比较两个字符串的前 n 个字符是否相等。
 * 首先比较两个字符串的第一个字符，若相等则调用 std::strncmp 进行前 n 个字符的区分大小写比较。
 * @param a 第一个字符串。
 * @param b 第二个字符串。
 * @param n 要比较的字符数量。
 * @return 如果前 n 个字符区分大小写相等则返回 true，否则返回 false。
 */
#define ZCC_STR_N_EQ(a, b, n) ((a[0] == b[0]) && (!std::strncmp(a, b, n)))

/**
 * @brief 区分大小写地比较两个字符串是否相等。
 * 首先比较两个字符串的第一个字符，若相等则调用 std::strcmp 进行完整字符串的区分大小写比较。
 * @param a 第一个字符串。
 * @param b 第二个字符串。
 * @return 如果两个字符串区分大小写相等则返回 true，否则返回 false。
 */
#define ZCC_STR_EQ(a, b) ((a[0] == b[0]) && (!std::strcmp(a, b)))

/**
 * @brief 定义一个特殊的 void 指针常量，其值为 -1 转换后的指针。
 * 可用于表示某种特殊状态或标记。
 */
#define ZCC_VOID_PTR_ONE ((void *)(int64_t)-1)

/**
 * @brief 将一个整数转换为 void 指针。
 * @param n 要转换的整数。
 * @return 转换后的 void 指针。
 */
#define ZCC_NUMBER_TO_PTR(n) ((void *)(int64_t)(n))

/**
 * @brief 将一个 void 指针转换为整数。
 * @param p 要转换的 void 指针。
 * @return 转换后的整数。
 */
#define ZCC_PTR_TO_NUMBER(p) ((int64_t)(void *)(p))

#ifdef __cplusplus

zcc_namespace_c_begin;
struct stat;
struct _stat64i32;
struct tm;
zcc_namespace_c_end;

#ifndef _WIN64
#include <strings.h>
#endif // _WIN64
#include <cstring>
#include <cstdarg>
#include <cstddef>
#include <cstdlib>
#include <string>
#include <vector>
#include <list>
#include <map>
#ifdef _WIN64
#include <WinSock2.h>
#include <windows.h>
#endif // _WIN64

#pragma pack(push, 4)
zcc_namespace_begin;

extern const char *progname;

inline bool empty(const void *str)
{
    return ((!str) || (!(*((const char *)str))));
}
inline bool empty(const std::string &str)
{
    return str.empty();
}

void exit(int status);

struct size_data
{
    int64_t size;
    char *data;
};

class json;
class stream;
class iostream;
class redis;
class cdb_reader;
class cdb_builder;
class cdb_walker;
class msearch_reader;
class msearch_builder;
class msearch_walker;
class aio;
class aio_timer;
class aio_base;
class httpd;
class httpd_uploaded_file;

/* auto_delete ###################################################### */
template <class T>
class auto_delete
{
public:
    inline auto_delete(T *obj) { obj_ = obj; }
    inline ~auto_delete()
    {
        if (obj_)
        {
            delete obj_;
        }
    }
    T *obj_;
};

/* gloabal init ##################################################### */
#define zcc_global_init(doSth)                                   \
    namespace                                                    \
    {                                                            \
        static struct zcc_global_init_class_##__LINE__           \
        {                                                        \
            inline zcc_global_init_class_##__LINE__() { doSth; } \
        } zcc_global_init_class_instance_##__LINE__;             \
    }

#define zcc_global_fini(doSth)                                    \
    namespace                                                     \
    {                                                             \
        static struct zcc_global_fini_class_##__LINE__            \
        {                                                         \
            inline ~zcc_global_fini_class_##__LINE__() { doSth; } \
        } zcc_global_fini_class_instance_##__LINE__;              \
    }

/* log ############################################################## */
zcc_general_namespace_begin(logger);
/**
 * @brief 日志级别枚举，定义不同的日志输出级别。
 */
enum level
{
    verbose = 1,    // 详细级日志，输出最详细的信息，通常用于调试阶段。
    debug,          // 调试级日志，用于输出调试信息。
    info,           // 信息级日志，输出程序运行中的重要信息。
    warning,        // 警告级日志，输出可能存在问题但不影响程序正常运行的信息。
    error,          // 错误级日志，输出程序运行中出现的错误信息。
    fatal,          // 致命级日志，输出导致程序无法继续运行的严重错误信息。
    error_and_exit, // 错误并退出级日志，输出错误信息后程序将退出。
};

/**
 * @brief 标志位，指示是否捕获致命错误。
 */
extern bool var_fatal_catch;
/**
 * @brief 标志位，指示是否启用调试级日志输出。
 */
extern bool var_debug_enable;
/**
 * @brief 标志位，指示是否启用详细级日志输出。
 */
extern bool var_verbose_enable;
/**
 * @brief 标志位，指示是否禁用日志输出。
 */
extern bool var_output_disable;

/**
 * @brief 可变参数形式的日志输出函数。
 *
 * @param source_fn 日志输出所在的源文件名。
 * @param line_number 日志输出所在的代码行号。
 * @param ll 日志级别。
 * @param fmt 格式化字符串，类似于 printf 的格式。
 * @param ap 可变参数列表。
 */
void vlog_output(const char *source_fn, uint64_t line_number, level ll, const char *fmt, va_list ap);

/**
 * @brief 普通形式的日志输出函数，支持可变参数。
 *
 * @param source_fn 日志输出所在的源文件名。
 * @param line_number 日志输出所在的代码行号。
 * @param ll 日志级别。
 * @param fmt 格式化字符串，类似于 printf 的格式。
 * @param ... 可变参数。
 */
void log_output(const char *source_fn, uint64_t line_number, level ll, const char *fmt, ...);

/**
 * @brief 日志输出处理函数的类型定义。
 *
 * @param source_fn 日志输出所在的源文件名。
 * @param line_number 日志输出所在的代码行号。
 * @param ll 日志级别。
 * @param fmt 格式化字符串，类似于 printf 的格式。
 * @param ap 可变参数列表。
 */
typedef void (*output_handler_type)(const char *source_fn, uint64_t line_number, level ll, const char *fmt, va_list ap);

/**
 * @brief 外部定义的日志输出处理函数指针。
 */
extern output_handler_type var_output_handler;

// syslog
/**
 * @brief 根据配置使用 syslog 进行日志输出。
 *
 * @param attr 配置属性，格式为 "facility[,identity]"。
 */
void use_syslog_by_config(const char *attr /* facility[,identity */);

/**
 * @brief 使用 syslog 进行日志输出。
 *
 * @param identity 日志标识，用于区分不同程序的日志。
 * @param facility syslog 的设备类型。
 */
void use_syslog(const char *identity, int facility);

/**
 * @brief 使用 syslog 进行日志输出，重载函数，接受 std::string 类型的标识。
 *
 * @param identity 日志标识，用于区分不同程序的日志。
 * @param facility syslog 的设备类型。
 */
inline void use_syslog(const std::string &identity, int facility)
{
    use_syslog(identity.c_str(), facility);
}

/**
 * @brief 使用 syslog 进行日志输出，接受字符串类型的设备类型。
 *
 * @param identity 日志标识，用于区分不同程序的日志。
 * @param facility syslog 的设备类型字符串。
 */
void use_syslog(const char *identity, const char *facility);

/**
 * @brief 使用 syslog 进行日志输出，重载函数，接受 std::string 类型的标识和设备类型。
 *
 * @param identity 日志标识，用于区分不同程序的日志。
 * @param facility syslog 的设备类型字符串。
 */
inline void use_syslog(const std::string &identity, const std::string &facility)
{
    use_syslog(identity.c_str(), facility.c_str());
}

/**
 * @brief 根据设备类型字符串获取对应的 syslog 设备类型值。
 *
 * @param facility 设备类型字符串。
 * @return int syslog 设备类型值。
 */
int get_facility(const char *facility);

/**
 * @brief 根据设备类型字符串获取对应的 syslog 设备类型值，重载函数，接受 std::string 类型的设备类型。
 *
 * @param facility 设备类型字符串。
 * @return int syslog 设备类型值。
 */
inline int get_facility(const std::string &facility)
{
    return get_facility(facility.c_str());
}

/**
 * @brief 详细级日志输出宏，如果启用详细级日志，则输出日志信息。
 */
#define zcc_verbose(...) (zcc::logger::var_verbose_enable ? zcc::logger::log_output(ZCC__FILE__, __LINE__, zcc::logger::verbose, __VA_ARGS__) : (void)0);
/**
 * @brief 调试级日志输出宏，如果启用调试级日志，则输出日志信息。
 */
#define zcc_debug(...) (zcc::logger::var_debug_enable ? zcc::logger::log_output(ZCC__FILE__, __LINE__, zcc::logger::debug, __VA_ARGS__) : (void)0);
/**
 * @brief 信息级日志输出宏，输出信息级日志信息。
 */
#define zcc_info(...) zcc::logger::log_output(ZCC__FILE__, __LINE__, zcc::logger::info, __VA_ARGS__)
/**
 * @brief 警告级日志输出宏，输出警告级日志信息。
 */
#define zcc_warning(...) zcc::logger::log_output(ZCC__FILE__, __LINE__, zcc::logger::warning, __VA_ARGS__)
/**
 * @brief 错误级日志输出宏，输出错误级日志信息。
 */
#define zcc_error(...) zcc::logger::log_output(ZCC__FILE__, __LINE__, zcc::logger::error, __VA_ARGS__)
/**
 * @brief 致命级日志输出宏，输出致命级日志信息。
 */
#define zcc_fatal(...) zcc::logger::log_output(ZCC__FILE__, __LINE__, zcc::logger::fatal, __VA_ARGS__)
/**
 * @brief 错误并退出级日志输出宏，输出错误并退出级日志信息。
 */
#define zcc_error_and_exit(...) zcc::logger::log_output(ZCC__FILE__, __LINE__, zcc::logger::error_and_exit, __VA_ARGS__)
/**
 * @brief 程序退出宏，调用 zcc::exit 函数退出程序。
 */
#define zcc_exit(status) zcc::exit(status)

/**
 * @brief 日志类，用于流式输出日志信息。
 */
class LOG
{
public:
    /**
     * @brief 构造函数，初始化日志对象。
     *
     * @param ll 日志级别。
     * @param sourcePathname 日志输出所在的源文件名。
     * @param lineNumber 日志输出所在的代码行号。
     */
    LOG(level ll, const char *sourcePathname, uint64_t lineNumber);
    /**
     * @brief 析构函数，在对象销毁时输出日志信息。
     */
    ~LOG();

    /**
     * @brief 向日志缓冲区追加整数类型的信息。
     *
     * @param v 要追加的整数。
     * @return LOG& 返回当前对象的引用，以便链式调用。
     */
    inline LOG &append(int v)
    {
        buf_.append(std::to_string(v));
        return *this;
    };
    /**
     * @brief 重载 << 运算符，方便以流式方式追加整数类型的信息。
     *
     * @param v 要追加的整数。
     * @return LOG& 返回当前对象的引用，以便链式调用。
     */
    inline LOG &operator<<(int v) { return append(v); }

    /**
     * @brief 向日志缓冲区追加无符号整数类型的信息。
     *
     * @param v 要追加的无符号整数。
     * @return LOG& 返回当前对象的引用，以便链式调用。
     */
    inline LOG &append(unsigned int v)
    {
        buf_.append(std::to_string(v));
        return *this;
    };
    /**
     * @brief 重载 << 运算符，方便以流式方式追加无符号整数类型的信息。
     *
     * @param v 要追加的无符号整数。
     * @return LOG& 返回当前对象的引用，以便链式调用。
     */
    inline LOG &operator<<(unsigned int v) { return append(v); }

    /**
     * @brief 向日志缓冲区追加双精度浮点数类型的信息。
     *
     * @param v 要追加的双精度浮点数。
     * @return LOG& 返回当前对象的引用，以便链式调用。
     */
    inline LOG &append(double v)
    {
        buf_.append(std::to_string(v));
        return *this;
    };
    /**
     * @brief 重载 << 运算符，方便以流式方式追加双精度浮点数类型的信息。
     *
     * @param v 要追加的双精度浮点数。
     * @return LOG& 返回当前对象的引用，以便链式调用。
     */
    inline LOG &operator<<(double v) { return append(v); }

    /**
     * @brief 向日志缓冲区追加布尔类型的信息。
     *
     * @param v 要追加的布尔值。
     * @return LOG& 返回当前对象的引用，以便链式调用。
     */
    inline LOG &append(bool v)
    {
        buf_.append(v ? "true" : "false");
        return *this;
    };
    /**
     * @brief 重载 << 运算符，方便以流式方式追加布尔类型的信息。
     *
     * @param v 要追加的布尔值。
     * @return LOG& 返回当前对象的引用，以便链式调用。
     */
    inline LOG &operator<<(bool v) { return append(v); }

    /**
     * @brief 向日志缓冲区追加字符类型的信息。
     *
     * @param v 要追加的字符。
     * @return LOG& 返回当前对象的引用，以便链式调用。
     */
    inline LOG &append(char v)
    {
        buf_.push_back(v);
        return *this;
    };
    /**
     * @brief 重载 << 运算符，方便以流式方式追加字符类型的信息。
     *
     * @param v 要追加的字符。
     * @return LOG& 返回当前对象的引用，以便链式调用。
     */
    inline LOG &operator<<(char v) { return append(v); }

    /**
     * @brief 向日志缓冲区追加无符号字符类型的信息。
     *
     * @param v 要追加的无符号字符。
     * @return LOG& 返回当前对象的引用，以便链式调用。
     */
    inline LOG &append(unsigned char v)
    {
        buf_.push_back(v);
        return *this;
    };
    /**
     * @brief 重载 << 运算符，方便以流式方式追加无符号字符类型的信息。
     *
     * @param v 要追加的无符号字符。
     * @return LOG& 返回当前对象的引用，以便链式调用。
     */
    inline LOG &operator<<(unsigned char v) { return append(v); }

    /**
     * @brief 向日志缓冲区追加字符串类型的信息。
     *
     * @param v 要追加的字符串。
     * @return LOG& 返回当前对象的引用，以便链式调用。
     */
    inline LOG &append(const char *v)
    {
        buf_.append(v);
        return *this;
    };
    /**
     * @brief 重载 << 运算符，方便以流式方式追加字符串类型的信息。
     *
     * @param v 要追加的字符串。
     * @return LOG& 返回当前对象的引用，以便链式调用。
     */
    inline LOG &operator<<(const char *v) { return append(v); }

    /**
     * @brief 向日志缓冲区追加无符号字符串类型的信息。
     *
     * @param v 要追加的无符号字符串。
     * @return LOG& 返回当前对象的引用，以便链式调用。
     */
    inline LOG &append(const unsigned char *v)
    {
        buf_.append((const char *)v);
        return *this;
    };
    /**
     * @brief 重载 << 运算符，方便以流式方式追加无符号字符串类型的信息。
     *
     * @param v 要追加的无符号字符串。
     * @return LOG& 返回当前对象的引用，以便链式调用。
     */
    inline LOG &operator<<(const unsigned char *v) { return append(v); }

    /**
     * @brief 向日志缓冲区追加指定长度的字符串类型的信息。
     *
     * @param v 要追加的字符串。
     * @param len 要追加的字符串长度。
     * @return LOG& 返回当前对象的引用，以便链式调用。
     */
    inline LOG &append(const char *v, int len)
    {
        buf_.append(v, len);
        return *this;
    };

    /**
     * @brief 向日志缓冲区追加指定长度的无符号字符串类型的信息。
     *
     * @param v 要追加的无符号字符串。
     * @param len 要追加的无符号字符串长度。
     * @return LOG& 返回当前对象的引用，以便链式调用。
     */
    inline LOG &append(const unsigned char *v, int len)
    {
        buf_.append((const char *)v, len);
        return *this;
    };

    /**
     * @brief 向日志缓冲区追加 std::string 类型的信息。
     *
     * @param v 要追加的 std::string 对象。
     * @return LOG& 返回当前对象的引用，以便链式调用。
     */
    inline LOG &append(const std::string &v)
    {
        buf_.append(v);
        return *this;
    };
    /**
     * @brief 重载 << 运算符，方便以流式方式追加 std::string 类型的信息。
     *
     * @param v 要追加的 std::string 对象。
     * @return LOG& 返回当前对象的引用，以便链式调用。
     */
    inline LOG &operator<<(const std::string &v) { return append(v); }

    /**
     * @brief 向日志缓冲区追加 std::string 指针类型的信息。
     *
     * @param v 要追加的 std::string 指针。
     * @return LOG& 返回当前对象的引用，以便链式调用。
     */
    inline LOG &append(const std::string *v)
    {
        if (v)
        {
            buf_.append(*v);
        }
        return *this;
    };
    /**
     * @brief 重载 << 运算符，方便以流式方式追加 std::string 指针类型的信息。
     *
     * @param v 要追加的 std::string 指针。
     * @return LOG& 返回当前对象的引用，以便链式调用。
     */
    inline LOG &operator<<(const std::string *v) { return append(v); }

public:
    const char *sourcePathname_; ///< 日志输出所在的源文件名。
    uint64_t lineNumber_;        ///< 日志输出所在的代码行号。
    logger::level level_;        ///< 日志级别。
    std::string buf_;            ///< 日志缓冲区，用于存储要输出的日志信息。
};

/**
 * @brief 详细级日志流式输出宏，如果启用详细级日志，则返回 LOG 对象以便流式输出。
 */
#define zccVerbose()                     \
    if (zcc::logger::var_verbose_enable) \
    zcc::logger::LOG(zcc::logger::verbose, ZCC__FILE__, __LINE__)
/**
 * @brief 调试级日志流式输出宏，如果启用详细级或调试级日志，则返回 LOG 对象以便流式输出。
 */
#define zccDebug()                                                        \
    if (zcc::logger::var_verbose_enable || zcc::logger::var_debug_enable) \
    zcc::logger::LOG(zcc::logger::debug, ZCC__FILE__, __LINE__)
/**
 * @brief 信息级日志流式输出宏，返回 LOG 对象以便流式输出信息级日志。
 */
#define zccInfo() zcc::logger::LOG(zcc::logger::info, ZCC__FILE__, __LINE__)
/**
 * @brief 警告级日志流式输出宏，返回 LOG 对象以便流式输出警告级日志。
 */
#define zccWarning() zcc::logger::LOG(zcc::logger::warning, ZCC__FILE__, __LINE__)
/**
 * @brief 错误级日志流式输出宏，返回 LOG 对象以便流式输出错误级日志。
 */
#define zccError() zcc::logger::LOG(zcc::logger::error, ZCC__FILE__, __LINE__)
/**
 * @brief 致命级日志流式输出宏，返回 LOG 对象以便流式输出致命级日志。
 */
#define zccFatal() zcc::logger::LOG(zcc::logger::fatal, ZCC__FILE__, __LINE__)

zcc_general_namespace_end(logger);

/* malloc ########################################################### */
void *malloc(int64_t len);
void *calloc(int64_t nmemb, int64_t size);
void *realloc(const void *ptr, int64_t len);
void free(const void *ptr);
char *strdup(const char *ptr);
char *strndup(const char *ptr, int64_t n);
void *memdup(const void *ptr, int64_t n);
void *memdupnull(const void *ptr, int64_t n);
#define zcc_free(ptr)   \
    {                   \
        zcc::free(ptr); \
        ptr = nullptr;  \
    }

struct greedy_mpool_t;
class greedy_mpool
{
public:
    greedy_mpool();
    greedy_mpool(int single_buf_size, int once_malloc_max_size);
    ~greedy_mpool();
    void reinit(int single_buf_size, int once_malloc_max_size);
    void reset();
    void *malloc(int64_t len);
    void *calloc(int64_t nmemb, int64_t size);
    char *strdup(const char *ptr);
    char *strndup(const char *ptr, int64_t n);
    void *memdup(const void *ptr, int64_t n);
    void *memdupnull(const void *ptr, int64_t n);

protected:
    greedy_mpool_t *engine_{nullptr};
};

/* string ########################################################### */
/**
 * @brief 定义字典类型，使用 std::map 存储字符串键值对。
 */
typedef std::map<std::string, std::string> dict;

/**
 * @brief 外部声明一个字符数组，用于表示空白缓冲区。
 */
extern char var_blank_buffer[];

/**
 * @brief 外部声明一个常量 std::string 对象，用于表示空白字符串。
 */
extern const std::string var_blank_string;

/**
 * @brief 外部声明一个常量 std::list 对象，用于表示空白列表。
 */
extern const std::list<std::string> var_blank_list;

/**
 * @brief 外部声明一个常量 std::vector 对象，用于表示空白向量。
 */
extern const std::vector<std::string> var_blank_vector;

/**
 * @brief 外部声明一个常量字典对象，用于表示空白字典。
 */
extern const dict var_blank_map;

/**
 * @brief 外部声明一个无符号常量字符数组，用于存储十六进制字符对应的整数值。
 */
extern unsigned const char var_char_xdigitval_vector[];

/**
 * @brief 判断一个字符是否可被修剪。
 * 可被修剪的字符包括控制字符和空格。
 *
 * @param ch 要判断的字符。
 * @return bool 如果字符可被修剪则返回 true，否则返回 false。
 */
inline bool is_trimable(int ch) { return std::iscntrl(ch) || (ch == ' '); }

/**
 * @brief 判断一个字符是否可被修剪，功能与 is_trimable 相同。
 * 可被修剪的字符包括控制字符和空格。
 *
 * @param ch 要判断的字符。
 * @return bool 如果字符可被修剪则返回 true，否则返回 false。
 */
inline bool istrim(int ch) { return std::iscntrl(ch) || (ch == ' '); }

/**
 * @brief 将十六进制字符转换为对应的整数值。
 * 使用预定义的数组 var_char_xdigitval_vector 进行转换。
 *
 * @param c 要转换的十六进制字符。
 * @return int 转换后的整数值。
 */
inline int hex_char_to_int(int c) { return var_char_xdigitval_vector[(unsigned char)(c)]; }

/**
 * @brief 判断一个字符是否为字母或数字。
 * 调用标准库函数 std::isalnum 进行判断。
 *
 * @param c 要判断的字符。
 * @return int 如果是字母或数字则返回非零值，否则返回零。
 */
inline int isalnum(int c) { return std::isalnum(c); }

/**
 * @brief 判断一个字符是否为字母。
 * 调用标准库函数 std::isalpha 进行判断。
 *
 * @param c 要判断的字符。
 * @return int 如果是字母则返回非零值，否则返回零。
 */
inline int isalpha(int c) { return std::isalpha(c); }

/**
 * @brief 判断一个字符是否为控制字符。
 * 调用标准库函数 std::iscntrl 进行判断。
 *
 * @param c 要判断的字符。
 * @return int 如果是控制字符则返回非零值，否则返回零。
 */
inline int iscntrl(int c) { return std::iscntrl(c); }

/**
 * @brief 判断一个字符是否为数字。
 * 调用标准库函数 std::isdigit 进行判断。
 *
 * @param c 要判断的字符。
 * @return int 如果是数字则返回非零值，否则返回零。
 */
inline int isdigit(int c) { return std::isdigit(c); }

/**
 * @brief 判断一个字符是否为可打印的图形字符。
 * 调用标准库函数 std::isgraph 进行判断。
 *
 * @param c 要判断的字符。
 * @return int 如果是可打印的图形字符则返回非零值，否则返回零。
 */
inline int isgraph(int c) { return std::isgraph(c); }

/**
 * @brief 判断一个字符是否为小写字母。
 * 调用标准库函数 std::islower 进行判断。
 *
 * @param c 要判断的字符。
 * @return int 如果是小写字母则返回非零值，否则返回零。
 */
inline int islower(int c) { return std::islower(c); }

/**
 * @brief 判断一个字符是否为可打印字符。
 * 调用标准库函数 std::isprint 进行判断。
 *
 * @param c 要判断的字符。
 * @return int 如果是可打印字符则返回非零值，否则返回零。
 */
inline int isprint(int c) { return std::isprint(c); }

/**
 * @brief 判断一个字符是否为标点符号。
 * 调用标准库函数 std::ispunct 进行判断。
 *
 * @param c 要判断的字符。
 * @return int 如果是标点符号则返回非零值，否则返回零。
 */
inline int ispunct(int c) { return std::ispunct(c); }

/**
 * @brief 判断一个字符是否为空白字符。
 * 调用标准库函数 std::isspace 进行判断。
 *
 * @param c 要判断的字符。
 * @return int 如果是空白字符则返回非零值，否则返回零。
 */
inline int isspace(int c) { return std::isspace(c); }

/**
 * @brief 判断一个字符是否为大写字母。
 * 调用标准库函数 std::isupper 进行判断。
 *
 * @param c 要判断的字符。
 * @return int 如果是大写字母则返回非零值，否则返回零。
 */
inline int isupper(int c) { return std::isupper(c); }

/**
 * @brief 判断一个字符是否为十六进制数字。
 * 调用标准库函数 std::isxdigit 进行判断。
 *
 * @param c 要判断的字符。
 * @return int 如果是十六进制数字则返回非零值，否则返回零。
 */
inline int isxdigit(int c) { return std::isxdigit(c); }

/**
 * @brief 判断一个字符是否为空白分隔符（空格或制表符）。
 * 调用标准库函数 std::isblank 进行判断。
 *
 * @param c 要判断的字符。
 * @return int 如果是空白分隔符则返回非零值，否则返回零。
 */
inline int isblank(int c) { return std::isblank(c); }

// trim
/**
 * @brief 去除字符串左侧的空白字符。
 *
 * @param str 待处理的字符串指针。
 * @return char* 处理后的字符串指针。
 */
char *trim_left(char *str);
/**
 * @brief 去除字符串右侧的空白字符。
 *
 * @param str 待处理的字符串指针。
 * @return char* 处理后的字符串指针。
 */
char *trim_right(char *str);
/**
 * @brief 去除字符串两侧的空白字符。
 *
 * @param str 待处理的字符串指针。
 * @return char* 处理后的字符串指针。
 */
char *trim(char *str);
/**
 * @brief 去除 std::string 右侧指定的分隔字符。
 *
 * @param str 待处理的 std::string 对象引用。
 * @param delims 可选参数，指定要去除的分隔字符，默认为 0。
 * @return std::string& 处理后的 std::string 对象引用。
 */
std::string &trim_right(std::string &str, const char *delims = 0);
/**
 * @brief 去除 std::string 末尾的换行符（\r 和 \n）。
 *
 * @param str 待处理的 std::string 对象引用。
 * @return std::string& 处理后的 std::string 对象引用。
 */
std::string &trim_line_end_rn(std::string &str);

std::string trim(const std::string &str);

// skip
/**
 * @brief 跳过字符串左侧指定的忽略字符。
 *
 * @param str 待处理的字符串指针。
 * @param ignores 要忽略的字符集合。
 * @return char* 跳过忽略字符后的字符串指针。
 */
char *skip_left(const char *str, const char *ignores);
/**
 * @brief 从字符串右侧开始跳过指定的忽略字符，返回剩余字符串的长度。
 *
 * @param str 待处理的字符串指针。
 * @param len 字符串的长度。
 * @param ignores 要忽略的字符集合。
 * @return int 跳过忽略字符后剩余字符串的长度。
 */
int skip_right(const char *str, int len, const char *ignores);
/**
 * @brief 从字符串两侧跳过指定的忽略字符，返回剩余字符串的长度，并更新起始指针。
 *
 * @param line 待处理的字符串指针。
 * @param len 字符串的长度。
 * @param ignores_left 左侧要忽略的字符集合。
 * @param ignores_right 右侧要忽略的字符集合。
 * @param start 用于存储跳过忽略字符后字符串的起始指针。
 * @return int 跳过忽略字符后剩余字符串的长度。
 */
int skip(const char *line, int len, const char *ignores_left, const char *ignores_right, char **start);

// s 是 "0", "n", "N", "no", "NO", "false", "FALSE" 返回 false
// s 是 "1", "y", "Y", "yes", "YES", "true", "TRUE" 返回 true
// 否则 返回 def
/**
 * @brief 将字符串转换为布尔值。
 *
 * @param s 待转换的字符串指针。
 * @param def 转换失败时的默认值，默认为 false。
 * @return bool 转换后的布尔值。
 */

inline int64_t atoi(const char *s) { return std::atoi(s); }
inline int64_t atoi(const std::string &s) { return std::atoi(s.c_str()); }
#ifdef _WIN64
inline int64_t atol(const char *s) { return std::atoll(s); }
inline int64_t atol(const std::string &s) { return std::atoll(s.c_str()); }
#else
inline int64_t atol(const char *s) { return std::atol(s); }
inline int64_t atol(const std::string &s) { return std::atol(s.c_str()); }
#endif

bool str_to_bool(const char *s, bool def = false);
/**
 * @brief 将 std::string 转换为布尔值，调用 str_to_bool(const char*, bool) 实现。
 *
 * @param s 待转换的 std::string 对象。
 * @param def 转换失败时的默认值，默认为 false。
 * @return bool 转换后的布尔值。
 */
inline bool str_to_bool(const std::string &s, bool def = false)
{
    return str_to_bool(s.c_str(), def);
}

// 转换字符串为秒, 支持 h(小时), m(分), s(秒), d(天), w(周)
// 如 "1026S" => 1026, "8h" => 8 * 3600, "" => def
/**
 * @brief 将字符串转换为秒数。
 *
 * 支持的单位有 h(小时), m(分), s(秒), d(天), w(周)。
 *
 * @param s 待转换的字符串指针。
 * @param def 转换失败时的默认值。
 * @return int64_t 转换后的秒数。
 */
int64_t str_to_second(const char *s, int64_t def = 0);
inline int64_t str_to_second(const std::string &s, int64_t def = 0)
{
    return str_to_second(s.c_str(), def);
}

// 转换字符串为大小, 支持 g(G), m(兆), k(千), b
// 如 "9M" => 9 * 1024 * 1024
/**
 * @brief 将字符串转换为字节大小。
 *
 * 支持的单位有 g(G), m(兆), k(千), b。
 *
 * @param s 待转换的字符串指针。
 * @param def 转换失败时的默认值。
 * @return int64_t 转换后的字节大小。
 */
int64_t str_to_size(const char *s, int64_t def = 0);
inline int64_t str_to_size(const std::string &s, int64_t def = 0)
{
    return str_to_size(s.c_str(), def);
}

//
/**
 * @brief 将字符转换为小写，调用标准库函数 std::tolower 实现。
 *
 * @param ch 待转换的字符。
 * @return int 转换后的小写字符。
 */
inline int tolower(int ch)
{
    return std::tolower(ch);
}
/**
 * @brief 将字符转换为大写，调用标准库函数 std::toupper 实现。
 *
 * @param ch 待转换的字符。
 * @return int 转换后的大写字符。
 */
inline int toupper(int ch)
{
    return std::toupper(ch);
}
/**
 * @brief 将字符串中的所有字符转换为小写。
 *
 * @param str 待处理的字符串指针。
 * @return char* 处理后的字符串指针。
 */
char *tolower(char *str);
/**
 * @brief 将字符串中的所有字符转换为大写。
 *
 * @param str 待处理的字符串指针。
 * @return char* 处理后的字符串指针。
 */
char *toupper(char *str);
/**
 * @brief 将 std::string 中的所有字符转换为小写。
 *
 * @param str 待处理的 std::string 对象引用。
 * @return std::string& 处理后的 std::string 对象引用。
 */
std::string &tolower(std::string &str);
/**
 * @brief 将 std::string 中的所有字符转换为大写。
 *
 * @param str 待处理的 std::string 对象引用。
 * @return std::string& 处理后的 std::string 对象引用。
 */
std::string &toupper(std::string &str);

//
/**
 * @brief 清除字符串中指定大小范围内的空字符（'\0'）。
 *
 * @param data 待处理的字符串指针。
 * @param size 处理的范围大小。
 * @return char* 处理后的字符串指针。
 */
char *clear_null(char *data, int64_t size);
/**
 * @brief 清除 std::string 中的空字符（'\0'）。
 *
 * @param data 待处理的 std::string 对象引用。
 * @return std::string& 处理后的 std::string 对象引用。
 */
std::string &clear_null(std::string &data);

// windows 没有
/**
 * @brief 不区分大小写比较两个字符串。
 *
 * 在 Windows 平台使用 _stricmp 实现，其他平台使用系统的 strcasecmp 函数。
 *
 * @param a 第一个字符串指针。
 * @param b 第二个字符串指针。
 * @return int 比较结果：0 表示相等，小于 0 表示 a 小于 b，大于 0 表示 a 大于 b。
 */
int strcasecmp(const char *a, const char *b);
/**
 * @brief 不区分大小写比较两个字符串的前 c 个字符。
 *
 * @param a 第一个字符串指针。
 * @param b 第二个字符串指针。
 * @param c 要比较的字符数量。
 * @return int 比较结果：0 表示相等，小于 0 表示 a 小于 b，大于 0 表示 a 大于 b。
 */
inline int strncasecmp(const char *a, const char *b, size_t c);
/**
 * @brief 使用可变参数列表格式化字符串到 std::string 中，最多支持 1024 个字符。
 *
 * @param str 用于存储格式化结果的 std::string 对象引用。
 * @param format 格式化字符串。
 * @param ap 可变参数列表。
 * @return std::string& 存储格式化结果的 std::string 对象引用。
 */
std::string &vsprintf_1024(std::string &str, const char *format, va_list ap);

#ifdef __linux__
/**
 * @brief 在 Linux 平台使用可变参数格式化字符串到 std::string 中，最多支持 1024 个字符。
 *
 * @param str 用于存储格式化结果的 std::string 对象引用。
 * @param format 格式化字符串。
 * @param ... 可变参数。
 * @return std::string& 存储格式化结果的 std::string 对象引用。
 */
std::string __attribute__((format(gnu_printf, 2, 3))) & sprintf_1024(std::string &str, const char *format, ...);
#else  // __linux__
/**
 * @brief 在非 Linux 平台使用可变参数格式化字符串到 std::string 中，最多支持 1024 个字符。
 *
 * @param str 用于存储格式化结果的 std::string 对象引用。
 * @param format 格式化字符串。
 * @param ... 可变参数。
 * @return std::string& 存储格式化结果的 std::string 对象引用。
 */
std::string &sprintf_1024(std::string &str, const char *format, ...);
#endif // __linux__

/**
 * @brief 根据指定的分隔符分割字符串。
 *
 * @param s 待分割的字符串指针。
 * @param len 字符串的长度，-1 表示自动计算。
 * @param delims 分隔符集合。
 * @param ignore_empty_token 是否忽略空的分割结果，默认为 false。
 * @return std::vector<std::string> 分割后的字符串向量。
 */
std::vector<std::string> split(const char *s, int len, const char *delims, bool ignore_empty_token = false);
/**
 * @brief 根据指定的分隔符分割字符串，自动计算字符串长度。
 *
 * @param s 待分割的字符串指针。
 * @param delims 分隔符集合。
 * @param ignore_empty_token 是否忽略空的分割结果，默认为 false。
 * @return std::vector<std::string> 分割后的字符串向量。
 */
inline std::vector<std::string> split(const char *s, const char *delims, bool ignore_empty_token = false)
{
    return split(s, -1, delims, ignore_empty_token);
}
/**
 * @brief 根据指定的分隔符分割 std::string。
 *
 * @param s 待分割的 std::string 对象。
 * @param delims 分隔符集合。
 * @param ignore_empty_token 是否忽略空的分割结果，默认为 false。
 * @return std::vector<std::string> 分割后的字符串向量。
 */
inline std::vector<std::string> split(const std::string &s, const char *delims, bool ignore_empty_token = false)
{
    return split(s.c_str(), s.size(), delims, ignore_empty_token);
}

/**
 * @brief 根据指定的单个字符分隔符分割字符串。
 *
 * @param s 待分割的字符串指针。
 * @param len 字符串的长度，-1 表示自动计算。
 * @param delim 单个字符分隔符。
 * @param ignore_empty_token 是否忽略空的分割结果，默认为 false。
 * @return std::vector<std::string> 分割后的字符串向量。
 */
std::vector<std::string> split(const char *s, int len, int delim, bool ignore_empty_token = false);
/**
 * @brief 根据指定的单个字符分隔符分割字符串，自动计算字符串长度。
 *
 * @param s 待分割的字符串指针。
 * @param delim 单个字符分隔符。
 * @param ignore_empty_token 是否忽略空的分割结果，默认为 false。
 * @return std::vector<std::string> 分割后的字符串向量。
 */
inline std::vector<std::string> split(const char *s, int delim, bool ignore_empty_token = false)
{
    return split(s, -1, delim, ignore_empty_token);
}
/**
 * @brief 根据指定的单个字符分隔符分割 std::string。
 *
 * @param s 待分割的 std::string 对象。
 * @param delim 单个字符分隔符。
 * @param ignore_empty_token 是否忽略空的分割结果，默认为 false。
 * @return std::vector<std::string> 分割后的字符串向量。
 */
inline std::vector<std::string> split(const std::string &s, int delim, bool ignore_empty_token = false)
{
    return split(s.c_str(), s.size(), delim, ignore_empty_token);
}

/**
 * @brief 根据指定的分隔符分割字符串，并忽略空的分割结果。
 *
 * @param s 待分割的字符串指针。
 * @param len 字符串的长度，-1 表示自动计算。
 * @param delims 分隔符集合。
 * @return std::vector<std::string> 分割后的字符串向量。
 */
inline std::vector<std::string> split_and_ignore_empty_token(const char *s, int len, const char *delims)
{
    return split(s, len, delims, true);
}
/**
 * @brief 根据指定的分隔符分割字符串，并忽略空的分割结果，自动计算字符串长度。
 *
 * @param s 待分割的字符串指针。
 * @param delims 分隔符集合。
 * @return std::vector<std::string> 分割后的字符串向量。
 */
inline std::vector<std::string> split_and_ignore_empty_token(const char *s, const char *delims)
{
    return split_and_ignore_empty_token(s, -1, delims);
}
/**
 * @brief 根据指定的分隔符分割 std::string，并忽略空的分割结果。
 *
 * @param s 待分割的 std::string 对象。
 * @param delims 分隔符集合。
 * @return std::vector<std::string> 分割后的字符串向量。
 */
inline std::vector<std::string> split_and_ignore_empty_token(const std::string &s, const char *delims)
{
    return split_and_ignore_empty_token(s.c_str(), s.size(), delims);
}

/**
 * @brief 根据指定的单个字符分隔符分割字符串，并忽略空的分割结果。
 *
 * @param s 待分割的字符串指针。
 * @param len 字符串的长度，-1 表示自动计算。
 * @param delim 单个字符分隔符。
 * @return std::vector<std::string> 分割后的字符串向量。
 */
inline std::vector<std::string> split_and_ignore_empty_token(const char *s, int len, int delim)
{
    return split(s, len, delim, true);
}

/**
 * @brief 根据指定的单个字符分隔符分割字符串，并忽略空的分割结果，自动计算字符串长度。
 *
 * @param s 待分割的字符串指针。
 * @param delim 单个字符分隔符。
 * @return std::vector<std::string> 分割后的字符串向量。
 */
inline std::vector<std::string> split_and_ignore_empty_token(const char *s, int delim)
{
    return split_and_ignore_empty_token(s, -1, delim);
}
/**
 * @brief 根据指定的单个字符分隔符分割 std::string，并忽略空的分割结果。
 *
 * @param s 待分割的 std::string 对象。
 * @param delim 单个字符分隔符。
 * @return std::vector<std::string> 分割后的字符串向量。
 */
inline std::vector<std::string> split_and_ignore_empty_token(const std::string &s, int delim)
{
    return split_and_ignore_empty_token(s.c_str(), s.size(), delim);
}

// windows 没有 memmem
/**
 * @brief 在 Windows 平台实现 memmem 函数，用于在大字符串中查找小字符串。
 *
 * @param l 大字符串指针。
 * @param l_len 大字符串的长度。
 * @param s 小字符串指针。
 * @param s_len 小字符串的长度。
 * @return void* 找到小字符串的起始指针，未找到返回 nullptr。
 */
void *no_memmem(const void *l, int64_t l_len, const void *s, int64_t s_len);
/**
 * @brief 根据平台选择使用系统的 memmem 函数或自定义的 no_memmem 函数。
 *
 * @param l 大字符串指针。
 * @param l_len 大字符串的长度。
 * @param s 小字符串指针。
 * @param s_len 小字符串的长度。
 * @return void* 找到小字符串的起始指针，未找到返回 nullptr。
 */
inline static void *memmem(const void *l, int64_t l_len, const void *s, int64_t s_len)
{
#ifdef _WIN64
    return no_memmem(l, l_len, s, s_len);
#else  // _WIN64
    return ::memmem(l, l_len, s, s_len);
#endif // _WIN64
}

// windows 没有 strcasecmp
/**
 * @brief 不区分大小写比较两个字符串，根据平台选择不同的实现。
 *
 * 在 Windows 平台使用 _stricmp 实现，其他平台使用系统的 strcasecmp 函数。
 *
 * @param a 第一个字符串指针。
 * @param b 第二个字符串指针。
 * @return int 比较结果：0 表示相等，小于 0 表示 a 小于 b，大于 0 表示 a 大于 b。
 */
inline int strcasecmp(const char *a, const char *b)
{
#ifdef _WIN64
    return _stricmp(a, b);
#else  // _WIN64
    return ::strcasecmp(a, b);
#endif // _WIN64
}

/**
 * @brief 不区分大小写比较两个字符串的前 c 个字符，根据平台选择不同的实现。
 *
 * 在 Windows 平台使用 _strnicmp 实现，其他平台使用系统的 strncasecmp 函数。
 *
 * @param a 第一个字符串指针。
 * @param b 第二个字符串指针。
 * @param c 要比较的字符数量。
 * @return int 比较结果：0 表示相等，小于 0 表示 a 小于 b，大于 0 表示 a 大于 b。
 */
inline int strncasecmp(const char *a, const char *b, size_t c)
{
#ifdef _WIN64
    return _strnicmp(a, b, c);
#else  // _WIN64
    return ::strncasecmp(a, b, c);
#endif // _WIN64
}

#ifdef _WIN64
/**
 * @brief 在 Windows 平台实现 strcasestr 函数，用于在大字符串中不区分大小写查找小字符串。
 *
 * @param haystack 大字符串指针。
 * @param needle 小字符串指针。
 * @return char* 找到小字符串的起始指针，未找到返回 nullptr。
 */
char *strcasestr(const char *haystack, const char *needle);
#else  // _WIN64
/**
 * @brief 在非 Windows 平台调用系统的 strcasestr 函数，用于在大字符串中不区分大小写查找小字符串。
 *
 * @param haystack 大字符串指针。
 * @param needle 小字符串指针。
 * @return char* 找到小字符串的起始指针，未找到返回 nullptr。
 */
inline char *strcasestr(const char *haystack, const char *needle)
{
    return (char *)(void *)::strcasestr(haystack, needle);
}
#endif // _WIN64

std::string str_replace(const std::string &input, const std::string &from, const std::string &to);

std::string join(const std::vector<std::string> &strs, const std::string &delimiter);
std::string join(const std::list<std::string> &strs, const std::string &delimiter);

/**
 * @brief 显示词典的调试信息。
 *
 * 该函数用于打印词典的相关信息，方便调试使用。
 *
 * @param dt 待显示调试信息的词典对象。
 */
void debug_show(const dict &dt);

/**
 * @brief 从词典中获取指定键对应的 C 风格字符串值。
 *
 * 如果键存在，则返回对应的值；如果键不存在，则返回默认值。
 *
 * @param dt 词典对象。
 * @param key 要查找的键，C 风格字符串。
 * @param def_val 键不存在时返回的默认值，默认为空字符串。
 * @return const char* 查找到的值或默认值。
 */
const char *get_cstring(const dict &dt, const char *key, const char *def_val = "");

/**
 * @brief 从词典中获取指定键对应的 C 风格字符串值。
 *
 * 如果键存在，则返回对应的值；如果键不存在，则返回默认值。
 *
 * @param dt 词典对象。
 * @param key 要查找的键，std::string 类型。
 * @param def_val 键不存在时返回的默认值，默认为空字符串。
 * @return const char* 查找到的值或默认值。
 */
const char *get_cstring(const dict &dt, const std::string &key, const char *def_val = "");

/**
 * @brief 从词典中获取指定键对应的 std::string 类型值。
 *
 * 如果键存在，则返回对应的值；如果键不存在，则返回默认值。
 *
 * @param dt 词典对象。
 * @param key 要查找的键，C 风格字符串。
 * @param def_val 键不存在时返回的默认值，默认为空字符串。
 * @return std::string 查找到的值或默认值。
 */
std::string get_string(const dict &dt, const char *key, const char *def_val = "");

/**
 * @brief 从词典中获取指定键对应的 std::string 类型值的引用。
 *
 * 如果键存在，则返回对应的值的引用；如果键不存在，则返回默认值的引用。
 *
 * @param dt 词典对象。
 * @param key 要查找的键，std::string 类型。
 * @param def_val 键不存在时返回的默认值的引用，默认为 var_blank_string。
 * @return const std::string& 查找到的值或默认值的引用。
 */
const std::string &get_string(const dict &dt, const std::string &key, const std::string &def_val = var_blank_string);

/**
 * @brief 从词典中获取指定键对应的布尔值。
 *
 * 如果键存在，则将对应的值转换为布尔值返回；如果键不存在，则返回默认值。
 *
 * @param dt 词典对象。
 * @param key 要查找的键，C 风格字符串。
 * @param def_val 键不存在时返回的默认值，默认为 false。
 * @return bool 查找到的值转换后的布尔值或默认值。
 */
bool get_bool(const dict &dt, const char *key, bool def_val = false);

/**
 * @brief 从词典中获取指定键对应的布尔值。
 *
 * 如果键存在，则将对应的值转换为布尔值返回；如果键不存在，则返回默认值。
 *
 * @param dt 词典对象。
 * @param key 要查找的键，std::string 类型。
 * @param def_val 键不存在时返回的默认值，默认为 false。
 * @return bool 查找到的值转换后的布尔值或默认值。
 */
bool get_bool(const dict &dt, const std::string &key, bool def_val = false);

/**
 * @brief 从词典中获取指定键对应的整数值。
 *
 * 如果键存在，则将对应的值转换为整数返回；如果键不存在，则返回默认值。
 *
 * @param dt 词典对象。
 * @param key 要查找的键，C 风格字符串。
 * @param def_val 键不存在时返回的默认值，默认为 0。
 * @return int 查找到的值转换后的整数或默认值。
 */
int get_int(const dict &dt, const char *key, int def_val = 0);

/**
 * @brief 从词典中获取指定键对应的整数值。
 *
 * 如果键存在，则将对应的值转换为整数返回；如果键不存在，则返回默认值。
 *
 * @param dt 词典对象。
 * @param key 要查找的键，std::string 类型。
 * @param def_val 键不存在时返回的默认值，默认为 0。
 * @return int 查找到的值转换后的整数或默认值。
 */
int get_int(const dict &dt, const std::string &key, int def_val = 0);

/**
 * @brief 从词典中获取指定键对应的长整数值。
 *
 * 如果键存在，则将对应的值转换为长整数返回；如果键不存在，则返回默认值。
 *
 * @param dt 词典对象。
 * @param key 要查找的键，C 风格字符串。
 * @param def_val 键不存在时返回的默认值，默认为 -1。
 * @return int64_t 查找到的值转换后的长整数或默认值。
 */
int64_t get_long(const dict &dt, const char *key, int64_t def_val = 0);

/**
 * @brief 从词典中获取指定键对应的长整数值。
 *
 * 如果键存在，则将对应的值转换为长整数返回；如果键不存在，则返回默认值。
 *
 * @param dt 词典对象。
 * @param key 要查找的键，std::string 类型。
 * @param def_val 键不存在时返回的默认值，默认为 0。
 * @return int64_t 查找到的值转换后的长整数或默认值。
 */
int64_t get_long(const dict &dt, const std::string &key, int64_t def_val = 0);

/**
 * @brief 从词典中获取指定键对应的秒数值。
 *
 * 如果键存在，则将对应的值转换为秒数返回；如果键不存在，则返回默认值。
 *
 * @param dt 词典对象。
 * @param key 要查找的键，C 风格字符串。
 * @param def_val 键不存在时返回的默认值，默认为 0。
 * @return int64_t 查找到的值转换后的秒数或默认值。
 */
int64_t get_second(const dict &dt, const char *key, int64_t def_val = 0);

/**
 * @brief 从词典中获取指定键对应的秒数值。
 *
 * 如果键存在，则将对应的值转换为秒数返回；如果键不存在，则返回默认值。
 *
 * @param dt 词典对象。
 * @param key 要查找的键，std::string 类型。
 * @param def_val 键不存在时返回的默认值，默认为 0。
 * @return int64_t 查找到的值转换后的秒数或默认值。
 */
int64_t get_second(const dict &dt, const std::string &key, int64_t def_val = 0);

/**
 * @brief 从词典中获取指定键对应的字节大小值。
 *
 * 如果键存在，则将对应的值转换为字节大小返回；如果键不存在，则返回默认值。
 *
 * @param dt 词典对象。
 * @param key 要查找的键，C 风格字符串。
 * @param def_val 键不存在时返回的默认值，默认为 0。
 * @return int64_t 查找到的值转换后的字节大小或默认值。
 */
int64_t get_size(const dict &dt, const char *key, int64_t def_val = 0);

/**
 * @brief 从词典中获取指定键对应的字节大小值。
 *
 * 如果键存在，则将对应的值转换为字节大小返回；如果键不存在，则返回默认值。
 *
 * @param dt 词典对象。
 * @param key 要查找的键，std::string 类型。
 * @param def_val 键不存在时返回的默认值，默认为 0。
 * @return int64_t 查找到的值转换后的字节大小或默认值。
 */
int64_t get_size(const dict &dt, const std::string &key, int64_t def_val = 0);

/**
 * @brief 生成一个唯一的 ID。
 *
 * 该函数用于生成一个唯一的字符串 ID，可用于各种需要唯一标识的场景。
 *
 * @return std::string 生成的唯一 ID。
 */
std::string build_unique_id();

/**
 * @brief 将字节大小转换为人类可读的字符串格式。
 *
 * 该函数将给定的字节大小转换为带有合适单位（如 KB、MB、GB 等）的字符串。
 *
 * @param a 要转换的字节大小。
 * @return std::string 转换后的人类可读的字符串。
 */
std::string human_byte_size(int64_t a);
inline std::string human_number(int64_t a)
{
    return human_byte_size(a);
}

std::string human_kmg_size(double size);

/**
 * @brief 外部声明的默认 MIME 类型字符串。
 *
 * 该变量存储了默认的 MIME 类型，可在获取 MIME 类型时作为默认值使用。
 */
extern const char *var_default_mime_type;

/**
 * @brief 根据文件路径名获取对应的 MIME 类型。
 *
 * 如果能根据路径名找到对应的 MIME 类型，则返回该类型；否则返回默认值。
 *
 * @param pathname 文件的路径名。
 * @param def 找不到 MIME 类型时返回的默认值，默认为 nullptr。
 * @return const char* 查找到的 MIME 类型或默认值。
 */
const char *get_mime_type(const char *pathname, const char *def = nullptr);

/**
 * @brief 根据文件路径名获取对应的 MIME 类型。
 *
 * 功能与 get_mime_type 相同，是该函数的另一个命名版本。
 *
 * @param pathname 文件的路径名。
 * @param def 找不到 MIME 类型时返回的默认值，默认为 nullptr。
 * @return const char* 查找到的 MIME 类型或默认值。
 */
const char *get_mime_type_from_pathname(const char *pathname, const char *def = nullptr);

template <typename T, typename Container = std::list<T *>>
bool popup(Container &C, T *&r)
{
    if (C.empty())
    {
        return false;
    }
    r = C.back();
    C.pop_back();
    return true;
}

template <typename T, typename V, typename Container = std::map<T, V>>
bool find(Container &C, const T &k, V &r)
{
    auto it = C.find(k);
    if (it == C.end())
    {
        return false;
    }
    r = it->second;
    return true;
}

// 生成序列号, mac: mac地址
std::string license_build(const char *salt, const char *mac);
// 验证序列号, -1: 错误, 0: 不匹配, 1: 匹配
int license_check(const char *salt, const char *license);

/* encode/decode, ################################################## */
static const char var_encoding_type_base64 = 'B';
static const char var_encoding_type_qp = 'Q';
static const char var_encoding_type_none = '\0';
// base64
extern const unsigned char var_base64_decode_table[];
void base64_encode(const void *src, int src_size, std::string &str, bool mime_flag = false);
inline void base64_encode(const std::string &src, std::string &str, bool mime_flag = false)
{
    base64_encode(src.c_str(), src.size(), str, mime_flag);
}
std::string base64_encode(const void *src, int src_size, bool mime_flag = false);
inline std::string base64_encode(const std::string &src, bool mime_flag = false)
{
    return base64_encode(src.c_str(), src.size(), mime_flag);
}
//
void base64_decode(const void *src, int src_size, std::string &str);
inline void base64_decode(const std::string &src, std::string &str)
{
    base64_decode(src.c_str(), src.size(), str);
}
std::string base64_decode(const void *src, int src_size);
inline std::string base64_decode(const std::string &src)
{
    return base64_decode(src.c_str(), src.size());
}
int base64_decode_get_valid_len(const void *src, int src_size);
int base64_encode_get_min_len(int in_len, bool mime_flag = false);
// qp
void qp_encode_2045(const void *src, int src_size, std::string &result, bool mime_flag = false);
inline void qp_encode_2045(const std::string &src, std::string &result, bool mime_flag = false)
{
    return qp_encode_2045(src.c_str(), src.size(), result, mime_flag);
}
std::string qp_encode_2045(const void *src, int src_size, bool mime_flag = false);
inline std::string qp_encode_2045(const std::string &src, bool mime_flag = false)
{
    return qp_encode_2045(src.c_str(), src.size(), mime_flag);
}
void qp_decode_2045(const void *src, int src_size, std::string &str);
inline void qp_decode_2045(const std::string &src, std::string &str)
{
    qp_decode_2045(src.c_str(), src.size(), str);
}
std::string qp_decode_2045(const void *src, int src_size);
inline std::string qp_decode_2045(const std::string &src)
{
    return qp_decode_2045(src.c_str(), src.size());
}
//
void qp_encode_2047(const void *src, int src_size, std::string &result);
inline void qp_encode_2047(const std::string &src, std::string &result)
{
    return qp_encode_2047(src.c_str(), src.size(), result);
}
std::string qp_encode_2047(const void *src, int src_size);
inline std::string qp_encode_2047(const std::string &src)
{
    return qp_encode_2047(src.c_str(), src.size());
}
void qp_decode_2047(const void *src, int src_size, std::string &str);
inline void qp_decode_2047(const std::string &src, std::string &str)
{
    qp_decode_2047(src.c_str(), src.size(), str);
}
std::string qp_decode_2047(const void *src, int src_size);
inline std::string qp_decode_2047(const std::string &src)
{
    return qp_decode_2047(src.c_str(), src.size());
}
int qp_decode_get_valid_len(const void *src, int src_size);
// hex
void hex_encode(const void *src, int src_size, std::string &str);
inline void hex_encode(const std::string &src, std::string &str)
{
    hex_encode(src.c_str(), src.size(), str);
}
std::string hex_encode(const void *src, int src_size);
inline std::string hex_encode(const std::string &src)
{
    return hex_encode(src.c_str(), src.size());
}
void hex_decode(const void *src, int src_size, std::string &str);
inline void hex_decode(const std::string &src, std::string &str)
{
    hex_decode(src.c_str(), src.size(), str);
}
std::string hex_decode(const void *src, int src_size);
inline std::string hex_decode(const std::string &src)
{
    return hex_decode(src.c_str(), src.size());
}
// xml
void xml_unescape_string(const char *data, int len, std::string &content);
inline void xml_unescape_string(std::string &content, const char *data, int len)
{
    xml_unescape_string(data, len, content);
}
std::string xml_unescape_string(const char *data, int len);
// uu
void uudecode(const void *src, int src_size, std::string &str);
// ncr
int ncr_decode(int ins, char *wchar);
// csv
/**
 * 从 CSV 数据中反序列化一行数据.
 * 该函数根据 CSV 数据的格式,从给定的数据区间中反序列化一行数据,并将结果存储在 fields 向量中.
 * 返回下一行数据的起始位置.
 */
const char *csv_unserialize_one_row(const char *data_start, const char *data_end, std::vector<std::string> &fields);
//
std::string csv_serialize_one_field(const std::string &field);

/* hash ############################################################# */
/* 最经典的hash函数, 需要更高级的可以考虑 crc16, crc32, crc64, 甚至md5 等*/
inline unsigned hash_djb(const void *buf, int64_t len, unsigned int initial = 5381)
{
    const unsigned char *p = (const unsigned char *)buf;
    const unsigned char *end = p + len;
    unsigned hash = initial; /* start value */
    while (p < end)
    {
        hash = (hash + (hash << 5)) ^ *p++;
    }
    return hash;
}

unsigned short int crc16(const void *data, int len = -1, unsigned short int init_value = 0);
inline unsigned short int crc16(const std::string &data, unsigned short int init_value = 0)
{
    return crc16(data.c_str(), data.size(), init_value);
}
unsigned int crc32(const void *data, int size = -1, unsigned int init_value = 0);
inline unsigned int crc32(const std::string &data, unsigned int init_value = 0)
{
    return crc32(data.c_str(), data.size(), init_value);
}
uint64_t crc64(const void *data, int size = -1, uint64_t init_value = 0);
inline uint64_t crc64(const std::string &data, uint64_t init_value = 0)
{
    return crc64(data.c_str(), data.size(), init_value);
}
std::string md5(const void *data, int len = -1);
inline std::string md5(const std::string &data)
{
    return md5(data.c_str(), (unsigned int)data.size());
}

/* config ########################################################## */
// 线程, 读写不安全
// 线程, 只有读是安全的
class config : public std::map<std::string, std::string>
{
public:
    config();
    virtual ~config();
    config &reset();
    virtual inline void afterUpdate() {};
    config &update(const char *key, const char *val, int vlen = -1);
    config &update(const char *key, const std::string &val);
    config &update(const std::string &key, const char *val, int vlen = -1);
    config &update(const std::string &key, const std::string &val);
    inline config &update(const std::string &key, int64_t val) { return update(key, std::to_string(val)); }
    inline config &update(const std::string &key, bool val) { return update(key, val ? "1" : "0"); }
    config &remove(const char *key);
    config &remove(const std::string &key);
    // 从文件加载配置, 且覆盖
    bool load_from_file(const char *pathname);
    bool load_from_file(const std::string &pathname)
    {
        return load_from_file(pathname.c_str());
    }
    // 从另一个配置复制
    config &load_another(config &another);
    config &debug_show();
    // 获取值
    std::string *get_value(const char *key);
    std::string *get_value(const std::string &key);
    const char *get_cstring(const char *key, const char *def_val = "");
    const char *get_cstring(const std::string &key, const char *def_val = "");
    std::string get_string(const char *key, const char *def_val = "");
    std::string get_string(const std::string &key, const char *def_val = "");
    const std::string &get_string(const std::string &key, const std::string &def_val);
    // y/Y/t/T/1 => true, n/N/f/F/0 => false
    bool get_bool(const char *key, bool def_val = false);
    bool get_bool(const std::string &key, bool def_val = false);
    // atoi
    int get_int(const char *key, int def_val = 0);
    int get_int(const std::string &key, int def_val = 0);
    // atol
    int64_t get_long(const char *key, int64_t def_val = 0);
    int64_t get_long(const std::string &key, int64_t def_val = 0);
    // 1s, 1m, 1h, 1d, 1w
    int64_t get_second(const char *key, int64_t def_val = 0);
    int64_t get_second(const std::string &key, int64_t def_val = 0);
    // 1b, 1k, 1m, 1g
    int64_t get_size(const char *key, int64_t def_val = 0);
    int64_t get_size(const std::string &key, int64_t def_val = 0);
    //
    std::list<int64_t> get_int64_list(const char *key, const char *sep = ",");
    inline std::list<int64_t> get_int64_list(const std::string &key, const char *sep = ",")
    {
        return get_int64_list(key.c_str(), sep);
    }
    //
    std::vector<int64_t> get_int64_vector(const char *key, const char *sep = ",");
    inline std::vector<int64_t> get_int64_vector(const std::string &key, const char *sep = ",")
    {
        return get_int64_vector(key.c_str(), sep);
    }

private:
    void *unused_;
};
extern config var_main_config;

/* main argument ################################################### */
extern bool var_memleak_check_enable;
extern bool var_sigint_flag;

zcc_general_namespace_begin(main_argument);
struct option
{
    const char *key;
    const char *val;
};

extern int var_argc;
extern char **var_argv;
extern std::vector<option> var_options;
extern std::vector<const char *> var_parameters;
extern int var_parameter_argc;
extern char **var_parameter_argv;
void run(int argc, char **argv, bool cmd_mode = true);
zcc_general_namespace_end(main_argument);

/* os ############################################################### */
// 进程 ID
int get_process_id();
inline int getpid() { return get_process_id(); }

// 父进程ID
int get_parent_process_id();
inline int getppid() { return get_parent_process_id(); }

// 线程ID
int get_thread_id();
inline int gettid() { return get_thread_id(); }

// 程序真实的绝对路径名称
std::string get_cmd_pathname();
// 程序真实的名字
std::string get_cmd_name();

#ifdef __linux__
/**
 * @brief 快速设置资源限制。
 *
 * 该函数用于快速设置系统资源限制，具体限制类型由 cmd 参数指定，
 * 当前限制值由 cur_val 参数指定。
 *
 * @param cmd 资源限制命令，用于指定要设置的资源类型。
 * @param cur_val 当前要设置的资源限制值。
 * @return bool 如果设置成功返回 true，失败返回 false。
 */
bool quick_setrlimit(int cmd, unsigned long cur_val);

/**
 * @brief 设置核心转储文件的最大大小。
 *
 * 该函数用于设置核心转储文件的最大大小，单位为兆字节。
 *
 * @param megabyte 核心转储文件的最大大小，单位为兆字节。
 * @return bool 如果设置成功返回 true，失败返回 false。
 */
bool set_core_file_size(int megabyte);

/**
 * @brief 设置进程的最大内存使用量。
 *
 * 该函数用于设置进程的最大内存使用量，单位为兆字节。
 *
 * @param megabyte 进程的最大内存使用量，单位为兆字节。
 * @return bool 如果设置成功返回 true，失败返回 false。
 */
bool set_max_mem(int megabyte);

/**
 * @brief 设置控制组（cgroup）的名称。
 *
 * 该函数用于设置控制组的名称，控制组用于对进程进行资源管理。
 *
 * @param name 要设置的控制组名称。
 * @return bool 如果设置成功返回 true，失败返回 false。
 */
bool set_cgroup_name(const char *name);

/**
 * @brief 获取系统可用内存的大小。
 *
 * 该函数用于获取系统当前可用内存的大小，单位为字节。
 *
 * @return int64_t 系统可用内存的大小，单位为字节。
 */
int64_t get_MemAvailable();

/**
 * @brief 获取系统的 CPU 核心数。
 *
 * 该函数用于获取系统中可用的 CPU 核心数量。
 *
 * @return int 系统的 CPU 核心数。
 */
int get_cpu_core_count();

// 是否是终端
bool isatty();

/**
 * @brief 改变进程的根目录并切换用户。
 *
 * 该函数用于改变进程的根目录到指定的目录，并将进程的有效用户切换到指定用户。
 *
 * @param root_dir 要设置为进程根目录的路径。
 * @param user_name 要切换到的用户名称。
 * @return int 如果操作成功返回 0，失败返回一个非零错误码。
 */
int chroot_user(const char *root_dir, const char *user_name);
#endif // __linux__

// 获取本机mac地址
int get_mac_address(std::vector<std::string> &mac_list);

void signal(int signum, void (*handler)(int));
void signal_ignore(int signum);

/* fs ############################################################### */
/**
 * @brief 用于内存映射文件读取的类。
 *
 * 该类提供了打开、关闭内存映射文件的功能，
 * 并能获取映射文件的大小和数据指针。
 */
class mmap_reader
{
public:
    /**
     * @brief 构造函数，初始化对象。
     */
    mmap_reader();
    /**
     * @brief 析构函数，释放资源。
     */
    ~mmap_reader();
    /**
     * @brief 打开指定路径的文件并进行内存映射。
     *
     * @param pathname 文件的路径名。
     * @return int 操作结果，成功返回 >= 0，失败返回 -1
     */
    int open(const char *pathname);
    /**
     * @brief 重载的 open 函数，接受 std::string 类型的路径名。
     *
     * @param pathname 文件的路径名。
     * @return int 操作结果，成功返回 >= 0，失败返回 -1
     */
    inline int open(const std::string &pathname)
    {
        return open(pathname.c_str());
    }
    /**
     * @brief 关闭内存映射文件，释放资源。
     *
     * @return int 操作结果，成功返回 >= 0，失败返回 -1
     */
    int close();
    //
    inline int64_t get_size() { return size_; }
    //
    inline const char *get_data() { return data_; }

protected:
#ifdef _WIN64
    // Windows 平台下的文件句柄，初始化为 ZCC_VOID_PTR_ONE
    void *fd_{ZCC_VOID_PTR_ONE};
    // Windows 平台下的文件映射句柄，初始化为 nullptr
    void *fm_{nullptr};
#else  // _WIN64
    // 非 Windows 平台下的文件描述符，初始化为 -1
    int fd_{-1};
#endif // _WIN64

public:
    // 映射后文件的长度，初始化为 -1
    int64_t size_{-1};
    // 映射后文件数据的指针，初始化为 nullptr
    const char *data_{nullptr};
};

/**
 * @brief 打开指定路径的文件。
 *
 * @param pathname 文件的路径名。
 * @param mode 文件打开模式。
 * @return FILE* 指向文件流的指针，失败时返回 nullptr。
 */
FILE *fopen(const char *pathname, const char *mode);
/**
 * @brief 重载的 fopen 函数，接受 std::string 类型的路径名。
 *
 * @param pathname 文件的路径名。
 * @param mode 文件打开模式。
 * @return FILE* 指向文件流的指针，失败时返回 nullptr。
 */
inline FILE *fopen(const std::string &pathname, const char *mode)
{
    return fopen(pathname.c_str(), mode);
}

#ifdef _WIN64
/**
 * @brief 从文件流中读取一行，直到遇到指定分隔符。
 *
 * @param lineptr 指向存储读取行的缓冲区指针。
 * @param n 指向缓冲区大小的指针。
 * @param delim 分隔符。
 * @param stream 文件流指针。
 * @return int64_t 读取的字符数，失败时返回 -1。
 */
int64_t getdelim(char **lineptr, int64_t *n, int delim, FILE *stream);
/**
 * @brief 从文件流中读取一行，直到遇到换行符。
 *
 * @param lineptr 指向存储读取行的缓冲区指针。
 * @param n 指向缓冲区大小的指针。
 * @param stream 文件流指针。
 * @return int64_t 读取的字符数，失败时返回 -1。
 */
inline int64_t getline(char **lineptr, int64_t *n, FILE *stream)
{
    return getdelim(lineptr, (int64_t *)n, '\n', stream);
}

#else  // _WIN64
/**
 * @brief 从文件流中读取一行，直到遇到指定分隔符。
 *
 * @param lineptr 指向存储读取行的缓冲区指针。
 * @param n 指向缓冲区大小的指针。
 * @param delim 分隔符。
 * @param stream 文件流指针。
 * @return int64_t 读取的字符数，失败时返回 -1。
 */
inline int64_t getdelim(char **lineptr, int64_t *n, int delim, FILE *stream)
{
    return ::getdelim(lineptr, (size_t *)n, delim, stream);
}
/**
 * @brief 从文件流中读取一行，直到遇到换行符。
 *
 * @param lineptr 指向存储读取行的缓冲区指针。
 * @param n 指向缓冲区大小的指针。
 * @param stream 文件流指针。
 * @return int64_t 读取的字符数，失败时返回 -1。
 */
inline int64_t getline(char **lineptr, int64_t *n, FILE *stream)
{
    return ::getline(lineptr, (size_t *)n, stream);
}
#endif // _WIN64

/**
 * @brief 获取指定路径的绝对路径。
 *
 * @param pathname 文件或目录的路径名。
 * @return std::string 绝对路径的字符串。
 */
std::string realpath(const char *pathname);
/**
 * @brief 重载的 realpath 函数，接受 std::string 类型的路径名。
 *
 * @param pathname 文件或目录的路径名。
 * @return std::string 绝对路径的字符串。
 */
inline std::string realpath(const std::string &pathname)
{
    return realpath(pathname.c_str());
}

#ifdef _WIN64
// Windows 平台下使用 _stat64i32 结构体进行文件状态查询
#define zcc_stat struct _stat64i32
/**
 * @brief 获取指定文件的状态信息。
 *
 * @param pathname 文件的路径名。
 * @param statbuf 指向存储文件状态信息的结构体指针。
 * @return int 操作结果，成功返回 1，失败返回 -1。
 */
int stat(const char *pathname, struct _stat64i32 *statbuf);
#else // _WIN64
// 非 Windows 平台下使用 stat 结构体进行文件状态查询
#define zcc_stat struct stat
/**
 * @brief 获取指定文件的状态信息。
 *
 * @param pathname 文件的路径名。
 * @param statbuf 指向存储文件状态信息的结构体指针。
 * @return int 操作结果，成功返回 0，失败返回 -1。
 */
int stat(const char *pathname, struct stat *statbuf);
#endif // _WIN64

/**
 * @brief 获取指定文件的大小。
 *
 * @param pathname 文件的路径名。
 * @return int64_t 文件的大小，失败时返回 -1。
 */
int64_t file_get_size(const char *pathname);
/**
 * @brief 重载的 file_get_size 函数，接受 std::string 类型的路径名。
 *
 * @param pathname 文件的路径名。
 * @return int64_t 文件的大小，失败时返回 -1。
 */
inline int64_t file_get_size(const std::string &pathname)
{
    return file_get_size(pathname.c_str());
}

/**
 * @brief 检查指定文件是否存在。
 *
 * @param pathname 文件的路径名。
 * @return int  1/0/-1, 存在/不存在/发生错误
 */
int file_exists(const char *pathname);
/**
 * @brief 重载的 file_exists 函数，接受 std::string 类型的路径名。
 *
 * @param pathname 文件的路径名。
 * @return int 0, -1, 1
 */
inline int file_exists(const std::string &pathname)
{
    return file_exists(pathname.c_str());
}

/**
 * @brief 检查指定路径是否为普通文件
 * @param pathname 文件路径(C字符串)
 * @return int 1:是普通文件, 0:不是普通文件, -1:发生错误
 */
int file_is_regular(const char *pathname);

/**
 * @brief 检查指定路径是否为普通文件(std::string重载版本)
 * @param pathname 文件路径(std::string)
 * @return int 1:是普通文件, 0:不是普通文件, -1:发生错误
 */
inline int file_is_regular(const std::string &pathname)
{
    return file_is_regular(pathname.c_str());
}

/**
 * @brief 检查指定路径是否为目录
 * @param pathname 目录路径(C字符串)
 * @return int 1:是目录, 0:不是目录, -1:发生错误
 */
int file_is_dir(const char *pathname);

/**
 * @brief 检查指定路径是否为目录(std::string重载版本)
 * @param pathname 目录路径(std::string)
 * @return int 1:是目录, 0:不是目录, -1:发生错误
 */
inline int file_is_dir(const std::string &pathname)
{
    return file_is_dir(pathname.c_str());
}

/**
 * @brief 将数据写入指定文件。
 *
 * @param pathname 文件的路径名。
 * @param data 要写入的数据指针。
 * @param len 要写入的数据长度。
 * @return int 失败返回 -1
 */
int file_put_contents(const char *pathname, const void *data, int len);
/**
 * @brief 重载的 file_put_contents 函数，接受 std::string 类型的路径名。
 *
 * @param pathname 文件的路径名。
 * @param data 要写入的数据指针。
 * @param len 要写入的数据长度。
 * @return int 失败返回 -1
 */
inline int file_put_contents(const std::string &pathname, const void *data, int len)
{
    return file_put_contents(pathname.c_str(), data, len);
}
/**
 * @brief 重载的 file_put_contents 函数，接受 std::string 类型的数据。
 *
 * @param pathname 文件的路径名。
 * @param data 要写入的字符串数据。
 * @return int 失败返回 -1
 */
inline int file_put_contents(const char *pathname, const std::string &data)
{
    return file_put_contents(pathname, data.c_str(), data.size());
}
/**
 * @brief 重载的 file_put_contents 函数，接受 std::string 类型的路径名和数据。
 *
 * @param pathname 文件的路径名。
 * @param data 要写入的字符串数据。
 * @return int 失败返回 -1
 */
inline int file_put_contents(const std::string &pathname, const std::string &data)
{
    return file_put_contents(pathname.c_str(), data.c_str(), data.size());
}

/**
 * @brief 读取指定文件的全部内容。
 *
 * @param pathname 文件的路径名。
 * @return std::string 文件的全部内容。
 */
std::string file_get_contents(const char *pathname);
/**
 * @brief 重载的 file_get_contents 函数，接受 std::string 类型的路径名。
 *
 * @param pathname 文件的路径名。
 * @return std::string 文件的全部内容。
 */
inline std::string file_get_contents(const std::string &pathname)
{
    return file_get_contents(pathname.c_str());
}

/**
 * @brief 读取指定文件的全部内容到字符串缓冲区。
 *
 * @param pathname 文件的路径名。
 * @param bf 存储文件内容的字符串缓冲区。
 * @return int64_t 失败时返回 -1
 */
int64_t file_get_contents(const char *pathname, std::string &bf);
/**
 * @brief 重载的 file_get_contents 函数，接受 std::string 类型的路径名。
 *
 * @param pathname 文件的路径名。
 * @param bf 存储文件内容的字符串缓冲区。
 * @return int64_t 读取的字节数，失败时返回 -1
 */
inline int64_t file_get_contents(const std::string &pathname, std::string &bf)
{
    return file_get_contents(pathname.c_str(), bf);
}

/**
 * @brief 读取指定文件的部分内容到字符串缓冲区。
 *
 * @param pathname 文件的路径名。
 * @param bf 存储文件部分内容的字符串缓冲区。
 * @return int64_t 失败时返回 -1。
 */
int64_t file_get_contents_sample(const char *pathname, std::string &bf);
/**
 * @brief 读取指定文件的部分内容。
 *
 * @param pathname 文件的路径名。
 * @return std::string 文件的部分内容。
 */
std::string file_get_contents_sample(const char *pathname);
inline std::string file_get_contents_sample(const std::string &pathname)
{
    return file_get_contents_sample(pathname.c_str());
}

/**
 * @brief 从标准输入读取全部内容到字符串缓冲区。
 *
 * @param bf 存储标准输入内容的字符串缓冲区。
 * @return int 失败返回 -1。
 */
int stdin_get_contents(std::string &bf);
/**
 * @brief 从标准输入读取全部内容。
 *
 * @return std::string 标准输入的全部内容。
 */
std::string stdin_get_contents();

/**
 * @brief 复制文件
 * @param sourcePathname 源文件路径(C字符串)
 * @param destPathname 目标文件路径(C字符串)
 * @return int 成功返回1，失败返回-1
 */
int file_copy(const char *sourcePathname, const char *destPathname);
inline int file_copy(const std::string &sourcePathname, const std::string &destPathname)
{
    return file_copy(sourcePathname.c_str(), destPathname.c_str());
}

/**
 * @brief 打开指定路径的文件。
 *
 * @param pathname 文件的路径名。
 * @param flags 打开文件的标志。
 * @param mode 文件的访问权限。
 * @return int 文件描述符，失败时返回 -1。
 */
int open(const char *pathname, int flags, int mode);
/**
 * @brief 重载的 open 函数，接受 std::string 类型的路径名。
 *
 * @param pathname 文件的路径名。
 * @param flags 打开文件的标志。
 * @param mode 文件的访问权限。
 * @return int 文件描述符，失败时返回 -1。
 */
inline int open(const std::string &pathname, int flags, int mode)
{
    return open(pathname.c_str(), flags, mode);
}

/**
 * @brief 创建一个空文件，如果文件已存在则更新其访问和修改时间。
 *
 * @param pathname 文件的路径名。
 * @return int 操作成功返回 >= 0，失败返回 -1。
 */
int touch(const char *pathname);
/**
 * @brief 重载的 touch 函数，接受 std::string 类型的路径名。
 *
 * @param pathname 文件的路径名。
 * @return int 操作成功返回 >= 0，失败返回 -1。
 */
inline int touch(const std::string &pathname)
{
    return touch(pathname.c_str());
}

/**
 * @brief 创建多个目录。
 *
 * @param paths 存储目录路径的向量。
 * @param mode 目录的访问权限，默认为 0666。
 * @return int 操作成功返回 1，失败返回 -1。
 */
int mkdir(std::vector<std::string> paths, int mode = 0666);
/**
 * @brief 可变参数函数，创建多个目录。
 *
 * @param mode 目录的访问权限。
 * @param path1 第一个目录路径。
 * @param ... 可变参数，后续的目录路径。
 * @return int 操作成功返回 >= 0，失败返回 -1。
 */
int mkdir(int mode, const char *path1, ...);
/**
 * @brief 创建指定路径的目录。
 *
 * @param pathname 目录的路径名。
 * @param mode 目录的访问权限，默认为 0666。
 * @return int 操作成功返回 >= 0，失败返回 -1。
 */
int mkdir(const char *pathname, int mode = 0666);

/**
 * @brief 重命名文件或目录。
 *
 * @param oldpath 旧的文件或目录路径名。
 * @param newpath 新的文件或目录路径名。
 * @return int 操作成功返回 >= 0，失败返回 -1。
 */
int rename(const char *oldpath, const char *newpath);
inline int rename(const std::string &oldpath, const std::string &newpath)
{
    return rename(oldpath.c_str(), newpath.c_str());
}

/**
 * @brief 删除指定文件。
 *
 * @param pathname 文件的路径名。
 * @return int 操作成功返回 1/0(文件存在/不存在)，失败返回 -1。
 */
int unlink(const char *pathname);
/**
 * @brief 重载的 unlink 函数，接受 std::string 类型的路径名。
 *
 * @param pathname 文件的路径名。
 * @return int 操作成功返回 >= 0，失败返回 -1。
 */
inline int unlink(const std::string &pathname)
{
    return unlink(pathname.c_str());
}

/**
 * @brief 创建一个硬链接。
 *
 * @param oldpath 源文件的路径名。
 * @param newpath 硬链接的路径名。
 * @return int 操作成功返回 0，失败返回 -1。
 */
int link(const char *oldpath, const char *newpath);
/**
 * @brief 强制创建一个硬链接，如果目标已存在则先删除。
 *
 * @param oldpath 源文件的路径名。
 * @param newpath 硬链接的路径名。
 * @param tmpdir 临时目录的路径名。
 * @return int 操作成功返回 >= 0，失败返回 -1。
 */
int link_force(const char *oldpath, const char *newpath, const char *tmpdir);

/**
 * @brief 创建一个符号链接。
 *
 * @param oldpath 源文件的路径名。
 * @param newpath 符号链接的路径名。
 * @return int 操作成功返回 >= 0，失败返回 -1。
 */
int symlink(const char *oldpath, const char *newpath);
/**
 * @brief 强制创建一个符号链接，如果目标已存在则先删除。
 *
 * @param oldpath 源文件的路径名。
 * @param newpath 符号链接的路径名。
 * @param tmpdir 临时目录的路径名。
 * @return int 操作成功返回 >= 0，失败返回 -1。
 */
int symlink_force(const char *oldpath, const char *newpath, const char *tmpdir);

/**
 * @brief 创建一个快捷方式链接。
 *
 * @param from 源文件的路径名。
 * @param to 快捷方式的路径名。
 * @return bool 创建成功返回 true，失败返回 false。
 */
bool create_shortcut_link(const char *from, const char *to);
/**
 * @brief 重载的 create_shortcut_link 函数，接受 std::string 类型的路径名。
 *
 * @param from 源文件的路径名。
 * @param to 快捷方式的路径名。
 * @return bool 创建成功返回 true，失败返回 false。
 */
inline bool create_shortcut_link(const std::string &from, const std::string &to)
{
    return create_shortcut_link(from.c_str(), to.c_str());
}

/**
 * @brief 删除指定目录。
 *
 * @param pathname 目录的路径名。
 * @param recurse_mode 是否递归删除，默认为 false。
 * @return int 操作成功返回 1，失败返回 -1。
 */
int rmdir(const char *pathname, bool recurse_mode = false);
/**
 * @brief 重载的 rmdir 函数，接受 std::string 类型的路径名。
 *
 * @param pathname 目录的路径名。
 * @param recurse_mode 是否递归删除，默认为 false。
 * @return int 操作成功返回 >= 0，失败返回 -1。
 */
inline int rmdir(const std::string &pathname, bool recurse_mode = false)
{
    return rmdir(pathname.c_str(), recurse_mode);
}

int rmdir_by_system_cmd(const char *pathname);

/**
 * @brief 表示目录项信息的结构体。
 */
struct dir_item_info
{
    // 文件名
    std::string filename;
    // 是否为目录
    bool dir{false};
    // 是否为普通文件
    bool regular{false};
    // 是否为命名管道
    bool fifo{false};
    // 是否为符号链接
    bool link{false};
    // 是否为套接字
    bool socket{false};
    // 是否为设备文件
    bool dev{false};
};

/**
 * @brief 扫描指定目录下的所有文件和子目录。
 *
 * @param dirname 目录的路径名。
 * @param filenames 存储目录项信息的向量。
 * @return int 扫描到的目录项数量，失败时返回 -1。
 */
int scandir(const char *dirname, std::vector<dir_item_info> &filenames);
inline int scandir(const std::string &dirname, std::vector<dir_item_info> &filenames)
{
    return scandir(dirname.c_str(), filenames);
}

/**
 * @brief 扫描指定目录下的所有文件和子目录，返回目录项信息向量。
 *
 * @param dirname 目录的路径名。
 * @return std::vector<dir_item_info> 包含目录项信息的向量。
 */
std::vector<dir_item_info> scandir(const char *dirname);
/**
 * @brief 重载的 scandir 函数，接受 std::string 类型的路径名。
 *
 * @param dirname 目录的路径名。
 * @return std::vector<dir_item_info> 包含目录项信息的向量。
 */
inline std::vector<dir_item_info> scandir(const std::string &dirname)
{
    return scandir(dirname.c_str());
}

/**
 * @brief 格式化文件名，去除非法字符等。
 *
 * @param filename 原始文件名。
 * @return std::string 格式化后的文件名。
 */
std::string format_filename(const char *filename);
/**
 * @brief 重载的 format_filename 函数，接受 std::string 类型的文件名。
 *
 * @param filename 原始文件名。
 * @return std::string 格式化后的文件名。
 */
inline std::string format_filename(const std::string &filename)
{
    return format_filename(filename.c_str());
}

/**
 * @brief 在指定目录或文件中查找匹配的文件。
 *
 * @param dir_or_file 存储目录或文件路径的向量。
 * @param pathname_match 文件名匹配模式，默认为 nullptr。
 * @return std::vector<std::string> 匹配的文件路径向量。
 */
std::vector<std::string> find_file_sample(std::vector<const char *> dir_or_file, const char *pathname_match = nullptr);
/**
 * @brief 在指定目录或文件中查找匹配的文件。
 *
 * @param dir_or_file 存储目录或文件路径的数组。
 * @param item_count 数组中元素的数量。
 * @param pathname_match 文件名匹配模式，默认为 nullptr。
 * @return std::vector<std::string> 匹配的文件路径向量。
 */

std::vector<std::string> find_file_sample(const char **dir_or_file, int item_count, const char *pathname_match = nullptr);

/**
 * @brief 拼接多个路径字符串为一个完整的路径。
 *
 * 该函数接受一个以 `path1` 开始的可变参数列表，将所有路径字符串拼接成一个完整的路径。
 * 拼接过程中会处理路径分隔符，确保路径格式正确。
 *
 * @param path1 第一个路径字符串。
 * @param ... 可变参数列表，包含后续的路径字符串。
 * @return std::string 拼接后的完整路径字符串。
 */
std::string path_concat(const char *path1, ...);

/**
 * @brief 获取指定路径的父目录名称。
 *
 * 该函数接受一个路径字符串作为输入，返回该路径对应的父目录的名称。
 * 如果路径没有父目录（例如根目录），则返回空字符串或根据具体实现处理。
 *
 * @param pathname 要处理的路径字符串。
 * @return std::string 父目录的名称，如果没有父目录则返回空字符串。
 */
std::string get_dirname(const std::string &pathname);

//
std::string get_filename(const std::string &pathname);

/**
 * @brief 从给定的路径名中提取目录名和文件名。
 *
 * 此函数接受一个C风格的字符串路径名，将路径中的目录名和文件名分别提取出来，
 * 并存储到传入的 `dirname` 和 `filename` 引用中。
 *
 * @param pathname 要处理的路径名，C风格字符串。
 * @param dirname 用于存储提取出的目录名的引用。
 * @param filename 用于存储提取出的文件名的引用。
 */
void get_dirname_and_filename(const std::string &pathname, std::string &dirname, std::string &filename);

/**
 * @brief 生成用于转储文件的路径名。
 *
 * 该函数根据指定的目录名和文件名生成一个用于转储文件的路径名。
 * 如果指定了最大循环次数 `max_loop`，则会尝试生成唯一的文件名，
 * 避免文件名冲突。如果 `max_loop` 为 -1，则不进行循环尝试。
 *
 * @param dirname 存储转储文件的目录名。
 * @param filename 要生成的转储文件的基础文件名。
 * @param max_loop 最大循环次数，用于生成唯一的文件名，默认为 -1。
 * @return 生成的用于转储文件的完整路径名。
 */
std::string get_pathname_for_dump(const char *dirname, const char *filename, int max_loop = -1);
inline std::string get_pathname_for_dump(const std::string &dirname, const std::string &filename, int max_loop = -1)
{
    return get_pathname_for_dump(dirname.c_str(), filename.c_str(), max_loop);
}
std::string get_pathname_for_dump(const char *pathname, int max_loop = -1);
inline std::string get_pathname_for_dump(const std::string &pathname, int max_loop = -1)
{
    return get_pathname_for_dump(pathname.c_str(), max_loop);
}

/**
 * @brief 清理指定目录下过期的文件和子目录。
 *
 * 该函数会扫描指定目录下的所有文件和子目录，删除超过指定超时时间的文件和子目录。
 * 超时时间以秒为单位，通过 `timeout` 参数指定。
 *
 * @param dirname 要清理的目录路径。
 * @param timeout 文件的超时时间，单位为秒。
 * @return bool 清理操作是否成功。
 */
bool clear_expired_file_in_dir(std::string dirname, int64_t timeout);

// flock
int flock(int fd, bool exclusive = true);
int try_flock(int fd, bool exclusive = true);
bool funlock(int fd);

class flocker
{
public:
    flocker();
    ~flocker();

public:
    inline int lock(const std::string &pathname, bool exclusive = true)
    {
        return __lock(pathname, exclusive, false);
    }
    inline int try_lock(const std::string &pathname, bool exclusive = true)
    {
        return __lock(pathname, exclusive, true);
    }
    int unlock();

private:
    int __lock(const std::string &pathname, bool exclusive, bool try_mode);
    std::string pathname_;
    int fd_{-1};
};

/* io ############################################################### */
static const int var_io_max_timeout = (3600 * 24 * 365 * 10);

// 是否可读/可写
int timed_read_write_wait_millisecond(int fd, int read_wait_timeout, int *readable, int *writeable);
int timed_read_write_wait(int fd, int read_wait_timeout, int *readable, int *writeable);
// 带超时, 是否可读
int timed_read_wait_millisecond(int fd, int wait_timeout);
int timed_read_wait(int fd, int wait_timeout);
// 超时读
int timed_read(int fd, void *buf, int size, int wait_timeout);
// 带超时, 是否可写
int timed_write_wait_millisecond(int fd, int wait_timeout);
int timed_write_wait(int fd, int wait_timeout);
// 超时写
int timed_write(int fd, const void *buf, int size, int wait_timeout);
// 是否可读写
int rwable(int fd);
// 是否可读
int readable(int fd);
// 是否可写
int writeable(int fd);
// 设置fd阻塞
int nonblocking(int fd, bool tf = true);
#ifdef _WIN64
int close(HANDLE fd);
#else  // _WIN64
int close(int fd);
#endif // _WIN64
// close_on_exec
int close_on_exec(int fd, bool tf = true);
// 获取真实可读字节数
int get_readable_count(int fd);
#ifdef __linux__
// 跨父子进程接收 fd
int recv_fd(int fd);
// 跨父子进程发送 fd
int send_fd(int fd, int sendfd);
#endif // __linux__

/* socket ########################################################### */
static const char var_tcp_listen_type_inet = 'i';
static const char var_tcp_listen_type_unix = 'u';
static const char var_tcp_listen_type_fifo = 'f';
// 举例 netpath:
// 192.168.1.1:25
// /somepath/someppp/123_domain_socket_path
// 0:25;127.0.0.1:46;./somepath/123;/home/xxx/111;0:8899
/**
 * @brief 初始化Windows Sockets DLL。
 *
 * 该函数用于在Windows平台上初始化Windows Sockets DLL，使应用程序能够使用Windows Sockets API。
 *
 * @return int 成功时返回0，失败时返回非零错误码。
 */
int WSAStartup();

/**
 * @brief 关闭指定的套接字描述符。
 *
 * 该函数用于关闭指定的套接字描述符，释放相关的系统资源。
 *
 * @param fd 要关闭的套接字描述符。
 * @return int 成功时返回0，失败时返回非零错误码。
 */
int close_socket(int fd);

/**
 * @brief 接受一个Unix域套接字的连接请求。
 *
 * 该函数用于接受一个Unix域套接字的连接请求，并返回一个新的套接字描述符用于与客户端通信。
 *
 * @param fd 监听的Unix域套接字描述符。
 * @return int 成功时返回新的套接字描述符，失败时返回-1。
 */
int unix_accept(int fd);

/**
 * @brief 接受一个Internet域套接字的连接请求。
 *
 * 该函数用于接受一个Internet域套接字的连接请求，并返回一个新的套接字描述符用于与客户端通信。
 *
 * @param fd 监听的Internet域套接字描述符。
 * @return int 成功时返回新的套接字描述符，失败时返回-1。
 */
int inet_accept(int fd);

/**
 * @brief 根据指定的套接字类型接受连接请求。
 *
 * 该函数根据指定的套接字类型（Unix域或Internet域）接受连接请求，并返回一个新的套接字描述符用于与客户端通信。
 *
 * @param fd 监听的套接字描述符。
 * @param type 套接字类型，用于指定是Unix域还是Internet域。
 * @return int 成功时返回新的套接字描述符，失败时返回-1。
 */
int socket_accept(int fd, int type);

/**
 * @brief 创建并监听一个Unix域套接字。
 *
 * 该函数用于创建一个Unix域套接字，并将其绑定到指定的地址，开始监听连接请求。
 *
 * @param addr 要绑定的Unix域套接字地址。
 * @param backlog 监听队列的最大长度，默认为128。
 * @return int 成功时返回监听的套接字描述符，失败时返回-1。
 */
int unix_listen(char *addr, int backlog = 128);

/**
 * @brief 创建并监听一个Internet域套接字。
 *
 * 该函数用于创建一个Internet域套接字，并将其绑定到指定的IP地址和端口，开始监听连接请求。
 *
 * @param sip 要绑定的IP地址。
 * @param port 要绑定的端口号。
 * @param backlog 监听队列的最大长度，默认为128。
 * @return int 成功时返回监听的套接字描述符，失败时返回-1。
 */
int inet_listen(const char *sip, int port, int backlog = 128);

/**
 * @brief 创建并监听一个命名管道（FIFO）。
 *
 * 该函数用于创建一个命名管道（FIFO），并开始监听连接请求。
 *
 * @param path 命名管道的路径。
 * @return int 成功时返回监听的文件描述符，失败时返回-1。
 */
int fifo_listen(const char *path);

/**
 * @brief 根据网络路径创建并监听套接字。
 *
 * 该函数根据指定的网络路径（如IP地址和端口、Unix域套接字路径等）创建并监听套接字。
 *
 * @param netpath 网络路径字符串。
 * @param backlog 监听队列的最大长度，默认为128。
 * @param type 可选参数，用于返回监听的套接字类型。
 * @return int 成功时返回监听的套接字描述符，失败时返回-1。
 */
int netpath_listen(const char *netpath, int backlog = 128, int *type = nullptr);

/**
 * @brief 连接到指定的Unix域套接字。
 *
 * 该函数用于连接到指定的Unix域套接字，并在指定的超时时间内等待连接完成。
 *
 * @param addr 要连接的Unix域套接字地址。
 * @param timeout 连接超时时间（毫秒）。
 * @return int 成功时返回连接的套接字描述符，失败时返回-1。
 */
int unix_connect(const char *addr, int timeout);

/**
 * @brief 连接到指定的Internet域套接字。
 *
 * 该函数用于连接到指定的Internet域套接字（IP地址和端口），并在指定的超时时间内等待连接完成。
 *
 * @param dip 要连接的目标IP地址。
 * @param port 要连接的目标端口号。
 * @param timeout 连接超时时间（毫秒）。
 * @return int 成功时返回连接的套接字描述符，失败时返回-1。
 */
int inet_connect(const char *dip, int port, int timeout);

/**
 * @brief 连接到指定主机的指定端口。
 *
 * 该函数用于连接到指定主机的指定端口，会自动解析主机名，并在指定的超时时间内等待连接完成。
 *
 * @param host 要连接的主机名或IP地址。
 * @param port 要连接的目标端口号。
 * @param timeout 连接超时时间（毫秒）。
 * @return int 成功时返回连接的套接字描述符，失败时返回-1。
 */
int host_connect(const char *host, int port, int timeout);

/**
 * @brief 根据网络路径连接到目标套接字。
 *
 * 该函数根据指定的网络路径（如IP地址和端口、Unix域套接字路径等）连接到目标套接字，并在指定的超时时间内等待连接完成。
 *
 * @param netpath 网络路径字符串。
 * @param timeout 连接超时时间（毫秒）。
 * @return int 成功时返回连接的套接字描述符，失败时返回-1。
 */
int netpath_connect(const char *netpath, int timeout);

/**
 * @brief 获取与指定套接字连接的对端地址和端口。
 *
 * 该函数用于获取与指定套接字连接的对端的IP地址和端口号。
 *
 * @param sockfd 已连接的套接字描述符。
 * @param host 用于存储对端IP地址的指针。
 * @param port 用于存储对端端口号的指针。
 * @return int 成功时返回0，失败时返回非零错误码。
 */

int get_peername(int sockfd, int *host, int *port);

/* dns ############################################################## */
// 获取域名/主机名的ip地址, 存储到addrs
int get_hostaddr(const char *host, std::vector<std::string> &addrs);
inline int get_hostaddr(const std::string &host, std::vector<std::string> &addrs)
{
    return get_hostaddr(host.c_str(), addrs);
}
// 获取本机ip地址
int get_localaddr(std::vector<std::string> &addrs);
// ip 地址转 int
int get_ipint(const char *ipstr);
inline int get_ipint(const std::string &ipstr)
{
    return get_ipint(ipstr.c_str());
}
// 获得ip地址的网段地址
int get_network(int ip, int masklen);
// 获得ip地址的掩码
int get_netmask(int masklen);
// 获得ip地址的广播地址
int get_broadcast(int ip, int masklen);
// 指定掩码, 最小的ip
int get_ip_min(int ip, int masklen);
// 指定掩码, 最大的ip
int get_ip_max(int ip, int masklen);
// 是不是 ip
bool is_ip(const char *ip);
inline bool is_ip(const std::string &ip)
{
    return is_ip(ip.c_str());
}
// 是否保留地址
int is_intranet(int ip);
int is_intranet2(const char *ip);
// ip 转字符串
char *get_ipstring(int ip, char *ipstr);
std::string get_ipstring(int ip);

/* time ############################################################ */
static const int64_t var_use_current_time = -20250428001;
static const int64_t var_invalid_time = -202508221140;
#ifdef _WIN64
static const int64_t var_max_millisecond_duration(3600LL * 24 * 365 * 10 * 1000);
#else  // _WIN64
static const int64_t var_max_millisecond_duration(3600L * 24 * 365 * 10 * 1000);
#endif // _WIN64
struct timeofday
{
    int64_t tv_sec;  /* seconds */
    int64_t tv_usec; /* microseconds */
};
// MSVC 没类似函数
struct timeofday gettimeofday();
// 毫秒
int64_t millisecond();
// 睡眠毫秒
void sleep_millisecond(int64_t delay);
// 睡眠直到
int64_t millisecond_to(int64_t stamp);
// 秒
int64_t second();
// 睡眠
void sleep(int64_t delay);
// 微秒
int64_t microsecond();

// 获取 week day 的简称, 从 0 开始
// {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const char *get_day_abbr_of_week(int day);
// 获 月份的简称, 从 0 开始
// const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
const char *get_month_abbr(int month);

// http time
std::string rfc7231_time(int64_t t);
inline std::string rfc7231_time()
{
    return rfc7231_time(var_use_current_time);
}
ZCC_DEPRECATED inline std::string rfc1123_time(int64_t t) { return rfc7231_time(t); }
ZCC_DEPRECATED inline std::string rfc1123_time() { return rfc7231_time(); }
inline std::string http_time(int64_t t = 0) { return rfc7231_time(t); }

// mime time
std::string rfc822_time(int64_t t);
inline std::string rfc822_time() { return rfc822_time(var_use_current_time); }
inline std::string mail_time(int64_t t) { return rfc822_time(t); }
inline std::string mail_time() { return rfc822_time(); }

// 形如 xxxx-xx-xx xx:xx
std::string simple_date_time(int64_t t);
inline std::string simple_date_time() { return simple_date_time(var_use_current_time); }

// 形如 xxxx-xx-xx xx:xx:xx
std::string simple_date_time_with_second(int64_t t);
inline std::string simple_date_time_with_second() { return simple_date_time_with_second(var_use_current_time); }

// 类似GCC标准C库的 time_t timegm(struct tm *tm); windows平台没有
int64_t timegm(struct tm *tm);

/**
 * 解析符合ISO 8601:2004标准的时间字符串
 * @param s 时间字符串
 * @return 对应的Unix时间戳，解析失败返回var_invalid_time
 */
//  102200
//  1022
//  10
//  -2200
//  --00
//  102200Z
//  102200-0800
int64_t iso8601_2004_time_from_time(const std::string &s);

/**
 * 解析符合ISO 8601:2004标准的日期时间字符串
 * @param s 日期时间字符串
 * @param day_is_preferred 当只有年份时是否优先解析为年
 * @return 对应的Unix时间戳，解析失败返回var_invalid_time
 */
//  19961022T140000
//  --1022T1400
//  ---22T14
//  19961022T140000
//  19961022T140000Z
//  19961022T140000-05
//  19961022T140000-0500
//  19961022T140000
//  --1022T1400
//  ---22T14
//  19850412
//  1985-04
//  1985          ### 如果day_is_preferred==true, 这个就应该解析为年
//  --0412
//  ---12
//  T102200
//  T1022
//  T10
//  T-2200
//  T--00
//  T102200Z
//  T102200-0800
int64_t iso8601_2004_time_from_date(const std::string &s, bool day_is_preferred);

/* zcc end ######################################################### */
zcc_namespace_end;
#pragma pack(pop)
#endif /* __cplusplus */

#endif // ZCC_LIB_INCLUDE_STDLIB___

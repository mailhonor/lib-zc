<A name="readme_md" id="readme_md"></A>

[C版本](./log.md)

## 日志输出, [LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md)

[LIB-ZC](https://gitee.com/linuxmail/lib-zc#readme_md) 支持通用日志机制

命名空间  zcc::logger

## 日志输出

### 日志级别和全局变量
```c++
enum level
{
    verbose = 1,
    debug,
    info,
    warning,
    error,
    fatal,
    error_and_exit,
};

// fatal错误是否抛出异常, 方便gdb
extern bool var_fatal_catch;
extern bool var_debug_enable;
extern bool var_verbose_enable;
// 是否禁用输出
extern bool var_output_disable;
```

### 输出函数

```c++
void vlog_output(const char *source_fn, uint64_t line_number, level ll, const char *fmt, va_list ap);
void log_output(const char *source_fn, uint64_t line_number, level ll, const char *fmt, ...);
typedef void (*output_handler_type)(const char *source_fn, uint64_t line_number, level ll, const char *fmt, va_list ap);
// 默认输出函数, 可覆盖此函数
extern output_handler_type var_output_handler;
```

### 使用 syslog

```c++
// syslog
void use_syslog_by_config(const char *attr /* facility[,identity */);
void use_syslog(const char *identity, int facility);
inline void use_syslog(const std::string &identity, int facility)
{
    use_syslog(identity.c_str(), facility);
}
void use_syslog(const char *identity, const char *facility);
inline void use_syslog(const std::string &identity, const std::string &facility)
{
    use_syslog(identity.c_str(), facility.c_str());
}
int get_facility(const char *facility);
inline int get_facility(const std::string &facility)
{
    return get_facility(facility.c_str());
}
```

### 宏, 带文件和行数

```c++
#define zcc_verbose(...) (zcc::logger::var_verbose_enable ? zcc::logger::log_output(ZCC__FILE__, __LINE__, zcc::logger::verbose, __VA_ARGS__) : (void)0);
#define zcc_debug(...) (zcc::logger::var_debug_enable ? zcc::logger::log_output(ZCC__FILE__, __LINE__, zcc::logger::debug, __VA_ARGS__) : (void)0);
#define zcc_info(...) zcc::logger::log_output(ZCC__FILE__, __LINE__, zcc::logger::info, __VA_ARGS__)
#define zcc_warning(...) zcc::logger::log_output(ZCC__FILE__, __LINE__, zcc::logger::warning, __VA_ARGS__)
#define zcc_error(...) zcc::logger::log_output(ZCC__FILE__, __LINE__, zcc::logger::error, __VA_ARGS__)
#define zcc_fatal(...) zcc::logger::log_output(ZCC__FILE__, __LINE__, zcc::logger::fatal, __VA_ARGS__)
#define zcc_error_and_exit(...) zcc::logger::log_output(ZCC__FILE__, __LINE__, zcc::logger::error_and_exit, __VA_ARGS__)
#define zcc_exit(status) zcc::exit(status)
```
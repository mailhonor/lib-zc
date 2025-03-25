/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2015-10-12
 * ================================
 */

#include "zcc/zcc_stdlib.h"
#if __linux__
#include <assert.h>
#include <syslog.h>
#endif // __linux__

zcc_namespace_begin;
zcc_general_namespace_begin(logger);

// 控制是否开启致命错误捕获功能的标志。当该标志为 true 时，发生致命错误会触发断言失败以终止程序。
bool var_fatal_catch = false;
// 控制是否开启调试日志输出的标志。当该标志为 true 时，可能会输出更多调试相关的日志信息。
bool var_debug_enable = false;
// 控制是否开启详细日志输出的标志。当该标志为 true 时，会输出更详细的日志内容。
bool var_verbose_enable = false;
// 控制是否禁用日志输出的标志。当该标志为 true 时，所有日志输出都会被禁止。
bool var_output_disable = false;

/**
 * @brief 默认的日志输出处理函数，将日志信息输出到标准错误流。
 * 
 * 该函数会将传入的格式化字符串和日志相关信息（如源文件名、行号）组合成一个新的格式化字符串，
 * 然后使用 `std::vfprintf` 函数将最终的日志信息输出到标准错误流 `stderr`。
 * 
 * @param source_fn 日志输出所在的源文件名。
 * @param line_number 日志输出所在的源文件行号。
 * @param ll 日志级别，在当前代码中未实际使用，但保留以保持函数接口的一致性。
 * @param fmt 格式化字符串，用于指定日志信息的输出格式。
 * @param ap 可变参数列表，包含了格式化字符串中需要的参数。
 */
static void output_handler_default(const char *source_fn, uint64_t line_number, level ll, const char *fmt, va_list ap)
{
    // 定义一个缓冲区，用于存储组合后的格式化字符串，长度为 1024 加上字符串结束符
    char fmt_buf[1024 + 1];
    // 将原始的格式化字符串和源文件名、行号组合成一个新的格式化字符串，存储到 fmt_buf 中
    std::snprintf(fmt_buf, 1024, "%s [%s:%zu]\n", fmt, source_fn, line_number);
    // 将组合后的格式化字符串和可变参数列表中的参数进行格式化，并输出到标准错误流 stderr
    std::vfprintf(stderr, fmt_buf, ap);
}

output_handler_type var_output_handler = output_handler_default;

static int fatal_times = 0;
/**
 * @brief 可变参数日志输出函数，根据日志级别处理并输出日志信息。
 * 
 * 该函数会根据传入的日志级别处理日志输出，对于致命错误（`fatal`），仅输出一次并可能触发程序终止；
 * 对于其他日志级别，直接调用当前设置的日志输出处理函数进行输出。若日志级别为 `error_and_exit`，
 * 则在输出日志后终止程序。
 * 
 * @param source_fn 日志输出所在的源文件名。
 * @param line_number 日志输出所在的源文件行号。
 * @param ll 日志级别。
 * @param fmt 格式化字符串，用于指定日志信息的输出格式。
 * @param ap 可变参数列表，包含了格式化字符串中需要的参数。
 */
void vlog_output(const char *source_fn, uint64_t line_number, level ll, const char *fmt, va_list ap)
{
    // 检查日志级别是否为致命错误
    if (ll == fatal)
    {
        // 统计致命错误发生的次数，若已经发生过致命错误，则直接返回，避免重复处理
        if (fatal_times++)
        {
            return;
        }
        // 使用默认的日志输出处理函数输出致命错误信息
        output_handler_default(source_fn, line_number, ll, fmt, ap);
        // 若日志输出未禁用，且当前设置了自定义的日志输出处理函数，并且该函数不是默认处理函数
        // 则调用自定义的日志输出处理函数再次输出致命错误信息
        if ((!var_output_disable) && var_output_handler && (var_output_handler != output_handler_default))
        {
            var_output_handler(source_fn, line_number, ll, fmt, ap);
        }
#ifdef __linux__
        // 若开启了致命错误捕获标志，则触发断言失败，终止程序
        if (var_fatal_catch)
        {
            assert(0);
        }
#endif // __linux__
        // 发生致命错误后，终止程序并返回状态码 1
        exit(1);
    }
    else
    {
        // 若日志输出未禁用，且当前设置了日志输出处理函数
        // 则调用该函数输出非致命错误的日志信息
        if ((!var_output_disable) && var_output_handler)
        {
            var_output_handler(source_fn, line_number, ll, fmt, ap);
        }
        // 若日志级别为 error_and_exit，则在输出日志后终止程序并返回状态码 1
        if (ll == error_and_exit)
        {
            exit(1);
        }
    }
}

/**
 * @brief 带有可变参数的日志输出函数，用于封装 `vlog_output` 函数。
 * 
 * 该函数接收日志输出所需的源文件名、行号、日志级别和格式化字符串等信息，
 * 并通过可变参数列表处理格式化字符串中的参数。若日志输出被禁用（`var_output_disable` 为 `true`），
 * 则直接返回，不进行日志输出操作。
 * 
 * @param source_fn 日志输出所在的源文件名。
 * @param line_number 日志输出所在的源文件行号。
 * @param ll 日志级别。
 * @param fmt 格式化字符串，用于指定日志信息的输出格式。
 * @param ... 可变参数列表，包含了格式化字符串中需要的参数。
 */
void log_output(const char *source_fn, uint64_t line_number, level ll, const char *fmt, ...)
{
    // 检查日志输出是否被禁用，如果禁用则直接返回，不进行后续操作
    if (var_output_disable)
    {
        return;
    }
    // 定义一个可变参数列表对象
    va_list ap;
    // 初始化可变参数列表，使其指向可变参数的起始位置
    va_start(ap, fmt);
    // 调用 vlog_output 函数，传递源文件名、行号、日志级别、格式化字符串和可变参数列表
    vlog_output(source_fn, line_number, ll, fmt, ap);
    // 清理可变参数列表，释放相关资源
    va_end(ap);
}

// class LOG
/**
 * @brief LOG 类的构造函数，用于初始化 LOG 对象。
 * 
 * 该构造函数接收日志级别、源文件路径名和行号作为参数，
 * 并将这些参数赋值给 LOG 对象的成员变量。
 * 
 * @param ll 日志级别，指定当前日志的严重程度。
 * @param sourcePathname 源文件的路径名，标识日志输出所在的文件。
 * @param lineNumber 源文件中的行号，标识日志输出所在的具体行。
 */
LOG::LOG(level ll, const char *sourcePathname, uint64_t lineNumber)
{
    // 保存源文件路径名到成员变量
    sourcePathname_ = sourcePathname;
    // 保存源文件行号到成员变量
    lineNumber_ = lineNumber;
    // 保存日志级别到成员变量
    level_ = ll;
}

/**
 * @brief LOG 类的析构函数，用于在对象销毁时处理日志输出。
 * 
 * 该析构函数会检查日志缓冲区 `buf_` 是否为空。
 * 如果不为空，则调用 `log_output` 函数将缓冲区中的日志信息输出。
 */
LOG::~LOG()
{
    // 检查日志缓冲区是否为空
    if (buf_.empty())
    {
        // 若为空，则不进行日志输出，直接返回
        return;
    }
    // 调用 log_output 函数输出日志信息
    log_output(sourcePathname_, lineNumber_, level_, "%s", buf_.c_str());
}

// syslog

/**
 * @brief 根据配置字符串使用系统日志功能。
 * 
 * 该函数接收一个配置字符串，该字符串格式为 `facility[,identity]`，
 * 其中 `facility` 是系统日志的设备类型，`identity` 是可选的日志标识。
 * 函数会根据配置字符串的内容调用 `use_syslog` 函数来设置系统日志。
 * 
 * @param attr 配置字符串，格式为 `facility[,identity]`。
 */
void use_syslog_by_config(const char *attr /* facility[,identity */)
{
    // 检查配置字符串是否为空，如果为空则直接返回，不进行后续操作
    if (empty(attr))
    {
        return;
    }
    // 将配置字符串按逗号分割成多个子字符串，存储在向量 ks 中
    auto ks = split(attr, ',');
    // 如果分割后的子字符串数量为 1，说明只提供了 facility，没有提供 identity
    if (ks.size() == 1)
    {
        // 获取当前命令的路径名
        std::string cmdname = get_cmd_pathname();
        // 如果命令路径名为空，则使用 nullptr 作为 identity 调用 use_syslog 函数
        if (cmdname.empty())
        {
            use_syslog(nullptr, ks[0]);
        }
        else
        {
            // 获取命令路径名的 C 风格字符串
            const char *ps = cmdname.c_str();
            // 查找路径名中最后一个斜杠的位置
            const char *p = strrchr(ps, '/');
            if (p)
            {
                // 如果找到斜杠，将指针移动到斜杠后面的位置，即命令名的起始位置
                p++;
            }
            else
            {
                // 如果没有找到斜杠，说明路径名就是命令名
                p = ps;
            }
            // 使用命令名作为 identity 调用 use_syslog 函数
            use_syslog(p, ks[0]);
        }
    }
    // 如果分割后的子字符串数量为 2，说明同时提供了 facility 和 identity
    else if (ks.size() == 2)
    {
        // 使用提供的 identity 和 facility 调用 use_syslog 函数
        use_syslog(ks[1], ks[0]);
    }
}

/**
 * @brief 系统日志输出处理函数，将日志信息输出到系统日志。
 * 
 * 该函数在 Linux 系统下工作，会将传入的格式化字符串和日志相关信息（如源文件名、行号）
 * 组合成一个新的格式化字符串，然后使用 `vsyslog` 函数将最终的日志信息输出到系统日志。
 * 
 * @param source_fn 日志输出所在的源文件名。
 * @param line_number 日志输出所在的源文件行号。
 * @param ll 日志级别，在当前函数中未对其进行特别处理，统一使用 `LOG_INFO` 级别输出。
 * @param fmt 格式化字符串，用于指定日志信息的输出格式。
 * @param ap 可变参数列表，包含了格式化字符串中需要的参数。
 */
static void output_handler_syslog(const char *source_fn, uint64_t line_number, level ll, const char *fmt, va_list ap)
{
#ifdef __linux__
    // 定义一个缓冲区，用于存储组合后的格式化字符串，长度为 1024 加上字符串结束符
    char fmt_buf[1024 + 1];
    // 将原始的格式化字符串和源文件名、行号组合成一个新的格式化字符串，存储到 fmt_buf 中
    std::snprintf(fmt_buf, 1024, "%s [%s:%zu]\n", fmt, source_fn, line_number);
    // 使用 vsyslog 函数将组合后的格式化字符串和可变参数列表中的参数输出到系统日志，日志级别为 LOG_INFO
    vsyslog(LOG_INFO, fmt_buf, ap);
#endif // __linux__
}

/**
 * @brief 使用系统日志功能，不指定日志标识。
 * 
 * 该函数是 `use_syslog` 函数的重载版本，允许用户仅指定系统日志的设备类型，
 * 而不指定日志标识。函数内部调用另一个 `use_syslog` 函数，将日志标识设为 `nullptr`。
 * 
 * @param facility 系统日志的设备类型，用于指定日志的来源和分类。
 */
void use_syslog(int facility)
{
    // 调用另一个 use_syslog 函数，将日志标识设为 nullptr，仅传递设备类型
    use_syslog(nullptr, facility);
}

static std::string var_syslog_identity;
/**
 * @brief 使用系统日志功能，设置日志标识和设备类型。
 * 
 * 该函数用于在 Linux 系统下配置系统日志功能。它接收日志标识和系统日志设备类型作为参数，
 * 根据传入的标识设置全局的系统日志标识，并调用 `openlog` 函数打开系统日志连接。
 * 同时，将日志输出处理函数设置为系统日志输出处理函数 `output_handler_syslog`。
 * 
 * @param identity 系统日志的标识，用于在日志条目中标识日志来源。可以为 `nullptr`。
 * @param facility 系统日志的设备类型，指定日志的来源和分类。
 */
void use_syslog(const char *identity, int facility)
{
#ifdef __linux__
    // 如果传入的日志标识不为空，则将其赋值给全局的系统日志标识变量
    if (identity)
    {
        var_syslog_identity = identity;
    }
    // 若全局的系统日志标识为空，则将传入的标识指针置为 nullptr
    if (var_syslog_identity.empty())
    {
        identity = nullptr;
    }
    else
    {
        // 若全局的系统日志标识不为空，则将传入的标识指针指向该全局变量的 C 风格字符串
        identity = var_syslog_identity.c_str();
    }

    // 打开系统日志连接，设置日志标识、选项（不延迟打开且包含进程 ID）和设备类型
    openlog(identity, LOG_NDELAY | LOG_PID, facility);
    // 将日志输出处理函数设置为系统日志输出处理函数
    var_output_handler = output_handler_syslog;
#endif // __linux__
}

/**
 * @brief 根据传入的日志标识和系统日志设备类型字符串使用系统日志功能。
 * 
 * 该函数接收日志标识和系统日志设备类型的字符串表示作为参数，
 * 首先调用 `get_facility` 函数将设备类型字符串转换为对应的整数常量，
 * 然后调用另一个重载的 `use_syslog` 函数，传入日志标识和转换后的设备类型常量，
 * 以完成系统日志功能的配置。
 * 
 * @param identity 系统日志的标识，用于在日志条目中标识日志来源。可以为 `nullptr`。
 * @param facility 系统日志的设备类型的字符串表示，例如 "LOG_SYSLOG" 等。
 */
void use_syslog(const char *identity, const char *facility)
{
    // 调用 get_facility 函数，将设备类型字符串转换为对应的整数常量
    int f = get_facility(facility);
    // 调用另一个重载的 use_syslog 函数，传入日志标识和转换后的设备类型常量
    use_syslog(identity, f);
}

/**
 * @brief 将系统日志设备类型的字符串表示转换为对应的整数常量。
 * 
 * 该函数接收一个系统日志设备类型的字符串，在 Linux 系统下，
 * 会尝试将其转换为对应的 `syslog.h` 中定义的整数常量。
 * 如果传入的字符串以 "LOG_" 开头，会先去掉这个前缀。
 * 若未匹配到任何已知的设备类型，默认返回 `LOG_SYSLOG`。
 * 在非 Linux 系统下，直接返回 -1。
 * 
 * @param facility 系统日志设备类型的字符串表示，例如 "KERN"、"USER" 等。
 * @return int 转换后的系统日志设备类型的整数常量，非 Linux 系统下返回 -1。
 */
int get_facility(const char *facility)
{
#ifdef __linux__
    // 初始化系统日志设备类型为 LOG_SYSLOG，作为默认值
    int fa = LOG_SYSLOG;
    // 检查传入的字符串是否以 "LOG_" 开头
    if (!strncasecmp(facility, "LOG_", 4))
    {
        // 若以 "LOG_" 开头，将指针向后移动 4 位，跳过 "LOG_" 前缀
        facility += 4;
    }
    /**
     * @brief 宏定义，用于比较字符串并设置系统日志设备类型。
     * 
     * 该宏会将传入的字符串 `S` 与 `facility` 进行不区分大小写的比较，
     * 如果相等，则将 `fa` 设置为 `LOG_##S` 对应的系统日志设备类型常量。
     */
#define ___LOG_S_I(S)              \
    if (!strcasecmp(#S, facility)) \
    {                              \
        fa = LOG_##S;              \
    }
    // 依次比较不同的系统日志设备类型字符串，并设置对应的整数常量
    ___LOG_S_I(KERN);
    ___LOG_S_I(USER);
    ___LOG_S_I(MAIL);
    ___LOG_S_I(DAEMON);
    ___LOG_S_I(AUTH);
    ___LOG_S_I(SYSLOG);
    ___LOG_S_I(LPR);
    ___LOG_S_I(NEWS);
    ___LOG_S_I(UUCP);
    ___LOG_S_I(CRON);
    ___LOG_S_I(AUTHPRIV);
    ___LOG_S_I(FTP);
    ___LOG_S_I(LOCAL0);
    ___LOG_S_I(LOCAL1);
    ___LOG_S_I(LOCAL2);
    ___LOG_S_I(LOCAL3);
    ___LOG_S_I(LOCAL4);
    ___LOG_S_I(LOCAL5);
    ___LOG_S_I(LOCAL6);
    ___LOG_S_I(LOCAL7);
#undef ___LOG_S_I

    // 返回转换后的系统日志设备类型的整数常量
    return fa;
#else  // __linux__
    // 非 Linux 系统下，返回 -1 表示不支持
    return -1;
#endif // __linux__
}

zcc_general_namespace_end(logger);
zcc_namespace_end;

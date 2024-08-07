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

#if defined(_MSC_VER)
#ifndef ZCC_MSVC
#define ZCC_MSVC
#endif // ZCC_MSVC
#endif // _MSC_VER

#ifdef _WIN64
#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0A00
#endif // _WIN64

#ifdef _WIN64
#define _AMD64_
#endif // _WIN64

#ifdef ZCC_DEV_MODE___
#ifdef ZCC_MSVC
#pragma warning(disable : 4267 4244 4996)
#endif // ZCC_MSVC
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#endif // ZCC_DEV_MODE___

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
#endif

#ifndef ZCC__FILE__
#define ZCC__FILE__ __FILE__
#endif

#define ZCC_STR_N_CASE_EQ(a, b, n) ((zcc::toupper((int)a[0]) == zcc::toupper((int)b[0])) && (!zcc::strncasecmp(a, b, n)))
#define ZCC_STR_CASE_EQ(a, b) ((zcc::toupper(a[0]) == zcc::toupper(b[0])) && (!zcc::strcasecmp(a, b)))
#define ZCC_STR_N_EQ(a, b, n) ((a[0] == b[0]) && (!std::strncmp(a, b, n)))
#define ZCC_STR_EQ(a, b) ((a[0] == b[0]) && (!std::strcmp(a, b)))
#define ZCC_VOID_PTR_ONE ((void *)(int64_t) - 1)
#define ZCC_NUMBER_TO_PTR(n) ((void *)(int64_t)(n))
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
#include <string.h>
#include <cstring>
#include <cstdarg>
#include <string>
#include <vector>
#include <list>
#include <map>
#ifdef _WIN64
#include <WinSock2.h>
#include <windows.h>
#endif // _WIN64

#ifdef ZCC_MSVC
#endif // ZCC_MSVC

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
class global_init
{
public:
    inline global_init(void (*_init_fn)()) { _init_fn(); }
    inline global_init(void (*_init_fn)(void *ctx), void *ctx) { _init_fn(ctx); }
    inline global_init(void (*_init_fn)(int i), int i) { _init_fn(i); }
    inline ~global_init() {}

public:
    int unused;
};
class global_fini
{
public:
    inline global_fini(void (*_fini_fn)()) { _fini_fn_ = _fini_fn; }
    inline ~global_fini() {}

public:
    void (*_fini_fn_)();
};

/* log ############################################################## */
zcc_general_namespace_begin(logger);
enum level
{
    verbose = 1,
    debug,
    info,
    output,
    warning,
    error,
    fatal,
    error_and_exit,
};

extern bool var_fatal_catch;
extern bool var_debug_enable;
extern bool var_verbose_enable;
extern bool var_output_disable;
void vlog_output(const char *source_fn, uint64_t line_number, level ll, const char *fmt, va_list ap);
void log_output(const char *source_fn, uint64_t line_number, level ll, const char *fmt, ...);
typedef void (*output_handler_type)(const char *source_fn, uint64_t line_number, level ll, const char *fmt, va_list ap);
extern output_handler_type var_output_handler;
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
zcc_general_namespace_end(logger);
#define zcc_verbose(...) (zcc::logger::var_verbose_enable ? zcc::logger::log_output(ZCC__FILE__, __LINE__, zcc::logger::verbose, __VA_ARGS__) : (void)0);
#define zcc_debug(...) (zcc::logger::var_debug_enable ? zcc::logger::log_output(ZCC__FILE__, __LINE__, zcc::logger::debug, __VA_ARGS__) : (void)0);
#define zcc_info(...) zcc::logger::log_output(ZCC__FILE__, __LINE__, zcc::logger::info, __VA_ARGS__)
#define zcc_warning(...) zcc::logger::log_output(ZCC__FILE__, __LINE__, zcc::logger::warning, __VA_ARGS__)
#define zcc_error(...) zcc::logger::log_output(ZCC__FILE__, __LINE__, zcc::logger::error, __VA_ARGS__)
#define zcc_fatal(...) zcc::logger::log_output(ZCC__FILE__, __LINE__, zcc::logger::fatal, __VA_ARGS__)
#define zcc_error_and_exit(...) zcc::logger::log_output(ZCC__FILE__, __LINE__, zcc::logger::error_and_exit, __VA_ARGS__)
#define zcc_output(...) zcc::logger::log_output(ZCC__FILE__, __LINE__, zcc::logger::output, __VA_ARGS__)
#define zcc_exit(status) zcc::exit(status)

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
typedef std::map<std::string, std::string> dict;
extern char var_blank_buffer[];
extern const std::string var_blank_string;
extern const std::list<std::string> var_blank_list;
extern const std::vector<std::string> var_blank_vector;
extern const dict var_blank_map;
extern unsigned const char var_char_xdigitval_vector[];
inline bool is_trimable(int ch) { return std::iscntrl(ch) || (ch == ' '); }
inline int hex_char_to_int(int c) { return var_char_xdigitval_vector[(unsigned char)(c)]; }
inline int isalnum(int c) { return std::isalnum(c); }
inline int isalpha(int c) { return std::isalpha(c); }
inline int iscntrl(int c) { return std::iscntrl(c); }
inline int isdigit(int c) { return std::isdigit(c); }
inline int isgraph(int c) { return std::isgraph(c); }
inline int islower(int c) { return std::islower(c); }
inline int isprint(int c) { return std::isprint(c); }
inline int ispunct(int c) { return std::ispunct(c); }
inline int isspace(int c) { return std::isspace(c); }
inline int isupper(int c) { return std::isupper(c); }
inline int isxdigit(int c) { return std::isxdigit(c); }
inline int isblank(int c) { return std::isblank(c); }
char *trim_left(char *str);
char *trim_right(char *str);
char *trim(char *str);
char *skip_left(const char *str, const char *ignores);
int skip_right(const char *str, int len, const char *ignores);
int skip(const char *line, int len, const char *ignores_left, const char *ignores_right, char **start);
bool str_to_bool(const char *s, bool def = false);
inline bool str_to_bool(const std::string &s, bool def = false)
{
    return str_to_bool(s.c_str(), def);
}
int64_t str_to_second(const char *s, int64_t def);
int64_t str_to_size(const char *s, int64_t def);
inline int tolower(int ch)
{
    return std::tolower(ch);
}
inline int toupper(int ch)
{
    return std::toupper(ch);
}
char *tolower(char *str);
char *toupper(char *str);
char *clear_null(char *data, int64_t size);
std::string &clear_null(std::string &data);
int strcasecmp(const char *a, const char *b);
inline int strncasecmp(const char *a, const char *b, size_t c);
std::string &vsprintf_1024(std::string &str, const char *format, va_list ap);
#ifdef __linux__
std::string __attribute__((format(gnu_printf, 2, 3))) & sprintf_1024(std::string &str, const char *format, ...);
#else  // __linux__
std::string &sprintf_1024(std::string &str, const char *format, ...);
#endif // __linux__
std::string &tolower(std::string &str);
std::string &toupper(std::string &str);
std::string &trim_right(std::string &str, const char *delims = 0);
std::string &trim_line_end_rn(std::string &str);

std::vector<std::string> split(const char *s, int len, const char *delims, bool ignore_empty_token = false);
inline std::vector<std::string> split(const char *s, const char *delims, bool ignore_empty_token = false)
{
    return split(s, -1, delims, ignore_empty_token);
}
inline std::vector<std::string> split(const std::string &s, const char *delims, bool ignore_empty_token = false)
{
    return split(s.c_str(), s.size(), delims, ignore_empty_token);
}

std::vector<std::string> split(const char *s, int len, int delim, bool ignore_empty_token = false);
inline std::vector<std::string> split(const char *s, int delim, bool ignore_empty_token = false)
{
    return split(s, -1, delim, ignore_empty_token);
}
inline std::vector<std::string> split(const std::string &s, int delim, bool ignore_empty_token = false)
{
    return split(s.c_str(), s.size(), delim, ignore_empty_token);
}

inline std::vector<std::string> split_and_ignore_empty_token(const char *s, int len, const char *delims)
{
    return split(s, len, delims, true);
}
inline std::vector<std::string> split_and_ignore_empty_token(const char *s, const char *delims)
{
    return split_and_ignore_empty_token(s, -1, delims);
}
inline std::vector<std::string> split_and_ignore_empty_token(const std::string &s, const char *delims)
{
    return split_and_ignore_empty_token(s.c_str(), s.size(), delims);
}

inline std::vector<std::string> split_and_ignore_empty_token(const char *s, int len, int delim)
{
    return split(s, len, delim, true);
}

inline std::vector<std::string> split_and_ignore_empty_token(const char *s, int delim)
{
    return split_and_ignore_empty_token(s, -1, delim);
}
inline std::vector<std::string> split_and_ignore_empty_token(const std::string &s, int delim)
{
    return split_and_ignore_empty_token(s.c_str(), s.size(), delim);
}

void *no_memmem(const void *l, int64_t l_len, const void *s, int64_t s_len);
inline static void *memmem(const void *l, int64_t l_len, const void *s, int64_t s_len)
{
#ifdef _WIN64
    return no_memmem(l, l_len, s, s_len);
#else  // _WIN64
    return ::memmem(l, l_len, s, s_len);
#endif // _WIN64
}

inline int strcasecmp(const char *a, const char *b)
{
#ifdef _WIN64
    return _stricmp(a, b);
#else  // _WIN64
    return ::strcasecmp(a, b);
#endif // _WIN64
}

inline int strncasecmp(const char *a, const char *b, size_t c)
{
#ifdef _WIN64
    return _strnicmp(a, b, c);
#else  // _WIN64
    return ::strncasecmp(a, b, c);
#endif // _WIN64
}

#ifdef _WIN64
char *strcasestr(const char *haystack, const char *needle);
#else  // _WIN64
inline char *strcasestr(const char *haystack, const char *needle)
{
    return (char *)(void *)::strcasestr(haystack, needle);
}
#endif // _WIN64

void debug_show(const dict &dt);
const char *get_cstring(const dict &dt, const char *key, const char *def_val = "");
const char *get_cstring(const dict &dt, const std::string &key, const char *def_val = "");
std::string get_string(const dict &dt, const char *key, const char *def_val = "");
std::string get_string(const dict &dt, const std::string &key, const char *def_val = "");
const std::string &get_string(const dict &dt, const std::string &key, const std::string &def_val = var_blank_string);
bool get_bool(const dict &dt, const char *key, bool def_val = false);
bool get_bool(const dict &dt, const std::string &key, bool def_val = false);
int get_int(const dict &dt, const char *key, int def_val = -1);
int get_int(const dict &dt, const std::string &key, int def_val = -1);
int64_t get_long(const dict &dt, const char *key, int64_t def_val = -1);
int64_t get_long(const dict &dt, const std::string &key, int64_t def_val = -1);
int64_t get_second(const dict &dt, const char *key, int64_t def_val = -1);
int64_t get_second(const dict &dt, const std::string &key, int64_t def_val = -1);
int64_t get_size(const dict &dt, const char *key, int64_t def_val = -1);
int64_t get_size(const dict &dt, const std::string &key, int64_t def_val = -1);

std::string build_unique_id();
std::string hunman_byte_size(int64_t a);

extern const char *var_default_mime_type;
const char *get_mime_type(const char *pathname, const char *def = nullptr);
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
class config : public std::map<std::string, std::string>
{
public:
    config();
    ~config();
    config &reset();
    config &update(const char *key, const char *val, int vlen = -1);
    config &update(const char *key, const std::string &val);
    config &update(const std::string &key, const std::string &val);
    config &remove(const char *key);
    config &remove(const std::string &key);
    bool load_from_file(const char *pathname);
    bool load_from_file(const std::string &pathname)
    {
        return load_from_file(pathname.c_str());
    }
    config &load_another(config &another);
    config &debug_show();
    std::string *get_value(const char *key);
    std::string *get_value(const std::string &key);
    const char *get_cstring(const char *key, const char *def_val = "");
    const char *get_cstring(const std::string &key, const char *def_val = "");
    std::string get_string(const char *key, const char *def_val = "");
    std::string get_string(const std::string &key, const char *def_val = "");
    const std::string &get_string(const std::string &key, const std::string &def_val);
    bool get_bool(const char *key, bool def_val = false);
    bool get_bool(const std::string &key, bool def_val = false);
    int get_int(const char *key, int def_val = 0);
    int get_int(const std::string &key, int def_val = 0);
    int64_t get_long(const char *key, int64_t def_val = 0);
    int64_t get_long(const std::string &key, int64_t def_val = 0);
    int64_t get_second(const char *key, int64_t def_val = 0);
    int64_t get_second(const std::string &key, int64_t def_val = 0);
    int64_t get_size(const char *key, int64_t def_val = 0);
    int64_t get_size(const std::string &key, int64_t def_val = 0);

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
int get_process_id();
int get_parent_process_id();
int get_thread_id();
inline int getpid()
{
    return get_process_id();
}
inline int getppid()
{
    return get_parent_process_id();
}
inline int gettid()
{
    return get_thread_id();
}
std::string get_cmd_pathname();
std::string get_cmd_name();
#ifdef __linux__
bool quick_setrlimit(int cmd, unsigned long cur_val);
bool set_core_file_size(int megabyte);
bool set_max_mem(int megabyte);
bool set_cgroup_name(const char *name);
int64_t get_MemAvailable();
int get_cpu_core_count();
int chroot_user(const char *root_dir, const char *user_name);
#endif // __linux__
int get_mac_address(std::vector<std::string> &mac_list);

void signal(int signum, void (*handler)(int));
void signal_ignore(int signum);

/* fs ############################################################### */
class mmap_reader
{
public:
    mmap_reader();
    ~mmap_reader();
    int open(const char *pathname);
    inline int open(const std::string &pathname)
    {
        return open(pathname.c_str());
    }
    int close();

protected:
#ifdef _WIN64
    void *fd_{ZCC_VOID_PTR_ONE};
    void *fm_{nullptr};
#else  // _WIN64
    int fd_{-1};
#endif // _WIN64

public:
    int64_t size_{-1};          // 映射后, 长度
    const char *data_{nullptr}; // 映射后, 指针
};
FILE *fopen(const char *pathname, const char *mode);
inline FILE *fopen(const std::string &pathname, const char *mode)
{
    return fopen(pathname.c_str(), mode);
}
#ifdef _WIN64
int64_t getdelim(char **lineptr, int64_t *n, int delim, FILE *stream);
inline int64_t getline(char **lineptr, int64_t *n, FILE *stream)
{
    return getdelim(lineptr, (size_t *)n, '\n', stream);
}

#else  // _WIN64
inline int64_t getdelim(char **lineptr, int64_t *n, int delim, FILE *stream)
{
    return ::getdelim(lineptr, (size_t *)n, delim, stream);
}
inline int64_t getline(char **lineptr, int64_t *n, FILE *stream)
{
    return ::getline(lineptr, (size_t *)n, stream);
}
#endif // _WIN64

std::string realpath(const char *pathname);
inline std::string realpath(const std::string &pathname)
{
    return realpath(pathname.c_str());
}
#ifdef _WIN64
#define zcc_stat struct _stat64i32
int stat(const char *pathname, struct _stat64i32 *statbuf);
#else // _WIN64
#define zcc_stat struct stat
int stat(const char *pathname, struct stat *statbuf);
#endif // _WIN64
int64_t file_get_size(const char *pathname);
inline int64_t file_get_size(const std::string &pathname)
{
    return file_get_size(pathname.c_str());
}
int file_exists(const char *pathname);
inline int file_exists(const std::string &pathname)
{
    return file_exists(pathname.c_str());
}
int file_put_contents(const char *pathname, const void *data, int len);
inline int file_put_contents(const std::string &pathname, const void *data, int len)
{
    return file_put_contents(pathname.c_str(), data, len);
}
inline int file_put_contents(const char *pathname, const std::string &data)
{
    return file_put_contents(pathname, data.c_str(), data.size());
}
inline int file_put_contents(const std::string &pathname, const std::string &data)
{
    return file_put_contents(pathname.c_str(), data.c_str(), data.size());
}
std::string file_get_contents(const char *pathname);
inline std::string file_get_contents(const std::string &pathname)
{
    return file_get_contents(pathname.c_str());
}
int64_t file_get_contents(const char *pathname, std::string &bf);
inline int64_t file_get_contents(const std::string &pathname, std::string &bf)
{
    return file_get_contents(pathname.c_str(), bf);
}
int64_t file_get_contents_sample(const char *pathname, std::string &bf);
std::string file_get_contents_sample(const char *pathname);
int stdin_get_contents(std::string &bf);
std::string stdin_get_contents();
int open(const char *pathname, int flags, int mode);
inline int open(const std::string &pathname, int flags, int mode)
{
    return open(pathname.c_str(), flags, mode);
}
int touch(const char *pathname);
inline int touch(const std::string &pathname)
{
    return touch(pathname.c_str());
}
int mkdir(std::vector<std::string> paths, int mode = 0666);
int mkdir(int mode, const char *path1, ...);
int mkdir(const char *pathname, int mode = 0666);
int rename(const char *oldpath, const char *newpath);
int unlink(const char *pathname);
inline int unlink(const std::string &pathname)
{
    return unlink(pathname.c_str());
}
int link(const char *oldpath, const char *newpath);
int link_force(const char *oldpath, const char *newpath, const char *tmpdir);
int symlink(const char *oldpath, const char *newpath);
int symlink_force(const char *oldpath, const char *newpath, const char *tmpdir);
bool create_shortcut_link(const char *from, const char *to);
inline bool create_shortcut_link(const std::string &from, const std::string &to)
{
    return create_shortcut_link(from.c_str(), to.c_str());
}
int rmdir(const char *pathname, bool recurse_mode = false);
inline int rmdir(const std::string &pathname, bool recurse_mode = false)
{
    return rmdir(pathname.c_str(), recurse_mode);
}
struct dir_item_info
{
    std::string filename;
    bool dir{false};
    bool regular{false};
    bool fifo{false};
    bool link{false};
    bool socket{false};
    bool dev{false};
};

int scandir(const char *dirname, std::vector<dir_item_info> &filenames);
inline int scandir(const std::string &dirname, std::vector<dir_item_info> &filenames)
{
    return scandir(dirname.c_str(), filenames);
}
std::vector<dir_item_info> scandir(const char *dirname);
inline std::vector<dir_item_info> scandir(const std::string &dirname)
{
    return scandir(dirname.c_str());
}

std::string format_filename(const char *filename);
inline std::string format_filename(const std::string &filename)
{
    return format_filename(filename.c_str());
}
std::vector<std::string> find_file_sample(std::vector<const char *> dir_or_file, const char *pathname_match = nullptr);
std::vector<std::string> find_file_sample(const char **dir_or_file, int item_count, const char *pathname_match = nullptr);

/* io ############################################################### */
static const int var_io_max_timeout = (3600 * 24 * 365 * 10);

int timed_read_write_wait_millisecond(int fd, int read_wait_timeout, int *readable, int *writeable);
int timed_read_write_wait(int fd, int read_wait_timeout, int *readable, int *writeable);
int timed_read_wait_millisecond(int fd, int wait_timeout);
int timed_read_wait(int fd, int wait_timeout);
int timed_read(int fd, void *buf, int size, int wait_timeout);
int timed_write_wait_millisecond(int fd, int wait_timeout);
int timed_write_wait(int fd, int wait_timeout);
int timed_write(int fd, const void *buf, int size, int wait_timeout);
int rwable(int fd);
int readable(int fd);
int writeable(int fd);
int nonblocking(int fd, bool tf = true);
#ifdef _WIN64
int close(HANDLE fd);
#else  // _WIN64
int close(int fd);
#endif // _WIN64
int close_on_exec(int fd, bool tf = true);
int get_readable_count(int fd);
#ifdef __linux__
int recv_fd(int fd);
int send_fd(int fd, int sendfd);
#endif // __linux__

/* socket ########################################################### */
static const char var_tcp_listen_type_inet = 'i';
static const char var_tcp_listen_type_unix = 'u';
static const char var_tcp_listen_type_fifo = 'f';
int WSAStartup();
int close_socket(int fd);
int unix_accept(int fd);
int inet_accept(int fd);
int socket_accept(int fd, int type);
int unix_listen(char *addr, int backlog = 128);
int inet_listen(const char *sip, int port, int backlog = 128);
int fifo_listen(const char *path);
int netpath_listen(const char *netpath, int backlog = 128, int *type = nullptr);
int unix_connect(const char *addr, int timeout);
int inet_connect(const char *dip, int port, int timeout);
int host_connect(const char *host, int port, int timeout);
int netpath_connect(const char *netpath, int timeout);
int get_peername(int sockfd, int *host, int *port);

/* dns ############################################################## */
int get_hostaddr(const char *host, std::vector<std::string> &addrs);
inline int get_hostaddr(const std::string &host, std::vector<std::string> &addrs)
{
    return get_hostaddr(host.c_str(), addrs);
}
int get_localaddr(std::vector<std::string> &addrs);
int get_ipint(const char *ipstr);
inline int get_ipint(const std::string &ipstr)
{
    return get_ipint(ipstr.c_str());
}
int get_network(int ip, int masklen);
int get_netmask(int masklen);
int get_broadcast(int ip, int masklen);
int get_ip_min(int ip, int masklen);
int get_ip_max(int ip, int masklen);
bool is_ip(const char *ip);
inline bool is_ip(const std::string &ip)
{
    return is_ip(ip.c_str());
}
int is_intranet(int ip);
int is_intranet2(const char *ip);
char *get_ipstring(int ip, char *ipstr);
std::string get_ipstring(int ip);

/* time ############################################################ */
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
struct timeofday gettimeofday();
int64_t millisecond(int64_t plus = 0);
void sleep_millisecond(int64_t delay);
int64_t millisecond_to(int64_t stamp);
int64_t second();
void sleep(int64_t delay);
std::string rfc1123_time(int64_t t = 0);
std::string rfc822_time(int64_t t = 0);
int64_t timegm(struct tm *tm);

/* zcc end ######################################################### */
zcc_namespace_end;
#pragma pack(pop)
#endif /* __cplusplus */

#endif // ZCC_LIB_INCLUDE_STDLIB___


## 常用字符串函数, [LIB-ZC](./README.md)

[LIB-ZC](./README.md) 封装了常用字符串函数

[C版本](./stream.md)

命名空间 zcc

## 字符操作,类别判定

都是宏, 实验性质

<TABLE class="tbview" width="100%">
<TR><TD>zcc::tolower(int ch)</TD><TD>字符转小写</TD></TR>
<TR><TD>zcc::toupper(int ch)</TD><TD>字符转大写</TD></TR>
<TR><TD>zcc::isalnum(int ch)</TD><TD>字符是否是字母或数字</TD></TR>
<TR><TD>zcc::isalpha(int ch)</TD><TD>字符是否是字母</TD></TR>
<TR><TD>zcc::islower(int ch)</TD><TD>字符是否是小写</TD></TR>
<TR><TD>zcc::isupper(int ch)</TD><TD>字符是否是大写</TD></TR>
<TR><TD>zcc::isdigit(int ch)</TD><TD>字符是否是数字</TD></TR>
<TR><TD>zcc::isxdigit(int ch)</TD><TD>字符是否是 16 进制数字 既: 0-9a-hA-H</TD></TR>
<TR><TD>zcc::hexval(int ch)</TD><TD>16 进制字符转 10 进制, 其他字符返回 -1</TD></TR>
<TR><TD>zcc::istrim(int ch)</TD><TD>字符是否是可以 trim 掉的</TD></TR>
<TR><TD>zcc::is_trimable(int ch)</TD><TD>字符是否是可以 trim 掉的</TD></TR>
</TABLE>

## 其他函数

```c++
// trim
char *trim_left(char *str);
char *trim_right(char *str);
char *trim(char *str);
std::string &trim_right(std::string &str, const char *delims = 0);
std::string &trim_line_end_rn(std::string &str);

// skip
char *skip_left(const char *str, const char *ignores);
int skip_right(const char *str, int len, const char *ignores);
int skip(const char *line, int len, const char *ignores_left, const char *ignores_right, char **start);

// s 是 "0", "n", "N", "no", "NO", "false", "FALSE" 返回 fals
// s 是 "1", "y", "Y", "yes", "YES", "true", "TRUE" 返回 true
// 否则 返回 def
bool str_to_bool(const char *s, bool def = false);
inline bool str_to_bool(const std::string &s, bool def = false);

// 转换字符串为秒, 支持 h(小时), m(分), s(秒), d(天), w(周) 
// 如 "1026S" = > 1026, "8h" => 8 * 3600, "" = > def
int64_t str_to_second(const char *s, int64_t def);

// 转换字符串为大小, 支持 g(G), m(兆), k千), b
// 如 "9M" = > 9 * 1024 * 1024 
int64_t str_to_size(const char *s, int64_t def);

//
inline int tolower(int ch);
inline int toupper(int ch);
char *tolower(char *str);
char *toupper(char *str);
std::string &tolower(std::string &str);
std::string &toupper(std::string &str);

//
char *clear_null(char *data, int64_t size);
std::string &clear_null(std::string &data);

// windows 没有
int strcasecmp(const char *a, const char *b);
inline int strncasecmp(const char *a, const char *b, size_t c);
std::string &vsprintf_1024(std::string &str, const char *format, va_list ap);

#ifdef __linux__
std::string __attribute__((format(gnu_printf, 2, 3))) & sprintf_1024(std::string &str, const char *format, ...);
#else  // __linux__
std::string &sprintf_1024(std::string &str, const char *format, ...);
#endif // __linux__

// split
std::vector<std::string> split(const char *s, int len, const char *delims, bool ignore_empty_token = false);
inline std::vector<std::string> split(const char *s, const char *delims, bool ignore_empty_token = false);
inline std::vector<std::string> split(const std::string &s, const char *delims, bool ignore_empty_token = false);
std::vector<std::string> split(const char *s, int len, int delim, bool ignore_empty_token = false);
inline std::vector<std::string> split(const char *s, int delim, bool ignore_empty_token = false);
inline std::vector<std::string> split(const std::string &s, int delim, bool ignore_empty_token = false);
inline std::vector<std::string> split_and_ignore_empty_token(const char *s, int len, const char *delims);
inline std::vector<std::string> split_and_ignore_empty_token(const char *s, const char *delims);
inline std::vector<std::string> split_and_ignore_empty_token(const std::string &s, const char *delims);
inline std::vector<std::string> split_and_ignore_empty_token(const char *s, int len, int delim);
inline std::vector<std::string> split_and_ignore_empty_token(const char *s, int delim);
inline std::vector<std::string> split_and_ignore_empty_token(const std::string &s, int delim);

// windows(MSVC) 没有
inline static void *memmem(const void *l, int64_t l_len, const void *s, int64_t s_len);
inline int strcasecmp(const char *a, const char *b);
inline int strncasecmp(const char *a, const char *b, size_t c);
char *strcasestr(const char *haystack, const char *needle);

// 下面是词典 std::map<std::string,
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
```
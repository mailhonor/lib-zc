
## 几个时间函数, [LIB-ZC](./README.md)

[LIB-ZC](./README.md) 封装了几个时间函数

[C++版本](./time.md)

命名空间 zcc

## 函数

```c++
static const int64_t var_max_millisecond_duration(3600L * 24 * 365 * 10 * 1000);

// MSVC 没类似函数
struct timeofday
{
    int64_t tv_sec;  /* seconds */
    int64_t tv_usec; /* microseconds */
};
struct timeofday gettimeofday();

// 毫秒
int64_t millisecond(int64_t plus = 0);
// 睡眠毫秒
void sleep_millisecond(int64_t delay);
// 睡眠直到
int64_t millisecond_to(int64_t stamp);


// 秒
int64_t second();
// 睡眠
void sleep(int64_t delay);

// 获取 week day 的简称, 从 0 开始
// {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const char *get_day_abbr_of_week(int day);
// 获 月份的简称, 从 0 开始
// const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
const char *get_month_abbr(int month);

// http time
std::string rfc7231_time(int64_t t = 0);
ZCC_DEPRECATED inline std::string rfc1123_time(int64_t t = 0);
inline std::string http_time(int64_t t = 0);

// mime time
std::string rfc822_time(int64_t t = 0);
inline std::string mail_time(int64_t t = 0);

// 类似GCC标准C库的 time_t timegm(struct tm *tm); windows平台没有
int64_t timegm(struct tm *tm);
```
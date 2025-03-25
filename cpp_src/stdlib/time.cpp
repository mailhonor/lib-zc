/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2024-02-18
 * ================================
 */

#include "zcc/zcc_stdlib.h"

#include <thread>
#include <chrono>
#include <ctime>
#ifdef _WIN64
#include <timezoneapi.h>
#else // _WIN64
#include <poll.h>
#endif // _WIN64

zcc_namespace_begin;

/**
 * @brief 根据传入的星期几的数字返回对应的缩写字符串
 * 
 * @param day 星期几的数字，范围0-6，0表示周日，6表示周六
 * @return const char* 对应的星期缩写字符串
 */
const char *get_day_abbr_of_week(int day)
{
    // 定义星期缩写数组
    const char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    // %w
    // 如果传入的数字小于0，将其置为0
    if (day < 0)
    {
        day = 0;
    }
    // 如果传入的数字大于6，将其置为6
    if (day > 6)
    {
        day = 6;
    }
    return days[day];
}

/**
 * @brief 根据传入的月份数字返回对应的缩写字符串
 * 
 * @param month 月份数字，范围0-11，0表示一月，11表示十二月
 * @return const char* 对应的月份缩写字符串
 */
const char *get_month_abbr(int month)
{
    // 定义月份缩写数组
    const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    // %m - 1
    // 如果传入的数字小于0，将其置为0
    if (month < 0)
    {
        month = 0;
    }
    // 如果传入的数字大于11，将其置为11
    if (month > 11)
    {
        month = 11;
    }
    return months[month];
}

/**
 * @brief 获取当前时间的秒和微秒部分
 * 
 * @return timeofday 包含秒和微秒的结构体
 */
timeofday gettimeofday()
{
    timeofday r;
    // 获取当前系统时间
    auto now = std::chrono::system_clock::now();
    // 获取从纪元开始到现在的持续时间
    auto duration_since_epoch = now.time_since_epoch();
    // 将持续时间转换为微秒
    auto millis = std::chrono::duration_cast<std::chrono::microseconds>(duration_since_epoch).count();
    r.tv_sec = millis / (1000 * 1000);
    r.tv_usec = millis % (1000 * 1000);
    return r;
}

/**
 * @brief 获取当前时间戳加上指定超时时间后的毫秒时间戳
 * 
 * @param timeout 超时时间（毫秒）
 * @return int64_t 当前时间戳加上超时时间后的毫秒时间戳
 */
int64_t millisecond(int64_t timeout)
{
    // 获取当前系统时间
    auto now = std::chrono::system_clock::now();
    // 获取从纪元开始到现在的持续时间
    auto duration_since_epoch = now.time_since_epoch();
    // 将持续时间转换为毫秒
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration_since_epoch).count();
    return millis + timeout;
}

/**
 * @brief 让当前线程休眠指定的毫秒数
 * 
 * @param delay 休眠时间（毫秒）
 */
void sleep_millisecond(int64_t delay)
{
#ifdef _WIN64
    // 在Windows 64位系统下，使用标准库的sleep_for函数进行休眠
    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
#else  // _WIN64
    int left = delay;
    // 计算休眠结束的时间戳
    int64_t end = millisecond() + left + 1;
    while (left > 0)
    {
        // 使用poll函数进行等待
        poll(0, 0, left);
        // 计算剩余的休眠时间
        left = end - millisecond();
    }
#endif // _WIN64
}

/**
 * @brief 计算从当前时间到指定时间戳的剩余毫秒数
 * 
 * @param stamp 指定的时间戳（毫秒）
 * @return int64_t 从当前时间到指定时间戳的剩余毫秒数
 */
int64_t millisecond_to(int64_t stamp)
{
    return stamp - millisecond();
}

/**
 * @brief 获取当前时间的秒级时间戳
 * 
 * @return int64_t 当前时间的秒级时间戳
 */
int64_t second(void)
{
    // 获取当前系统时间
    auto const clock_now = std::chrono::system_clock::now();
    // 将系统时间转换为C标准时间
    std::time_t time = std::chrono::system_clock::to_time_t(clock_now);
    return time;
}

/**
 * @brief 让当前线程休眠指定的秒数
 * 
 * @param delay 休眠时间（秒）
 */
void sleep(int64_t delay)
{
    // 调用sleep_millisecond函数，将秒数转换为毫秒数
    sleep_millisecond(delay * 1000);
}

/**
 * @brief 根据指定的时间戳生成符合RFC 7231标准的时间字符串
 * 
 * @param t 时间戳，如果为0则使用当前时间
 * @return std::string 符合RFC 7231标准的时间字符串
 */
std::string rfc7231_time(int64_t t)
{
    char buf[128 + 1];
    if (t == 0)
    {
        // 如果时间戳为0，使用当前时间
        t = second();
    }
    // 将时间戳转换为UTC时间结构体
    std::tm *now_tm = std::gmtime((time_t *)&t);
    // 格式化时间字符串
    std::strftime(buf, 128, "%a, %d %b %Y %H:%M:%S GMT", now_tm);
    return buf;
}

/**
 * @brief 根据指定的时间戳生成符合RFC 822标准的时间字符串
 * 
 * @param t 时间戳，如果小于1则使用当前时间
 * @return std::string 符合RFC 822标准的时间字符串
 */
std::string rfc822_time(int64_t t)
{
    std::string r;
    char buf[128 + 1];
    if (t < 1)
    {
        // 如果时间戳小于1，使用当前时间
        t = second();
    }
    // 将时间戳转换为本地时间结构体
    auto p = std::localtime((time_t *)&t);
    // FIXME %Z
    // std::strftime(buf, 128, "%a, %d %b %Y %H:%M:%S %z", p);

    // 添加星期缩写
    r.append(get_day_abbr_of_week(p->tm_wday));
    // 添加月份和日期
    std::strftime(buf, 128, ", %d ", p);
    r.append(buf);
    // 添加月份缩写
    r.append(get_month_abbr(p->tm_mon));
    // 添加剩余部分
    std::strftime(buf, 128, " %Y %H:%M:%S %z", p);
    r.append(buf);

    return r;
}

/**
 * @brief 将tm结构体转换为UTC时间戳
 * 
 * @param tm 指向tm结构体的指针
 * @return int64_t UTC时间戳
 */
int64_t timegm(struct tm *tm)
{
#ifdef _WIN64
    // 在Windows 64位系统下，使用mktime函数转换时间
    int64_t t = (int64_t)::mktime(tm);
    TIME_ZONE_INFORMATION tzi;
    // 获取时区信息
    GetTimeZoneInformation(&tzi);
    // 减去时区偏移量
    t -= tzi.Bias * 60;
    return t;
#else  // _WIN64
    // 在非Windows系统下，使用系统的timegm函数
    return ::timegm(tm);
#endif // _WIN64
}

zcc_namespace_end;

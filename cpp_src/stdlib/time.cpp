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

static int64_t tm_to_timestamp(std::tm &t)
{
    std::time_t time = std::mktime(&t);
    return (time == -1) ? var_invalid_time : static_cast<int64_t>(time);
}

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

int64_t millisecond()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

int64_t microsecond()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
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
 * @param t 时间戳
 * @return std::string 符合RFC 7231标准的时间字符串
 */
std::string rfc7231_time(int64_t t)
{
    char buf[128 + 1];
    if (t == var_use_current_time)
    {
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
    if (t == var_use_current_time)
    {
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
 * @brief 将时间戳转换为简单的日期时间字符串
 *
 * @param t 时间戳，如果小于1则使用当前时间
 * @return std::string 格式为" %Y %H:%M:"的日期时间字符串
 */
std::string simple_date_time(int64_t t)
{
    char buf[128 + 1];
    if (t == var_use_current_time)
    {
        t = second();
    }
    auto p = std::localtime((time_t *)&t);
    std::strftime(buf, 128, "%Y-%m-%d %H:%M", p);
    return buf;
}

std::string simple_date_time_with_second(int64_t t)
{
    char buf[128 + 1];
    if (t == var_use_current_time)
    {
        t = second();
    }
    auto p = std::localtime((time_t *)&t);
    std::strftime(buf, 128, "%Y-%m-%d %H:%M:%S", p);
    return buf;
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

// ISO.8601.2004 time
// 辅助函数：解析时区偏移（秒）
static int64_t iso8601_2004_parse_timezone_offset(const std::string &tz_str)
{
    if (tz_str.empty() || tz_str == "Z")
    {
        return 0; // UTC
    }

    int sign = 1;
    size_t start = 0;

    if (tz_str[0] == '+')
    {
        sign = 1;
        start = 1;
    }
    else if (tz_str[0] == '-')
    {
        sign = -1;
        start = 1;
    }
    else
    {
        return var_invalid_time; // 无效的时区格式
    }

    if (tz_str.size() - start != 2 && tz_str.size() - start != 4)
    {
        return var_invalid_time; // 无效的时区格式
    }

    try
    {
        int hours = std::stoi(tz_str.substr(start, 2));
        int minutes = (tz_str.size() - start == 4) ? std::stoi(tz_str.substr(start + 2, 2)) : 0;

        if (hours < 0 || hours > 23 || minutes < 0 || minutes > 59)
        {
            return var_invalid_time; // 无效的时区值
        }

        return sign * (hours * 3600 + minutes * 60);
    }
    catch (const std::exception &)
    {
        return var_invalid_time;
    }
}

// 辅助函数：解析时间部分
static bool iso8601_2004_parse_time_component(const std::string &time_str, std::tm &t)
{
    try
    {
        if (time_str.empty())
        {
            return true; // 时间部分为空，使用默认值
        }

        if (time_str == "--00")
        {
            t.tm_sec = 0;
        }
        else if (time_str == "-2200")
        {
            t.tm_min = 22;
            t.tm_sec = 0;
        }
        else if (time_str.size() == 2)
        {
            t.tm_hour = std::stoi(time_str);
        }
        else if (time_str.size() == 4)
        {
            t.tm_hour = std::stoi(time_str.substr(0, 2));
            t.tm_min = std::stoi(time_str.substr(2, 2));
        }
        else if (time_str.size() == 6)
        {
            t.tm_hour = std::stoi(time_str.substr(0, 2));
            t.tm_min = std::stoi(time_str.substr(2, 2));
            t.tm_sec = std::stoi(time_str.substr(4, 2));
        }
        else
        {
            return false; // 无效的时间格式
        }

        // 验证时间值范围
        return !(t.tm_hour < 0 || t.tm_hour > 23 ||
                 t.tm_min < 0 || t.tm_min > 59 ||
                 t.tm_sec < 0 || t.tm_sec > 59);
    }
    catch (const std::exception &)
    {
        return false;
    }
}

// 辅助函数：解析日期部分
static bool iso8601_2004_parse_date_component(const std::string &date_str, std::tm &t, bool day_is_preferred)
{
    try
    {
        if (date_str.empty())
        {
            return true; // 日期部分为空，使用默认值
        }

        if (date_str == "--0412")
        {
            t.tm_mon = 3; // 4月（tm_mon从0开始）
            t.tm_mday = 12;
        }
        else if (date_str == "---12")
        {
            t.tm_mday = 12;
        }
        else if (date_str == "--1022")
        {
            t.tm_mon = 9; // 10月
            t.tm_mday = 22;
        }
        else if (date_str.size() == 4)
        {
            // 年份（如1985）
            int year = std::stoi(date_str);
            t.tm_year = year - 1900; // tm_year是从1900年开始计算的
        }
        else if (date_str.size() == 7 && date_str[4] == '-')
        {
            // 如1985-04
            int year = std::stoi(date_str.substr(0, 4));
            int month = std::stoi(date_str.substr(5, 2));

            if (month < 1 || month > 12)
                return false;

            t.tm_year = year - 1900;
            t.tm_mon = month - 1;
        }
        else if (date_str.size() == 8)
        {
            // 如19961022或19850412
            int year = std::stoi(date_str.substr(0, 4));
            int month = std::stoi(date_str.substr(4, 2));
            int day = std::stoi(date_str.substr(6, 2));

            if (month < 1 || month > 12 || day < 1 || day > 31)
                return false;

            t.tm_year = year - 1900;
            t.tm_mon = month - 1;
            t.tm_mday = day;
        }
        else
        {
            return false; // 无效的日期格式
        }

        return true;
    }
    catch (const std::exception &)
    {
        return false;
    }
}

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
int64_t iso8601_2004_time_from_time(const std::string &s)
{
    try
    {
        std::tm t = {};
        // 默认日期为1970-01-01（Unix纪元起始点）
        t.tm_year = 70; // 1970年
        t.tm_mon = 0;   // 1月
        t.tm_mday = 1;  // 1日

        std::string time_str = s;
        std::string tz_str;

        // 分离时间和时区信息
        size_t z_pos = s.find('Z');
        if (z_pos != std::string::npos)
        {
            tz_str = "Z";
            time_str = s.substr(0, z_pos);
        }
        else
        {
            size_t plus_pos = s.find('+');
            size_t minus_pos = s.find('-');

            if (plus_pos != std::string::npos && (minus_pos == std::string::npos || plus_pos < minus_pos))
            {
                tz_str = s.substr(plus_pos);
                time_str = s.substr(0, plus_pos);
            }
            else if (minus_pos != std::string::npos && (minus_pos == 0 ||
                                                        (time_str.size() > minus_pos && (time_str[minus_pos - 1] < '0' || time_str[minus_pos - 1] > '9'))))
            {
                tz_str = s.substr(minus_pos);
                time_str = s.substr(0, minus_pos);
            }
        }

        // 解析时间部分
        if (!iso8601_2004_parse_time_component(time_str, t))
        {
            return var_invalid_time;
        }

        // 解析时区
        int64_t tz_offset = iso8601_2004_parse_timezone_offset(tz_str);
        if (tz_offset == var_invalid_time)
        {
            return var_invalid_time;
        }

        // 转换为时间戳并调整时区
        int64_t timestamp = tm_to_timestamp(t);
        return (timestamp == var_invalid_time) ? var_invalid_time : timestamp - tz_offset;
    }
    catch (const std::exception &)
    {
        return var_invalid_time;
    }
}

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
int64_t iso8601_2004_time_from_date(const std::string &s, bool day_is_preferred)
{
    try
    {
        std::tm t = {};
        // 默认日期为1970-01-01 00:00:00（Unix纪元起始点）
        t.tm_year = 70; // 1970年
        t.tm_mon = 0;   // 1月
        t.tm_mday = 1;  // 1日

        std::string date_str, time_str, tz_str;

        // 分离日期和时间部分
        size_t t_pos = s.find('T');
        if (t_pos != std::string::npos)
        {
            date_str = s.substr(0, t_pos);
            std::string time_part = s.substr(t_pos + 1);

            // 分离时间和时区
            size_t z_pos = time_part.find('Z');
            if (z_pos != std::string::npos)
            {
                tz_str = "Z";
                time_str = time_part.substr(0, z_pos);
            }
            else
            {
                size_t plus_pos = time_part.find('+');
                size_t minus_pos = time_part.find('-');

                if (plus_pos != std::string::npos && (minus_pos == std::string::npos || plus_pos < minus_pos))
                {
                    tz_str = time_part.substr(plus_pos);
                    time_str = time_part.substr(0, plus_pos);
                }
                else if (minus_pos != std::string::npos && (minus_pos == 0 ||
                                                            (time_part.size() > minus_pos && (time_part[minus_pos - 1] < '0' || time_part[minus_pos - 1] > '9'))))
                {
                    tz_str = time_part.substr(minus_pos);
                    time_str = time_part.substr(0, minus_pos);
                }
                else
                {
                    time_str = time_part;
                }
            }

            // 解析时间部分
            if (!iso8601_2004_parse_time_component(time_str, t))
            {
                return var_invalid_time;
            }
        }
        else if (!s.empty() && s[0] == 'T')
        {
            // 只有时间部分，如"T102200"
            std::string time_part = s.substr(1);

            // 分离时间和时区
            size_t z_pos = time_part.find('Z');
            if (z_pos != std::string::npos)
            {
                tz_str = "Z";
                time_str = time_part.substr(0, z_pos);
            }
            else
            {
                size_t plus_pos = time_part.find('+');
                size_t minus_pos = time_part.find('-');

                if (plus_pos != std::string::npos && (minus_pos == std::string::npos || plus_pos < minus_pos))
                {
                    tz_str = time_part.substr(plus_pos);
                    time_str = time_part.substr(0, plus_pos);
                }
                else if (minus_pos != std::string::npos && (minus_pos == 0 ||
                                                            (time_part.size() > minus_pos && (time_part[minus_pos - 1] < '0' || time_part[minus_pos - 1] > '9'))))
                {
                    tz_str = time_part.substr(minus_pos);
                    time_str = time_part.substr(0, minus_pos);
                }
                else
                {
                    time_str = time_part;
                }
            }

            // 解析时间部分
            if (!iso8601_2004_parse_time_component(time_str, t))
            {
                return var_invalid_time;
            }

            date_str = ""; // 没有日期部分
        }
        else
        {
            // 只有日期部分
            date_str = s;

            // 检查是否包含时区信息
            size_t z_pos = date_str.find('Z');
            if (z_pos != std::string::npos)
            {
                tz_str = "Z";
                date_str = date_str.substr(0, z_pos);
            }
            else
            {
                size_t plus_pos = date_str.find('+');
                size_t minus_pos = date_str.find('-');

                if (plus_pos != std::string::npos && (minus_pos == std::string::npos || plus_pos < minus_pos))
                {
                    tz_str = date_str.substr(plus_pos);
                    date_str = date_str.substr(0, plus_pos);
                }
                else if (minus_pos != std::string::npos)
                {
                    tz_str = date_str.substr(minus_pos);
                    date_str = date_str.substr(0, minus_pos);
                }
            }
        }

        // 解析日期部分
        if (!iso8601_2004_parse_date_component(date_str, t, day_is_preferred))
        {
            return var_invalid_time;
        }

        // 解析时区
        int64_t tz_offset = iso8601_2004_parse_timezone_offset(tz_str);
        if (tz_offset == var_invalid_time)
        {
            return var_invalid_time;
        }

        // 转换为时间戳并调整时区
        int64_t timestamp = tm_to_timestamp(t);
        return (timestamp == var_invalid_time) ? var_invalid_time : timestamp - tz_offset;
    }
    catch (const std::exception &)
    {
        return var_invalid_time;
    }
}

zcc_namespace_end;

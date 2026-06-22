/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2024-02-18
 * ================================
 */

#include "zcc/zcc_stdlib.h"
#include <mutex>
#include <unordered_map>
#include <chrono>
#include <ctime>
#include <fstream>
#include <vector>
#include <cstring>
#ifdef _WIN64
#include <timezoneapi.h>
#else // _WIN64
#include <unistd.h>
#endif // _WIN64
#ifdef __APPLE__
#include <sys/param.h>
#include <sys/sysctl.h>
#endif

zcc_namespace_begin;

// 辅助函数：解析时间部分
static bool iso8601_2004_parse_time_component(const std::string &time_str, std::tm &t)
{
    try
    {
        if (time_str.empty())
        {
            return true; // 时间部分为空，使用默认值
        }

        if (time_str.size() == 2)
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
static bool iso8601_2004_parse_date_component(const std::string &date_str, std::tm &t)
{
    try
    {
        if (date_str.empty())
        {
            return true; // 日期部分为空，使用默认值
        }

        if (date_str.size() == 4)
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
 * @return 对应的Unix时间戳，解析失败返回 var_invalid_time
 */
//  102200
//  1022
//  10
//  -2200
//  --00
//  102200Z
//  102200-0800
int64_t iso8601_2004_time_from_time(const std::string &str, const std::string &default_tzid)
{
    auto s = str;
    toupper(s);
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
        if (tz_str.empty())
        {
            tz_str = default_tzid;
        }

        // 解析时间部分
        if (!iso8601_2004_parse_time_component(time_str, t))
        {
            return var_invalid_time;
        }

        // 解析时区
        int64_t tz_offset = 0;
        if (tz_str.empty() || tz_str == "Z")
        {
            tz_offset = 0; // UTC
        }
        else if (tz_str[0] == '+' || tz_str[0] == '-' || isdigit(tz_str[0]))
        {
            tz_offset = timezone_0800_offset(tz_str);
        }
        else
        {
            timezone_info tzinfo = get_timezone_info(tz_str, second());
            tz_offset = tzinfo.gmtoff;
        }

        // 转换为时间戳并调整时区
        int64_t timestamp = zcc::timegm(&t);
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
int64_t iso8601_2004_time_from_datetime(const std::string &str, const std::string &default_tzid)
{
    auto s = str;
    toupper(s);
    try
    {
        struct tm t = {};
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
        if (!iso8601_2004_parse_date_component(date_str, t))
        {
            return var_invalid_time;
        }

        //
        int64_t timestamp = zcc::timegm(&t);

        // 解析时区
        if (tz_str.empty())
        {
            tz_str = default_tzid;
        }
        int64_t tz_offset = 0;
        if (tz_str.empty() || tz_str == "Z")
        {
            tz_offset = 0;
        }
        else if (tz_str[0] == '+' || tz_str[0] == '-' || isdigit(tz_str[0]))
        {
            tz_offset = timezone_0800_offset(tz_str);
        }
        else
        {
            timezone_info tzinfo = get_timezone_info(tz_str, timestamp);
            tz_offset = tzinfo.gmtoff;
        }
        // 转换为时间戳并调整时区
        return (timestamp == var_invalid_time) ? var_invalid_time : timestamp - tz_offset;
    }
    catch (const std::exception &)
    {
        return var_invalid_time;
    }
}

std::string unix_to_iso8601_utc(int64_t timestamp)
{
    char buf[128];
    struct tm *tm = gmtime(timestamp);
    if (tm == nullptr)
    {
        return "20260424T170000Z";
    }
    std::sprintf(buf, "%04d%02d%02dT%02d%02d%02dZ", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
    return buf;
}

zcc_namespace_end;

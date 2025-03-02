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
#include <windows.h>
#include <timezoneapi.h>
#else // _WIN64
#include <poll.h>
#endif // _WIN64

zcc_namespace_begin;

const char *get_day_abbr_of_week(int day)
{
    const char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    // %w
    if (day < 0)
    {
        day = 0;
    }
    if (day > 6)
    {
        day = 6;
    }
    return days[day];
}

const char *get_month_abbr(int month)
{
    const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    // %m - 1
    if (month < 0)
    {
        month = 0;
    }
    if (month > 11)
    {
        month = 11;
    }
    return months[month];
}

timeofday gettimeofday()
{
    timeofday r;
    auto now = std::chrono::system_clock::now();
    auto duration_since_epoch = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::microseconds>(duration_since_epoch).count();
    r.tv_sec = millis % (1000 * 1000);
    r.tv_usec = millis / (1000 * 1000);
    return r;
}

int64_t millisecond(int64_t timeout)
{
    auto now = std::chrono::system_clock::now();
    auto duration_since_epoch = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration_since_epoch).count();
    return millis + timeout;
}

void sleep_millisecond(int64_t delay)
{
#ifdef _WIN64
    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
#else  // _WIN64
    int left = delay;
    int64_t end = millisecond() + left + 1;
    while (left > 0)
    {
        poll(0, 0, left);
        left = end - millisecond();
    }
#endif // _WIN64
}

int64_t millisecond_to(int64_t stamp)
{
    return stamp - millisecond();
}

int64_t second(void)
{
    auto const clock_now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(clock_now);
    return time;
}

void sleep(int64_t delay)
{
    sleep_millisecond(delay * 1000);
}

std::string rfc1123_time(int64_t t)
{
    char buf[128 + 1];
    if (t == 0)
    {
        t = second();
    }
    std::tm *now_tm = std::gmtime((time_t *)&t);
    std::strftime(buf, 128, "%a, %d %b %Y %H:%M:%S GMT", now_tm);
    return buf;
}

std::string rfc822_time(int64_t t)
{
    std::string r;
    char buf[128 + 1];
    if (t < 1)
    {
        t = second();
    }
    auto p = std::localtime((time_t *)&t);
    // FIXME %Z
    // std::strftime(buf, 128, "%a, %d %b %Y %H:%M:%S %z", p);

    // week
    r.append(get_day_abbr_of_week(p->tm_wday));
    // month day
    std::strftime(buf, 128, ", %d ", p);
    r.append(buf);
    // month
    r.append(get_month_abbr(p->tm_mon));
    // left
    std::strftime(buf, 128, " %Y %H:%M:%S %z", p);
    r.append(buf);

    return r;
}

int64_t timegm(struct tm *tm)
{
#ifdef _WIN64
    int64_t t = (int64_t)::mktime(tm);
    TIME_ZONE_INFORMATION tzi;
    GetTimeZoneInformation(&tzi);
    t -= tzi.Bias * 60;
    return t;
#else  // _WIN64
    return ::timegm(tm);
#endif // _WIN64
}

zcc_namespace_end;

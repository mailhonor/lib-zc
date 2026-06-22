/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn
 * 2025-08-14
 * ================================
 */

#include "zcc/zcc_calendar.h"
#include "zcc/zcc_charset.h"
#include "zcc/zcc_json.h"
#include <set>
#include <ctime>
#include <time.h>

zcc_namespace_begin;

ics_calendar_event_matcher::ics_calendar_event_matcher()
{
}

ics_calendar_event_matcher::~ics_calendar_event_matcher()
{
}

static int get_weekday(int year, int month, int mday)
{
    std::tm tm = {};
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = mday;
    zcc::timegm(&tm);
    return tm.tm_wday;
}

static int64_t ymd_to_ordinal(int year, int month, int mday)
{
    std::tm tm = {};
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = mday;
    int64_t sec = zcc::timegm(&tm);
    return sec / 86400;
}

static int get_last_day_of_month(int year, int month)
{
    static const int month_days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month != 2)
    {
        return month_days[month - 1];
    }
    bool leap = ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
    return leap ? 29 : 28;
}

static bool local_tm(int64_t stamp, const std::string &tzid, std::tm &out_tm)
{
    std::string real_tzid = tzid;
    if (real_tzid.empty())
    {
        real_tzid = "UTC";
    }
    if (gmtime_with_timezone(stamp, real_tzid, &out_tm))
    {
        return true;
    }
    auto *p = gmtime(stamp);
    if (!p)
    {
        return false;
    }
    out_tm = *p;
    return true;
}

static int local_date_yyyymmdd(int64_t stamp, const std::string &tzid)
{
    std::tm tm = {};
    if (!local_tm(stamp, tzid, tm))
    {
        return -1;
    }
    return (tm.tm_year + 1900) * 10000 + (tm.tm_mon + 1) * 100 + tm.tm_mday;
}

static int local_weekday(int64_t stamp, const std::string &tzid)
{
    std::tm tm = {};
    if (!local_tm(stamp, tzid, tm))
    {
        return -1;
    }
    return tm.tm_wday == 0 ? 7 : tm.tm_wday;
}

static bool matches_bymonthday(int year, int month, int mday, const std::vector<int> &bymonthday)
{
    if (bymonthday.empty())
    {
        return true;
    }
    int last_day = get_last_day_of_month(year, month);
    for (int item : bymonthday)
    {
        if (item > 0 && item == mday)
        {
            return true;
        }
        if (item < 0 && mday == last_day + 1 + item)
        {
            return true;
        }
    }
    return false;
}

static bool matches_bymonth(int month, const std::vector<int> &bymonth)
{
    if (bymonth.empty())
    {
        return true;
    }
    for (int item : bymonth)
    {
        if (item == month)
        {
            return true;
        }
    }
    return false;
}

static bool matches_byweekday(int weekday, const std::vector<ics_calendar_event_rrule_byweekday> &byweekday)
{
    if (byweekday.empty())
    {
        return true;
    }
    for (auto &item : byweekday)
    {
        if (item.day == weekday)
        {
            return true;
        }
    }
    return false;
}

static bool matches_byyearday(int year, int month, int mday, const std::vector<int> &byyearday);
static bool is_nth_weekday_of_month(int year, int month, int mday, int weekday, int sn);

static bool ordinal_to_ymd(int64_t ordinal, int &year, int &month, int &mday)
{
    time_t t = (time_t)(ordinal * 86400);
    auto p = gmtime(t);
    if (!p)
    {
        return false;
    }
    year = p->tm_year + 1900;
    month = p->tm_mon + 1;
    mday = p->tm_mday;
    return true;
}

static bool is_rrule_match_date(int year, int month, int mday, int start_year, int start_month, int start_mday, int64_t start_ordinal, const ics_calendar_event_rrule &rrule)
{
    int64_t current_ordinal = ymd_to_ordinal(year, month, mday);
    if (current_ordinal < start_ordinal)
    {
        return false;
    }

    int weekday = get_weekday(year, month, mday);
    int64_t diff_days = current_ordinal - start_ordinal;
    int interval = rrule.interval > 0 ? rrule.interval : 1;

    if (rrule.freq == ics_calendar_event_rrule::freq_type::daily)
    {
        if (diff_days % interval != 0)
        {
            return false;
        }
        if (!matches_byweekday(weekday, rrule.byweekday))
        {
            return false;
        }
        if (!matches_bymonth(month, rrule.bymonth))
        {
            return false;
        }
        if (!matches_bymonthday(year, month, mday, rrule.bymonthday))
        {
            return false;
        }
        if (!matches_byyearday(year, month, mday, rrule.byyearday))
        {
            return false;
        }
        return true;
    }

    if (rrule.freq == ics_calendar_event_rrule::freq_type::weekly)
    {
        int start_weekday = get_weekday(start_year, start_month, start_mday);
        int64_t start_week_begin = start_ordinal - ((start_weekday + 6) % 7);
        int64_t current_week_begin = current_ordinal - ((weekday + 6) % 7);
        int64_t week_index = (current_week_begin - start_week_begin) / 7;
        if (week_index < 0 || week_index % interval != 0)
        {
            return false;
        }
        if (!matches_byweekday(weekday, rrule.byweekday))
        {
            return false;
        }
        if (!matches_bymonth(month, rrule.bymonth))
        {
            return false;
        }
        if (!matches_byyearday(year, month, mday, rrule.byyearday))
        {
            return false;
        }
        return true;
    }

    if (rrule.freq == ics_calendar_event_rrule::freq_type::monthly)
    {
        int months = (year - start_year) * 12 + (month - start_month);
        if (months < 0 || months % interval != 0)
        {
            return false;
        }
        if (!matches_bymonth(month, rrule.bymonth))
        {
            return false;
        }
        if (!rrule.bymonthday.empty())
        {
            if (!matches_bymonthday(year, month, mday, rrule.bymonthday))
            {
                return false;
            }
        }
        else if (rrule.byweekday.empty())
        {
            if (mday != start_mday)
            {
                return false;
            }
        }
        if (!rrule.byweekday.empty())
        {
            bool any = false;
            for (auto &item : rrule.byweekday)
            {
                if (is_nth_weekday_of_month(year, month, mday, item.day, item.sn))
                {
                    any = true;
                    break;
                }
            }
            if (!any)
            {
                return false;
            }
        }
        if (!matches_byyearday(year, month, mday, rrule.byyearday))
        {
            return false;
        }
        return true;
    }

    if (rrule.freq == ics_calendar_event_rrule::freq_type::yearly)
    {
        int years = year - start_year;
        if (years < 0 || years % interval != 0)
        {
            return false;
        }
        if (!matches_bymonth(month, rrule.bymonth))
        {
            return false;
        }
        if (!rrule.bymonthday.empty())
        {
            if (!matches_bymonthday(year, month, mday, rrule.bymonthday))
            {
                return false;
            }
        }
        if (!rrule.byweekday.empty())
        {
            bool any = false;
            for (auto &item : rrule.byweekday)
            {
                if (is_nth_weekday_of_month(year, month, mday, item.day, item.sn))
                {
                    any = true;
                    break;
                }
            }
            if (!any)
            {
                return false;
            }
        }
        if (rrule.bymonth.empty() && rrule.bymonthday.empty() && rrule.byweekday.empty())
        {
            if (month != start_month || mday != start_mday)
            {
                return false;
            }
        }
        if (!matches_byyearday(year, month, mday, rrule.byyearday))
        {
            return false;
        }
        return true;
    }

    return false;
}

static int count_rrule_occurrences(int start_year, int start_month, int start_mday, int64_t start_ordinal, int target_year, int target_month, int target_mday, const ics_calendar_event_rrule &rrule)
{
    int64_t target_ordinal = ymd_to_ordinal(target_year, target_month, target_mday);
    int count = 0;
    for (int64_t ord = start_ordinal; ord <= target_ordinal; ++ord)
    {
        int year, month, mday;
        if (!ordinal_to_ymd(ord, year, month, mday))
        {
            continue;
        }
        if (is_rrule_match_date(year, month, mday, start_year, start_month, start_mday, start_ordinal, rrule))
        {
            ++count;
        }
    }
    return count;
}

static int get_day_of_year(int year, int month, int mday)
{
    std::tm tm = {};
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = mday;
    zcc::timegm(&tm);
    return tm.tm_yday + 1;
}

static int get_days_in_year(int year)
{
    return ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) ? 366 : 365;
}

static bool matches_byyearday(int year, int month, int mday, const std::vector<int> &byyearday)
{
    if (byyearday.empty())
    {
        return true;
    }
    int day_of_year = get_day_of_year(year, month, mday);
    int days_in_year = get_days_in_year(year);
    for (int item : byyearday)
    {
        if (item > 0 && item == day_of_year)
        {
            return true;
        }
        if (item < 0 && day_of_year == days_in_year + 1 + item)
        {
            return true;
        }
    }
    return false;
}

static bool is_nth_weekday_of_month(int year, int month, int mday, int weekday, int sn)
{
    if (weekday < 0 || weekday > 6)
    {
        return false;
    }
    if (sn == 0)
    {
        return get_weekday(year, month, mday) == weekday;
    }
    if (get_weekday(year, month, mday) != weekday)
    {
        return false;
    }
    int last_day = get_last_day_of_month(year, month);
    if (sn > 0)
    {
        int count = 0;
        for (int day = 1; day <= mday; ++day)
        {
            if (get_weekday(year, month, day) == weekday)
            {
                ++count;
            }
        }
        return count == sn;
    }
    else
    {
        int count = 0;
        for (int day = last_day; day >= mday; --day)
        {
            if (get_weekday(year, month, day) == weekday)
            {
                ++count;
            }
        }
        return count == -sn;
    }
}

static int64_t get_local_event_end_day(int64_t dtend, const std::string &tzid)
{
    if (dtend <= 0)
    {
        return -1;
    }
    int64_t last_second = dtend - 1;
    return local_date_yyyymmdd(last_second, tzid);
}

static int64_t get_year_month_day_from_yyyymmdd(int yyyymmdd)
{
    int y = yyyymmdd / 10000;
    int m = (yyyymmdd / 100) % 100;
    int d = yyyymmdd % 100;
    return y * 10000 + m * 100 + d;
}

bool ics_calendar_event_matcher::match_second_scope(int64_t unix_start, int64_t unix_end)
{
    if (dtstart_ == -1 || unix_start > unix_end)
    {
        return false;
    }

    std::string use_tzid = event_tzid_.empty() ? get_current_timezone() : event_tzid_;
    if (use_tzid.empty())
    {
        use_tzid = "UTC";
    }

    // single occurrence without recurrence
    if (rrule_.freq == ics_calendar_event_rrule::freq_type::none)
    {
        int64_t effective_dtend = (dtend_ != -1) ? dtend_ : dtstart_ + 1; // If no explicit end time, assume 1 second duration

        // Check if the event interval overlaps with the given time range
        if (!(effective_dtend <= unix_start || dtstart_ >= unix_end))
        {
            return true;
        }
        return false;
    }

    // Handle recurring events
    // First check if the time range is beyond the 'until' limit
    if (rrule_.until > 0)
    {
        // Convert the unix_end to date in the event's timezone to compare with until
        int end_day = local_date_yyyymmdd(unix_end, use_tzid);
        if (end_day > rrule_.until)
        {
            return false;
        }
    }

    // For recurring events, we need to check if any occurrence falls within the time range
    // Start from the event's start time or the unix_start, whichever is later
    int64_t check_start = std::max(dtstart_, unix_start);

    // We'll iterate through occurrences within the range
    // This requires more complex logic to handle various recurrence patterns

    // Get start date components for recurrence calculation
    int start_day = local_date_yyyymmdd(dtstart_, use_tzid);
    if (start_day < 0)
    {
        return false;
    }

    int start_year = start_day / 10000;
    int start_month = (start_day / 100) % 100;
    int start_mday = start_day % 100;
    int64_t start_ordinal = ymd_to_ordinal(start_year, start_month, start_mday);

    // Calculate end date for search
    int search_end_day = local_date_yyyymmdd(unix_end, use_tzid);
    int64_t search_end_ordinal = ymd_to_ordinal(search_end_day / 10000, (search_end_day / 100) % 100, search_end_day % 100);

    // Check occurrences within the range
    int64_t current_ordinal = start_ordinal;
    int occurrence_count = 0;

    while (current_ordinal <= search_end_ordinal)
    {
        int current_year = 0, current_month = 0, current_day = 0;
        ordinal_to_ymd(current_ordinal, current_year, current_month, current_day);

        // Check if this date matches the rrule pattern
        if (is_rrule_match_date(current_year, current_month, current_day,
                                start_year, start_month, start_mday, start_ordinal, rrule_))
        {
            occurrence_count++;

            // Check if this occurrence's time overlaps with the target range
            // Calculate the occurrence's start and end time in Unix seconds
            struct tm base_tm = {};
            if (gmtime_with_timezone(dtstart_, use_tzid, &base_tm))
            {
                // Modify the date part to the current occurrence date
                base_tm.tm_year = current_year - 1900;
                base_tm.tm_mon = current_month - 1;
                base_tm.tm_mday = current_day;

                // Calculate the start time of this occurrence
                int64_t occurrence_start = zcc::timegm(&base_tm);

                // Calculate the end time of this occurrence
                int64_t occurrence_duration = (dtend_ != -1) ? (dtend_ - dtstart_) : 1;
                int64_t occurrence_end = occurrence_start + occurrence_duration;

                // Check if this occurrence overlaps with the target range
                if (!(occurrence_end <= unix_start || occurrence_start >= unix_end))
                {
                    // Found an overlapping occurrence
                    if (rrule_.count != -1 && occurrence_count > rrule_.count)
                    {
                        break; // Exceeded max occurrences
                    }
                    return true;
                }
            }
        }

        // Move to next day
        current_ordinal++;

        // Safety check to prevent infinite loops
        if ((current_ordinal - start_ordinal) > 365 * 10) // More than 10 years
        {
            break;
        }

        if (rrule_.count != -1 && occurrence_count >= rrule_.count)
        {
            break; // Reached max occurrences
        }
    }

    return false;
}

bool ics_calendar_event_matcher::match_day(int day, const std::string &tzid)
{
    int64_t unix_second = day_to_unix(day, tzid);
    if (unix_second == var_invalid_time)
    {
        return false;
    }
    //
    return match_second_scope(unix_second, unix_second + 86400);
}

void ics_calendar_event_matcher::load_from_event(const ics_calendar_event &event)
{
    event_tzid_ = event.tzid;
    dtstart_ = event.dtstart;
    dtend_ = event.dtend;
    rrule_ = event.rrule;
}

void ics_calendar_event_matcher::set_dtstart_and_rrule(int64_t dtstart, const std::string &tzid, const std::string &rrule_rawdata)
{
    dtstart_ = dtstart;
    event_tzid_ = tzid;
    rrule_ = ics_calendar_event_rrule::parse(rrule_rawdata, dtstart, tzid);
}

void ics_calendar_event_matcher::set_dtend(int64_t dtend)
{
    dtend_ = dtend;
}

zcc_namespace_end;
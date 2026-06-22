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

void ics_calendar_timezone::serialize_to_json(json &js)
{
    js["tzid"] = tzid;
    auto nodesjs = js.object_update("nodes", new json(json_type_array), true);
    for (auto &node : nodes)
    {
        auto tmpjs = nodesjs->array_push(new json(json_type_object), true);
        (*tmpjs)["type"] = node.type;
        (*tmpjs)["name"] = node.name;
        (*tmpjs)["tzoffsetto"] = node.tzoffsetto;
        (*tmpjs)["tzoffsetfrom"] = node.tzoffsetfrom;
        (*tmpjs)["dtstart"] = node.dtstart;
        if (!node.rrule_rawdata.empty())
        {
            (*tmpjs)["rrule_rawdata"] = node.rrule_rawdata;
        }
    }
}

void ics_calendar_timezone::unserialize_from_json(json &js)
{
    tzid = js.object_get_string_value("tzid");
    nodes.clear();
    auto nodesjs = js.object_get("nodes");
    if (nodesjs)
    {
        for (auto &nodejs : nodesjs->get_array_value())
        {
            if (!nodejs)
            {
                continue;
            }
            nodes.push_back(ics_calendar_timezone_node());
            auto &node = nodes.back();
            node.type = nodejs->object_get_string_value("type");
            node.name = nodejs->object_get_string_value("name");
            node.tzoffsetto = (int)nodejs->object_get_long_value("tzoffsetto");
            node.tzoffsetfrom = (int)nodejs->object_get_long_value("tzoffsetfrom");
            node.dtstart = nodejs->object_get_long_value("dtstart");
            node.rrule_rawdata = nodejs->object_get_string_value("rrule_rawdata");
            if (!node.rrule_rawdata.empty())
            {
                node.rrule = ics_calendar_event_rrule::parse(node.rrule_rawdata, node.dtstart, tzid);
            }
        }
    }
}

ics_calendar_event_rrule ics_calendar_event_rrule::parse(const std::string &rrule_rawata, int64_t dtstart, const std::string &tzid)
{
    ics_calendar_event_rrule rule;
    // 天
    // RRULE:FREQ=DAILY;COUNT=5;INTERVAL=3
    // RRULE:FREQ=DAILY;INTERVAL=5;UNTIL=20231117
    // RRULE:FREQ=DAILY;BYDAY=MO,TU,WE,TH,FR
    // 周
    // RRULE:FREQ=WEEKLY;COUNT=7;INTERVAL=3;BYDAY=MO,FR
    // RRULE:FREQ=WEEKLY;INTERVAL=2;UNTIL=20240125
    // RRULE:FREQ=WEEKLY;INTERVAL=2
    // RRULE:FREQ=WEEKLY;COUNT=5;INTERVAL=2
    // 月
    // RRULE:FREQ=MONTHLY;COUNT=12;INTERVAL=2;BYMONTHDAY=9,12,27
    // RRULE:FREQ=MONTHLY;COUNT=12;INTERVAL=2;BYDAY=2WE
    // RRULE:FREQ=MONTHLY;COUNT=12;INTERVAL=2;BYMONTHDAY=4
    // RRULE:FREQ=MONTHLY;COUNT=12;INTERVAL=3;BYMONTHDAY=4,-1
    // RRULE:FREQ=MONTHLY;COUNT=12;INTERVAL=3;BYDAY=-1TU
    // 年
    // RRULE:FREQ=YEARLY;COUNT=12;INTERVAL=2;BYMONTH=6;BYMONTHDAY=3
    // RRULE:FREQ=YEARLY;COUNT=12;INTERVAL=2;BYMONTH=6;BYDAY=3TU
    auto ss = split_and_ignore_empty_token(rrule_rawata, ';');
    for (auto &s : ss)
    {
        auto pos = s.find('=');
        if (pos == std::string::npos)
        {
            continue;
        }
        auto key = s.substr(0, pos);
        tolower(key);
        auto value = trim(s.substr(pos + 1));
        if (key == "freq")
        {
            auto tmp = value;
            zcc::tolower(tmp);
            if (tmp == "daily")
            {
                rule.freq = ics_calendar_event_rrule::freq_type::daily;
            }
            else if (tmp == "weekly")
            {
                rule.freq = ics_calendar_event_rrule::freq_type::weekly;
            }
            else if (tmp == "monthly")
            {
                rule.freq = ics_calendar_event_rrule::freq_type::monthly;
            }
            else if (tmp == "yearly")
            {
                rule.freq = ics_calendar_event_rrule::freq_type::yearly;
            }
        }
        else if (key == "count")
        {
            rule.count = zcc::atoi(value);
        }
        else if (key == "interval")
        {
            rule.interval = zcc::atoi(value);
        }
        else if (key == "until")
        {
            rule.until = iso8601_2004_time_from_datetime(value);
        }
        else if (key == "bymonth")
        {
            for (auto i : split_and_ignore_empty_token(value, ','))
            {
                rule.bymonth.push_back(zcc::atoi(i));
            }
        }
        else if (key == "bymonthday")
        {
            for (auto i : split_and_ignore_empty_token(value, ','))
            {
                rule.bymonthday.push_back(zcc::atoi(i));
            }
        }
        else if (key == "byyearday")
        {
            for (auto i : split_and_ignore_empty_token(value, ','))
            {
                rule.byyearday.push_back(zcc::atoi(i));
            }
        }
        else if (key == "byday")
        {
            // BYDAY=MO,2TU,-1WE,FR
            static const std::map<std::string, int> byday_map = {
                {"su", 0},
                {"mo", 1},
                {"tu", 2},
                {"we", 3},
                {"th", 4},
                {"fr", 5},
                {"sa", 6},
            };
            auto tmpValue = value;
            zcc::tolower(tmpValue);
            for (auto wd : split_and_ignore_empty_token(tmpValue, ','))
            {
                ics_calendar_event_rrule_byweekday byweekday;
                if (wd[0] == '-' || isdigit(wd[0]))
                {
                    byweekday.sn = zcc::atoi(wd.c_str());
                }
                const char *ps = wd.c_str();
                while (*ps && !isalpha(*ps))
                {
                    ps++;
                }
                auto it = byday_map.find(ps);
                if (it == byday_map.end())
                {
                    continue;
                }
                byweekday.day = it->second;
                rule.byweekday.push_back(byweekday);
            }
        }
        else if (key == "bysetpos")
        {
            for (auto i : split_and_ignore_empty_token(value, ','))
            {
                rule.bysetpos.push_back(zcc::atoi(i));
            }
        }
        else if (key == "byweekno")
        {
            for (auto i : split_and_ignore_empty_token(value, ','))
            {
                rule.byweekno.push_back(zcc::atoi(i));
            }
        }
    }
    if (rule.freq == ics_calendar_event_rrule::freq_type::weekly)
    {
        if (rule.byweekday.empty())
        {
            std::tm tm;
            if (gmtime_with_timezone(dtstart, tzid, &tm))
            {
                ics_calendar_event_rrule_byweekday byweekday;
                byweekday.day = tm.tm_wday;
                rule.byweekday.push_back(byweekday);
            }
        }
    }
    else if (rule.freq == ics_calendar_event_rrule::freq_type::monthly)
    {
        if (rule.bymonthday.empty() && rule.byweekday.empty())
        {
            std::tm tm;
            if (gmtime_with_timezone(dtstart, tzid, &tm))
            {
                rule.bymonthday.push_back(tm.tm_mday);
            }
        }
    }
    else if (rule.freq == ics_calendar_event_rrule::freq_type::yearly)
    {
        if (rule.byyearday.empty() && rule.bymonth.empty() && rule.bymonthday.empty() && rule.byweekday.empty())
        {
            std::tm tm;
            if (gmtime_with_timezone(dtstart, tzid, &tm))
            {
                rule.bymonth.push_back(tm.tm_mon + 1);
                rule.bymonthday.push_back(tm.tm_mday);
            }
        }
    }
    return rule;
}

std::string ics_calendar_event_rrule::build(const ics_calendar_event_rrule &rrule)
{
    std::string rawData;

    // 必须有有效的频率类型
    if (rrule.freq == freq_type::none)
    {
        return rawData;
    }

    // FREQ
    rawData = "FREQ=" + get_freq_string(rrule.freq);

    // INTERVAL (默认值1时不输出)
    if (rrule.interval != 1)
    {
        rawData += ";INTERVAL=" + std::to_string(rrule.interval);
    }

    // BYSETPOS
    if (!rrule.bysetpos.empty())
    {
        rawData += ";BYSETPOS=" + join(rrule.bysetpos, ",");
    }

    // BYWEEKNO
    if (!rrule.byweekno.empty())
    {
        rawData += ";BYWEEKNO=" + join(rrule.byweekno, ",");
    }

    // COUNT
    if (rrule.count != -1)
    {
        rawData += ";COUNT=" + std::to_string(rrule.count);
    }

    // UNTIL
    if (rrule.until != -1)
    {
        std::tm tm;
        auto *p = gmtime(rrule.until);
        if (p)
        {
            char buf[32];
            snprintf(buf, sizeof(buf), ";UNTIL=%04d%02d%02dT%02d%02d%02dZ", p->tm_year + 1900, p->tm_mon + 1, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
            rawData += buf;
        }
    }

    // BYMONTH
    if (!rrule.bymonth.empty())
    {
        rawData += ";BYMONTH=";
        bool first = true;
        for (auto month : rrule.bymonth)
        {
            if (!first)
            {
                rawData += ",";
            }
            rawData += std::to_string(month);
            first = false;
        }
    }

    // BYMONTHDAY
    if (!rrule.bymonthday.empty())
    {
        rawData += ";BYMONTHDAY=";
        bool first = true;
        for (auto day : rrule.bymonthday)
        {
            if (!first)
            {
                rawData += ",";
            }
            rawData += std::to_string(day);
            first = false;
        }
    }

    // BYYEARDAY
    if (!rrule.byyearday.empty())
    {
        rawData += ";BYYEARDAY=";
        bool first = true;
        for (auto day : rrule.byyearday)
        {
            if (!first)
            {
                rawData += ",";
            }
            rawData += std::to_string(day);
            first = false;
        }
    }

    // BYDAY
    static const std::vector<std::string> day_names = {"SU", "MO", "TU", "WE", "TH", "FR", "SA"};
    if (!rrule.byweekday.empty())
    {
        rawData += ";BYDAY=";
        bool first = true;
        for (auto &wd : rrule.byweekday)
        {
            if (!first)
            {
                rawData += ",";
            }
            if (wd.sn != 0)
            {
                rawData += std::to_string(wd.sn);
            }
            rawData += day_names[wd.day];
            first = false;
        }
    }

    return rawData;
}

std::string ics_calendar_event_rrule::get_freq_string(freq_type freq)
{
    std::string freq_str;
    switch (freq)
    {
    case freq_type::daily:
        freq_str = "DAILY";
        break;
    case freq_type::weekly:
        freq_str = "WEEKLY";
        break;
    case freq_type::monthly:
        freq_str = "MONTHLY";
        break;
    case freq_type::yearly:
        freq_str = "YEARLY";
        break;
    default:
        freq_str = "none";
        break;
    }
    return freq_str;
}

std::string ics_calendar_event_rrule::get_digest()
{
    std::string digest;
    if (freq == freq_type::none)
    {
    }
    else if (freq == freq_type::daily)
    {
        digest += "每";
        if (interval > 1)
        {
            digest += " " + std::to_string(interval) + " ";
        }
        digest += "天";
    }
    else if (freq == freq_type::weekly)
    {
        digest += "每";
        if (interval > 1)
        {
            digest += " " + std::to_string(interval) + " ";
        }
        digest += "周";
    }
    else if (freq == freq_type::monthly)
    {
        digest += "每";
        if (interval > 1)
        {
            digest += " " + std::to_string(interval) + " ";
        }
        digest += "月";
    }
    else if (freq == freq_type::yearly)
    {
        digest += "每";
        if (interval > 1)
        {
            digest += " " + std::to_string(interval) + " ";
        }
        digest += "年";
    }
    if (bymonth.size())
    {
        digest += "; ";
        bool first = true;
        for (auto &month : bymonth)
        {
            if (!first)
            {
                digest += ",";
            }
            else
            {
                first = false;
            }
            digest += std::to_string(month) + "月";
        }
    }
    if (bymonthday.size())
    {
        digest += "; ";
        bool first = true;
        for (auto &day : bymonthday)
        {
            if (!first)
            {
                digest += ",";
            }
            else
            {
                first = false;
            }
            digest += std::to_string(day) + "日";
        }
    }
    if (byweekday.size())
    {
        digest += "; ";
        bool first = true;
        for (auto &day : byweekday)
        {
            if (!first)
            {
                digest += ",";
            }
            else
            {
                first = false;
            }
            if (day.sn == 0)
            {
            }
            else if (day.sn > 0)
            {
                digest += "第 " + std::to_string(day.sn) + " 个";
            }
            else if (day.sn < 0)
            {
                digest += "倒数第 " + std::to_string(0 - day.sn) + " 个";
            }
            digest.append(get_day_name_of_week(day.day));
        }
    }
    if (until > 0)
    {
        digest += "; 直到: " + get_simple_date(until);
    }
    if (count > 0)
    {
        digest += "; 重复: " + std::to_string(count) + " 次";
    }
    return digest;
}

void ics_calendar_event_alarm::serialize_to_json(json &js)
{
#define ALARM_FIELD(field)      \
    {                           \
        if (!field.empty())     \
        {                       \
            js[#field] = field; \
        }                       \
    }
    json *tmpjs = js.object_update("trigger", new json(json_type_object), true);
    (*tmpjs)["stamp"] = trigger.stamp;
    (*tmpjs)["isTimePoint"] = trigger.isTimePoint;
    (*tmpjs)["isRelated"] = trigger.isRelated;
    (*tmpjs)["isBeforeOrAfter"] = trigger.isBeforeOrAfter;
    (*tmpjs)["isBeginOrEnd"] = trigger.isBeginOrEnd;
    ALARM_FIELD(action)
    ALARM_FIELD(description)
    ALARM_FIELD(duration)
    ALARM_FIELD(attach)
    if (attendees.size())
    {
        tmpjs = js.object_update("attendees", new json(json_type_array), true);
        for (auto &a : attendees)
        {
            (*tmpjs).array_add(a);
        }
    }
    js["repeat"] = repeat;
#undef ALARM_FIELD
}

void ics_calendar_event_alarm::unserialize_from_json(json &js)
{
    json *tmpjs = js.object_get("trigger");
    if (tmpjs)
    {
        trigger.stamp = tmpjs->object_get_long_value("stamp");
        trigger.isTimePoint = tmpjs->object_get_bool_value("isTimePoint");
        trigger.isRelated = tmpjs->object_get_bool_value("isRelated");
        trigger.isBeforeOrAfter = tmpjs->object_get_bool_value("isBeforeOrAfter");
        trigger.isBeginOrEnd = tmpjs->object_get_bool_value("isBeginOrEnd");
    }
    else
    {
        trigger.stamp = -1;
        trigger.isTimePoint = false;
        trigger.isRelated = true;
        trigger.isBeforeOrAfter = true;
        trigger.isBeginOrEnd = true;
    }
    action = js.object_get_string_value("action");
    description = js.object_get_string_value("description");
    duration = js.object_get_string_value("duration");
    attach = js.object_get_string_value("attach");
    tmpjs = js.object_get("attendees");
    if (tmpjs)
    {
        for (auto &a : tmpjs->get_array_value())
        {
            if (a)
            {
                attendees.push_back(a->get_string_value());
            }
        }
    }
    repeat = (int)js.object_get_long_value("repeat");
}

std::string ics_calendar_event_alarm::get_digest()
{
    std::string digest;
    if (trigger.isTimePoint)
    {
        digest = "时间点:";
        digest += " " + simple_date_time_with_second(trigger.stamp);
        return digest;
    }
    digest = "在";
    if (trigger.isBeginOrEnd)
    {
        digest += "开始";
    }
    else
    {
        digest += "结束";
    }
    if (trigger.isBeforeOrAfter)
    {
        digest += "前";
    }
    else
    {
        digest += "后";
    }
    if (trigger.stamp > 3600)
    {
        digest += " " + std::to_string(trigger.stamp / 3600) + "小时";
        auto minutes = trigger.stamp % 3600 / 60;
        if (minutes > 0)
        {
            digest += "" + std::to_string(minutes) + "分钟";
        }
    }
    else
    {
        digest += " " + std::to_string(trigger.stamp / 60) + "分钟";
        auto seconds = trigger.stamp % 60;
        if (seconds > 0)
        {
            digest += "" + std::to_string(seconds) + "秒";
        }
    }
    return digest;
}

//
void ics_calendar_event_attendee::serialize_to_json(json &js)
{
    js.used_for_object();
#define ATTENDEE_FIELD(field)   \
    {                           \
        if (!field.empty())     \
        {                       \
            js[#field] = field; \
        }                       \
    }

    ATTENDEE_FIELD(email);
    ATTENDEE_FIELD(partstat);
    ATTENDEE_FIELD(cn);
    ATTENDEE_FIELD(role);
    ATTENDEE_FIELD(rsvp);
    ATTENDEE_FIELD(cutype);
    ATTENDEE_FIELD(delegated_from);
    ATTENDEE_FIELD(delegated_to);
    ATTENDEE_FIELD(sent_by);
#undef ATTENDEE_FIELD
}

void ics_calendar_event_attendee::unserialize_from_json(json &js)
{
    email = js.object_get_string_value("email");
    partstat = js.object_get_string_value("partstat");
    cn = js.object_get_string_value("cn");
    role = js.object_get_string_value("role");
    rsvp = js.object_get_string_value("rsvp");
    cutype = js.object_get_string_value("cutype");
    delegated_from = js.object_get_string_value("delegated_from");
    delegated_to = js.object_get_string_value("delegated_to");
    sent_by = js.object_get_string_value("sent_by");
}

//
void ics_calendar_event::serialize_to_json(json &js)
{
    json *tmpjs;
    js.used_for_object();
    js["PRODID"] = ics_calender_prodid;
    js["VERSION"] = ics_calender_version;
    js["METHOD"] = ics_calender_method;
    js["CALSCALE"] = ics_calender_calscale;
    js["uid"] = uid;
    js["organizer"] = organizer;
    js["organizer_name"] = organizer_name;
    js["summary"] = summary;
    js["location"] = location;
    js["description_plain"] = description_plain;
    js["description_html"] = description_html;
    js["transp"] = transp;
    js["status"] = status;
    js["tzid"] = tzid;
    js["dtstamp"] = dtstamp;
    js["dtstart"] = dtstart;
    js["dtend"] = dtend;
    js["created"] = created;
    js["last_modified"] = last_modified;
    js["sequence"] = sequence;
    js["priority"] = priority;
    js["CLASS"] = CLASS;
    js["categories"] = categories;
    js["rrule_rawdata"] = rrule_rawdata;
    if (rrule_rawdata.empty() && rrule.is_valid())
    {
        js["rrule_rawdata"] = rrule.build();
    }
    js["isMeeting"] = isMeeting;
    js["isMeetingOwner"] = isMeetingOwner;
    js["isAllDay"] = isAllDay;
    js["isRecurrence"] = isRecurrence;
    //
    tmpjs = js.object_update("attendees", new json(json_type_array), true);
    for (auto &a : attendees)
    {
        auto attendee_js = new json(json_type_object);
        a.serialize_to_json(*attendee_js);
        tmpjs->array_add(attendee_js);
    }
    //
    tmpjs = js.object_update("alarms", new json(json_type_array), true);
    for (auto &a : alarms)
    {
        auto alarm_js = new json(json_type_object);
        a.serialize_to_json(*alarm_js);
        tmpjs->array_add(alarm_js);
    }
}

void ics_calendar_event::unserialize_from_json(json &js)
{
    json *tmpjs;
    ics_calender_prodid = js.object_get_string_value("PRODID");
    ics_calender_version = js.object_get_string_value("VERSION");
    ics_calender_method = js.object_get_string_value("METHOD");
    ics_calender_calscale = js.object_get_string_value("CALSCALE");
    uid = js.object_get_string_value("uid");
    organizer = js.object_get_string_value("organizer");
    organizer_name = js.object_get_string_value("organizer_name");
    summary = js.object_get_string_value("summary");
    location = js.object_get_string_value("location");
    description_plain = js.object_get_string_value("description_plain");
    description_html = js.object_get_string_value("description_html");
    transp = js.object_get_string_value("transp");
    status = js.object_get_string_value("status");
    tzid = js.object_get_string_value("tzid");
    dtstamp = js.object_get_long_value("dtstamp");
    dtstart = js.object_get_long_value("dtstart");
    dtend = js.object_get_long_value("dtend");
    created = js.object_get_long_value("created");
    last_modified = js.object_get_long_value("last_modified");
    sequence = (int)js.object_get_long_value("sequence");
    priority = (int)js.object_get_long_value("priority");
    CLASS = js.object_get_string_value("CLASS");
    categories = js.object_get_string_value("categories");
    rrule_rawdata = js.object_get_string_value("rrule_rawdata");
    isMeeting = js.object_get_bool_value("isMeeting");
    isMeetingOwner = js.object_get_bool_value("isMeetingOwner");
    isAllDay = js.object_get_bool_value("isAllDay");
    isRecurrence = js.object_get_bool_value("isRecurrence");
    //
    tmpjs = js.object_get("attendees");
    if (tmpjs)
    {
        for (auto &a : tmpjs->get_array_value())
        {
            if (a)
            {
                ics_calendar_event_attendee attendee;
                attendee.unserialize_from_json(*a);
                attendees.push_back(attendee);
            }
        }
    }
    //
    tmpjs = js.object_get("alarms");
    if (tmpjs)
    {
        for (auto &a : tmpjs->get_array_value())
        {
            if (a)
            {
                ics_calendar_event_alarm alarm;
                alarm.unserialize_from_json(*a);
                alarms.push_back(alarm);
            }
        }
    }
    //
    rrule = ics_calendar_event_rrule::parse(rrule_rawdata, dtstart, tzid);
}

std::string ics_calendar_event::serialize_to_json_string()
{
    json js;
    serialize_to_json(js);
    return js.serialize();
}

void ics_calendar_event::unserialize_from_json_string(const std::string &json_string)
{
    json js;
    js.unserialize(json_string);
    unserialize_from_json(js);
}
std::string ics_calendar_event::get_time_digest()
{
    char buf[128 + 1];
    auto p = localtime(dtstart);
    auto span = dtend - dtstart;
    std::string t;
    if (p)
    {
        std::strftime(buf, 128, "%Y-%m-%d", p);
        t.append(buf);
        t.append("(").append(get_day_name_of_week(p->tm_wday)).append(") ");
        std::strftime(buf, 128, "%H:%M", p);
        t.append(buf);
    }

    if (isAllDay)
    {
        if (span <= 86400)
        {
            if (!p)
            {
                t += " 全天";
            }
            else
            {
                t = "";
                std::strftime(buf, 128, "%Y-%m-%d", p);
                t.append(buf);
                t.append(" 全天");
            }
        }
        else
        {
            auto te = zcc::get_simple_date(dtend);
            t += " 至 " + te.substr(5);
        }
    }
    else if (span <= 86400)
    {
        t += " 至 " + zcc::get_hh_mm(dtend);
    }
    else
    {
        t += " 至 " + zcc::get_simple_date_time(dtend);
    }
    return t;
}

std::string ics_calendar_event::get_alarm_digest()
{
    std::string t = "";
    for (auto &a : alarms)
    {
        t += a.get_digest() + ",";
    }
    if (!t.empty())
    {
        t.pop_back();
    }
    return t;
}

std::string ics_calendar_event::get_rrule_digest()
{
    if (rrule.is_valid())
    {
        return rrule.get_digest();
    }
    return "";
}

std::string ics_calendar_event::get_attendee_digest(const ics_calendar_event_attendee &attendee)
{
    std::string digest;
    auto partstat = attendee.partstat;
    zcc::tolower(partstat);
    auto st = status;
    zcc::tolower(st);
    if (partstat == "accepted")
    {
        digest += "已接受";
    }
    else if (partstat == "declined")
    {
        digest += "已拒绝";
    }
    else if (partstat == "tentative")
    {
        digest += "待定";
    }
    else if (partstat == "needs-action")
    {
        digest += "未操作";
    }
    else if (st == "confirmed")
    {
        digest += "已确认";
    }
    else if (st == "tentative")
    {
        digest += "待定";
    }
    else if (st == "cancelled")
    {
        digest += "已取消";
    }
    return digest;
}

//
ics_calendar::ics_calendar()
{
}

ics_calendar::~ics_calendar()
{
}

void ics_calendar::parse(const char *text, int text_len)
{
    std::string tmpValue;
    auto top_node = begin_end_text_node::parse(text, text_len);
    //
    for (auto &node : top_node->children_)
    {
        auto &key = node->key_;
        if (key == "begin")
        {
            tmpValue = node->value_;
            zcc::tolower(tmpValue);
            if (tmpValue == "vtimezone")
            {
                parse_vtimezone(node);
            }
        }
        else if (key == "version")
        {
            version_ = node->value_;
            zcc::tolower(version_);
        }
        else if (key == "prodid")
        {
            prodid_ = node->value_;
        }
        else if (key == "method")
        {
            method_ = node->value_;
            zcc::tolower(method_);
        }
        else if (key == "calscale")
        {
            calscale_ = node->value_;
            zcc::tolower(calscale_);
        }
    }
    //
    for (auto &node : top_node->children_)
    {
        auto &key = node->key_;
        if (key == "begin")
        {
            tmpValue = node->value_;
            zcc::tolower(tmpValue);
            if (tmpValue == "vevent")
            {
                parse_vevent(node);
            }
            else if (tmpValue == "vtodo")
            {
                parse_vtodo(node);
            }
            else if (tmpValue == "vjournal")
            {
                parse_vjournal(node);
            }
        }
    }
    //
    delete top_node;
}

void ics_calendar::parse_vtimezone(begin_end_text_node *node)
{
    timezone_.push_back(ics_calendar_timezone());
    auto &tz = timezone_.back();
    for (auto &child : node->children_)
    {
        auto &key = child->key_;
        if (key == "tzid")
        {
            tz.tzid = child->value_;
            if (ignore_parse_timezone_)
            {
                break;
            }
            continue;
        }
        if (ignore_parse_timezone_)
        {
            continue;
        }
        if (key == "begin")
        {
            std::string type = child->value_;
            zcc::tolower(type);
            if (type == "standard")
            {
                parse_timezone_node(child, tz, type);
            }
            else if (type == "daylight")
            {
                parse_timezone_node(child, tz, type);
            }
        }
    }
}

void ics_calendar::parse_timezone_node(begin_end_text_node *node, ics_calendar_timezone &timezone, const std::string &type)
{
    timezone.nodes.push_back(ics_calendar_timezone_node());
    auto &tz_node = timezone.nodes.back();
    tz_node.type = type;

    for (auto &child : node->children_)
    {
        auto &key = child->key_;
        if (key == "name")
        {
            tz_node.name = parse_simple_string(child);
        }
        else if (key == "tzoffsetto")
        {
            tz_node.tzoffsetto = timezone_0800_offset(child->value_);
        }
        else if (key == "tzoffsetfrom")
        {
            tz_node.tzoffsetfrom = timezone_0800_offset(child->value_);
        }
        else if (key == "dtstart")
        {
            tz_node.dtstart = parse_simple_time(child);
        }
        else if (key == "rrule")
        {
            tz_node.rrule_rawdata = child->value_;
            tz_node.rrule = ics_calendar_event_rrule::parse(child->value_, tz_node.dtstart, timezone.tzid);
        }
    }
}

void ics_calendar::parse_vevent(begin_end_text_node *node)
{
    begin_end_text_node *rrule_node = nullptr;
    events_.push_back(ics_calendar_event());
    ics_calendar_event &event = events_.back();
    event.ics_calender_prodid = prodid_;
    event.ics_calender_version = version_;
    event.ics_calender_method = method_;
    event.ics_calender_calscale = calscale_;
    for (auto &node : node->children_)
    {
        auto &key = node->key_;
        if (key == "uid")
        {
            event.uid = node->value_;
        }
        else if (key == "organizer")
        {
            event.organizer = begin_end_text_node::get_mailto_value(node->value_);
            event.organizer_name = node->get_param_value("cn");
        }
        else if (key == "summary")
        {
            event.summary = parse_simple_string(node);
        }
        else if (key == "summary")
        {
            event.summary = parse_simple_string(node);
        }
        else if (key == "location")
        {
            event.location = parse_simple_string(node);
        }
        else if (key == "description")
        {
            parse_description(node, event);
        }
        else if (key == "created")
        {
            event.created = parse_simple_time(node);
        }
        else if (key == "last-modified")
        {
            event.last_modified = parse_simple_time(node);
        }
        else if (key == "dtstamp")
        {
            event.dtstamp = parse_simple_time(node);
        }
        else if (key == "dtstart")
        {
            event.dtstart = parse_simple_time(node);
            if (event.tzid.empty())
            {
                event.tzid = node->get_param_value("tzid");
            }
            event.isAllDay = parse_is_all_day(node);
        }
        else if (key == "dtend")
        {
            event.dtend = parse_simple_time(node);
            if (event.tzid.empty())
            {
                event.tzid = node->get_param_value("tzid");
            }
        }
        else if (key == "sequence")
        {
            event.sequence = zcc::atoi(node->value_);
        }
        else if (key == "priority")
        {
            event.priority = zcc::atoi(node->value_);
        }
        else if (key == "class")
        {
            event.CLASS = node->value_;
        }
        else if (key == "categories")
        {
            event.categories = node->value_;
        }
        else if (key == "transp")
        {
            event.transp = node->value_;
        }
        else if (key == "status")
        {
            event.status = node->value_;
        }
        else if (key == "rrule")
        {
            rrule_node = node;
        }
        else if (key == "attendee")
        {
            parse_attendee(node, event);
        }
        else if (key == "begin")
        {
            std::string tmpValue = node->value_;
            zcc::tolower(tmpValue);
            if (tmpValue == "valarm")
            {
                parse_valarm(node, event);
            }
        }
        else if (key == "x-apple-html-descrip" || key == "x-google-html-desc" || key == "ical-html")
        {
            if (event.description_html.empty())
            {
                auto tmp = node->get_param_value("value");
                zcc::tolower(tmp);
                if (tmp == "text")
                {
                    event.description_html = parse_simple_string(node);
                }
            }
        }
        else if (key == "x-alt-desc")
        {
            if (event.description_html.empty())
            {
                auto tmp = node->get_param_value("fmttype");
                zcc::tolower(tmp);
                if (tmp == "text/html")
                {
                    event.description_html = parse_simple_string(node);
                }
            }
        }
    }
    //
    if (rrule_node)
    {
        parse_rrule(rrule_node, event);
    }
    parse_check_is_meeting(event);
}

void ics_calendar::parse_valarm(begin_end_text_node *node, ics_calendar_event &event)
{
    if (ignore_parse_alarm_)
    {
        return;
    }
    event.alarms.push_back(ics_calendar_event_alarm());
    ics_calendar_event_alarm &alarm = event.alarms.back();
    for (auto &node : node->children_)
    {
        auto &key = node->key_;
        if (key == "action")
        {
            alarm.action = node->value_;
        }
        else if (key == "trigger")
        {
            parse_valarm_trigger(node, alarm);
        }
        else if (key == "description")
        {
            alarm.description = parse_simple_string(node);
        }
        else if (key == "duration")
        {
            alarm.duration = node->value_;
        }
        else if (key == "repeat")
        {
            alarm.repeat = zcc::atoi(node->value_);
        }
        else if (key == "attendee")
        {
            auto email = begin_end_text_node::get_mailto_value(node->value_);
            if (!email.empty())
            {
                alarm.attendees.push_back(email);
            }
        }
        else if (key == "attach")
        {
            alarm.attach = node->value_;
        }
    }
}

void ics_calendar::parse_valarm_trigger(begin_end_text_node *node, ics_calendar_event_alarm &alarm)
{
    // TRIGGER:-PT1M
    // TRIGGER:PT2H
    // TRIGGER;RELATED=END:-P3D
    // TRIGGER;RELATED=END:PT4M
    // TRIGGER;VALUE=DATE-TIME:20231026T072500Z

    auto value_type = node->get_param_value("value");
    tolower(value_type);
    if (!value_type.empty())
    {
        alarm.trigger.stamp = parse_simple_time(node);
        alarm.trigger.isTimePoint = true;
        return;
    }
    //
    alarm.trigger.isRelated = true;
    //
    alarm.trigger.isBeginOrEnd = true;
    auto related = node->get_param_value("related");
    zcc::tolower(related);
    if (related == "end")
    {
        alarm.trigger.isBeginOrEnd = false;
    }
    //
    auto value = node->value_;
    zcc::tolower(value);
    if (value.empty())
    {
        return;
    }
    const char *ps = value.c_str();
    if (*ps == '-')
    {
        alarm.trigger.isBeforeOrAfter = true;
        ps++;
    }
    else
    {
        alarm.trigger.isBeforeOrAfter = false;
    }
    while (*ps)
    {
        if (*ps == 'p' || *ps == 't')
        {
            ps++;
            continue;
        }
        break;
    }
    if (!*ps)
    {
        return;
    }
    alarm.trigger.stamp = str_to_second(ps);
}

void ics_calendar::parse_check_is_meeting(ics_calendar_event &event)
{
    event.isMeeting = false;
    if (event.attendees.size() > 0)
    {
        event.isMeeting = true;
    }
    else if (method_ == "reply")
    {
        event.isMeeting = true;
    }
    else if (method_ == "cancel")
    {
        event.isMeeting = true;
    }
    //
    if (event.isMeeting)
    {
        event.isMeetingOwner = false;
        if (method_ == "request")
        {
            event.isMeetingOwner = true;
        }
        else if (method_ == "publish")
        {
            event.isMeetingOwner = true;
        }
        else if (method_ == "refresh")
        {
            event.isMeetingOwner = false;
        }
        else if (method_ == "cancel")
        {
            event.isMeetingOwner = true;
        }
    }
}

bool ics_calendar::parse_is_all_day(begin_end_text_node *node)
{
    auto value = node->get_param_value("value");
    zcc::tolower(value);
    if (value == "date")
    {
        return true;
    }
    //
    value = node->value_;
    if (value.size() == 8)
    {
        return true;
    }
    //
    return false;
}

void ics_calendar::parse_check_is_recurrence(ics_calendar_event &event)
{
    event.isRecurrence = false;
    if (event.rrule.is_valid())
    {
        event.isRecurrence = true;
    }
}

void ics_calendar::parse_vtodo(begin_end_text_node *node)
{
}

void ics_calendar::parse_vjournal(begin_end_text_node *node)
{
}

std::string ics_calendar::parse_simple_string(begin_end_text_node *node)
{
    auto language = node->get_param_value("language");
    if (language.empty())
    {
        return node->value_;
    }
    else
    {
        return zcc::charset::convert_to_utf8(language, node->value_);
    }
}

int64_t ics_calendar::parse_simple_time(begin_end_text_node *node)
{
    std::string node_tzid = node->get_param_value("tzid");
    if (node_tzid.empty())
    {
        if (timezone_.size() > 0)
        {
            node_tzid = timezone_[0].tzid;
        }
    }
    std::string value_type = node->get_param_value("value");
    tolower(value_type);
    //
    std::string value = node->value_;
    if (value.empty())
    {
        return -1;
    }
    int64_t sec = 0;
    if (value_type == "time")
    {
        sec = iso8601_2004_time_from_time(value, node_tzid);
    }
    else // date, date-time
    {
        sec = iso8601_2004_time_from_datetime(value, node_tzid);
    }
    return sec;
}

void ics_calendar::parse_description(begin_end_text_node *node, ics_calendar_event &event)
{
    event.description_plain = parse_simple_string(node);
    auto altrep = node->get_param_value("altrep");
    if (!starts_with(altrep, "data:text/html,"))
    {
        return;
    }
    std::string tmpValue;
    auto language = node->get_param_value("language");
    if (language.empty())
    {
        tmpValue = altrep.substr(15);
    }
    else
    {
        tmpValue = zcc::charset::convert_to_utf8(language, altrep.substr(15));
    }
    event.description_html = url_token_decode(tmpValue);
}

void ics_calendar::parse_rrule(begin_end_text_node *node, ics_calendar_event &event)
{
    event.rrule_rawdata = node->value_;
    if (!ignore_parse_rrule_)
    {
        event.rrule = ics_calendar_event_rrule::parse(event.rrule_rawdata, event.dtstart, event.tzid);
    }
}

void ics_calendar::parse_attendee(begin_end_text_node *node, ics_calendar_event &event)
{
    event.attendees.push_back(ics_calendar_event_attendee());
    auto &attendee = event.attendees.back();
    //
    attendee.email = begin_end_text_node::get_mailto_value(node->value_);
    //
    attendee.cn = node->get_param_value("cn");
    attendee.role = node->get_param_value("role");
    attendee.partstat = node->get_param_value("partstat");
    attendee.rsvp = node->get_param_value("rsvp");
    attendee.cutype = node->get_param_value("cutype");
    attendee.delegated_from = node->get_param_value("delegated-from");
    attendee.delegated_to = node->get_param_value("delegated-to");
    attendee.sent_by = node->get_param_value("sent-by");
}

std::string ics_calendar::debug_info()
{
    std::string info;
    info += "version: " + version_ + "\n";
    info += "prodid: " + prodid_ + "\n";
    info += "method: " + method_ + "\n";
    for (auto &node : timezone_)
    {
        info += "timezone:\n";
        zcc::json js;
        node.serialize_to_json(js);
        auto nodesjs = js.object_get("nodes");
        if (nodesjs)
        {
            for (auto &njs : nodesjs->get_array_value())
            {
                if (!njs)
                {
                    continue;
                }
                auto rrrule_rawdata = njs->object_get_string_value("rrule_rawdata");
                if (!rrrule_rawdata.empty())
                {
                    auto rrule = ics_calendar_event_rrule::parse(rrrule_rawdata, njs->object_get_long_value("dtstart"), node.tzid);
                    (*njs)["rrule_digest"] = rrule.get_digest();
                }
            }
        }
        info += js.serialize(json_serialize_pretty) + "\n";
    }
    for (auto &event : events_)
    {
        info += "\nevent:\n";
        zcc::json js;
        event.serialize_to_json(js);
        if (!event.rrule_rawdata.empty())
        {
            ics_calendar_event_rrule rrule = ics_calendar_event_rrule::parse(event.rrule_rawdata, event.dtstart, event.tzid);
            js["rrule_digest"] = rrule.get_digest();
        }
        info += js.serialize(json_serialize_pretty) + "\n";
    }
    return info;
}

// 格式化时区偏移量
static std::string format_timezone_offset(int offset_seconds)
{
    char buffer[16];
    int hours = offset_seconds / 3600;
    int minutes = (offset_seconds % 3600) / 60;

    if (offset_seconds >= 0)
    {
        snprintf(buffer, sizeof(buffer), "+%02d%02d", hours, minutes);
    }
    else
    {
        snprintf(buffer, sizeof(buffer), "-%02d%02d", abs(hours), minutes);
    }

    return std::string(buffer);
}

// 格式化 ICS 时间
static std::string serialize_to_ics_string_format_time_line(const std::string &name, int64_t timestamp, const std::string &tzid, bool is_date_only = false)
{
    std::string newTzid;
    std::string r = name;
    std::tm *tm;
    tm = gmtime_with_timezone(timestamp, tzid);
    if (!tm)
    {
        tm = gmtime(timestamp);
    }
    if (!tm)
    {
        return "";
    }
    if (!tzid.empty())
    {
        newTzid = tzid;
        tm = gmtime_with_timezone(timestamp, tzid);
        if (!tm)
        {
            newTzid = get_current_timezone();
            tm = gmtime(timestamp);
            if (!tm)
            {
                r.append(":");
                return r;
            }
        }
        r.append(";").append("TZID=" + newTzid);
    }
    if (is_date_only)
    {
        // 仅日期格式：YYYYMMDD
        r.append(";VALUE=DATE");
        sprintf_1024(r, ":%04d%02d%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
    }
    else
    {
        // 完整时间格式：YYYYMMDDTHHMMSSZ
        sprintf_1024(r, ":%04d%02d%02dT%02d%02d%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
        if (newTzid.empty())
        {
            r.append("Z");
        }
    }

    return r;
}

static std::string serialize_to_ics_string_get_upper(const std::string &s)
{
    std::string r = s;
    zcc::toupper(r);
    return r;
}

static std::string serialize_to_ics_string_escape_param_value(const std::string s)
{
    std::string r;
    const unsigned char *ps = (const unsigned char *)s.c_str();
    while (*ps)
    {
        int ch = *ps++;
        if (ch == '^')
        {
            r.append("^^");
        }
        else if (ch == '"')
        {
            r.append("^'");
        }
        else if (ch == '\r')
        {
        }
        else if (ch == '\n')
        {
        }
        else
        {
            r.push_back(ch);
        }
    }
    return r;
}

// 转义 ICS 文本中的特殊字符
static std::string serialize_to_ics_string_escape_text_value(const std::string &text)
{
    std::string result;

    for (char c : text)
    {
        switch (c)
        {
        case '\\':
            result += "\\\\";
            break;
        case ';':
            result += "\\;";
            break;
        case ',':
            result += "\\,";
            break;
        case '\n':
            result += "\\n";
            break;
        case '\r':
            result += "\\r";
            break;
        default:
            result += c;
            break;
        }
    }

    return result;
}

static std::string serialize_to_ics_string_escape_description(std::string s, bool html_mode)
{
    std::string r;
    std::string line;
    const unsigned char *ps = (const unsigned char *)s.c_str();
    while (*ps)
    {
        int len = charset::utf8_len(ps);
        line.append((const char *)ps, len);
        ps += len;
        if (line.size() > 76)
        {
            if (!r.empty())
            {
                r.append(" ");
            }
            if (html_mode)
            {
                r.append(url_token_encode(line, true)).append("\r\n");
            }
            else
            {
                r.append(serialize_to_ics_string_escape_text_value(line)).append("\r\n");
            }
            line = "";
        }
    }
    if (line.size())
    {
        if (!r.empty())
        {
            r.append(" ");
        }
        if (html_mode)
        {
            r.append(url_token_encode(line, true)).append("\r\n");
        }
        else
        {
            r.append(serialize_to_ics_string_escape_text_value(line)).append("\r\n");
        }
        line = "";
    }
    trim_line_end_rn(r);
    return r;
}

std::string ics_calendar::serialize_to_ics_string()
{
    std::string result;

    // 添加 ICS 文件头
    result += "BEGIN:VCALENDAR\r\n";

    // 添加版本信息
    if (!version_.empty())
    {
        result += "VERSION:" + version_ + "\r\n";
    }
    else
    {
        result += "VERSION:2.0\r\n";
    }

    // 添加产品标识
    if (!prodid_.empty())
    {
        result += "PRODID:" + prodid_ + "\r\n";
    }
    else
    {
        result += "PRODID:-//ZCC//Calendar//EN\r\n";
    }

    // 添加日历方法
    if (!method_.empty())
    {
        result += "METHOD:" + serialize_to_ics_string_get_upper(method_) + "\r\n";
    }

    // 添加日历刻度
    if (!calscale_.empty())
    {
        result += "CALSCALE:" + serialize_to_ics_string_get_upper(calscale_) + "\r\n";
    }

    // 添加时区信息
    for (auto &tz : timezone_)
    {
        result += "BEGIN:VTIMEZONE\r\n";
        result += "TZID:" + tz.tzid + "\r\n";

        for (auto &node : tz.nodes)
        {
            if (node.type == "standard")
            {
                result += "BEGIN:STANDARD\r\n";
            }
            else if (node.type == "daylight")
            {
                result += "BEGIN:DAYLIGHT\r\n";
            }
            else
            {
                continue;
            }

            if (!node.name.empty())
            {
                result += "NAME:" + node.name + "\r\n";
            }

            if (node.dtstart != -1)
            {
                result += serialize_to_ics_string_format_time_line("DTSTART", node.dtstart, tz.tzid) + "\r\n";
            }

            if (node.tzoffsetfrom != -1)
            {
                result += "TZOFFSETFROM:" + format_timezone_offset(node.tzoffsetfrom) + "\r\n";
            }

            if (node.tzoffsetto != -1)
            {
                result += "TZOFFSETTO:" + format_timezone_offset(node.tzoffsetto) + "\r\n";
            }

            if (!node.rrule_rawdata.empty())
            {
                result += "RRULE:" + node.rrule_rawdata + "\r\n";
            }

            if (node.type == "standard")
            {
                result += "END:STANDARD\r\n";
            }
            else if (node.type == "daylight")
            {
                result += "END:DAYLIGHT\r\n";
            }
        }

        result += "END:VTIMEZONE\r\n";
    }

    // 添加事件
    for (auto &event : events_)
    {
        result += "BEGIN:VEVENT\r\n";

        // 必需字段
        if (!event.uid.empty())
        {
            result += "UID:" + event.uid + "\r\n";
        }

        result += "DTSTAMP:" + unix_to_iso8601_utc(event.dtstamp == -1 ? second() : event.dtstamp) + "\r\n";

        if (event.dtstart != -1)
        {
            result += serialize_to_ics_string_format_time_line("DTSTART", event.dtstart, event.tzid, event.isAllDay) + "\r\n";
        }

        if (event.dtend != -1)
        {
            result += serialize_to_ics_string_format_time_line("DTEND", event.dtend, event.tzid, event.isAllDay) + "\r\n";
        }

        // 可选字段
        if (!event.summary.empty())
        {
            result += "SUMMARY:" + serialize_to_ics_string_escape_text_value(event.summary) + "\r\n";
        }

        if (1)
        {
            result += "DESCRIPTION";
            if (!event.description_html.empty())
            {
                result += ";ALTREP=\"data:text/html," + serialize_to_ics_string_escape_description(event.description_html, true) + "\"";
            }
            result += ":" + serialize_to_ics_string_escape_description(event.description_plain, false) + "\r\n";
        }

        if (!event.location.empty())
        {
            result += "LOCATION:" + serialize_to_ics_string_escape_text_value(event.location) + "\r\n";
        }

        if (!event.status.empty())
        {
            result += "STATUS:" + serialize_to_ics_string_get_upper(event.status) + "\r\n";
        }

        if (!event.transp.empty())
        {
            result += "TRANSP:" + serialize_to_ics_string_get_upper(event.transp) + "\r\n";
        }

        if (event.sequence != -1)
        {
            result += "SEQUENCE:" + std::to_string(event.sequence) + "\r\n";
        }

        if (event.priority != -1)
        {
            result += "PRIORITY:" + std::to_string(event.priority) + "\r\n";
        }
        if (event.CLASS != "")
        {
            result += "CLASS:" + serialize_to_ics_string_get_upper(event.CLASS) + "\r\n";
        }

        if (event.categories != "")
        {
            result += "CATEGORIES:" + serialize_to_ics_string_escape_text_value(event.categories) + "\r\n";
        }

        result += "CREATED:" + unix_to_iso8601_utc(event.created == -1 ? second() : event.created) + "\r\n";

        result += "LAST-MODIFIED:" + unix_to_iso8601_utc(event.last_modified == -1 ? second() : event.last_modified) + "\r\n";

        if (!event.organizer.empty())
        {
            std::string organizer_line = "ORGANIZER";
            if (!event.organizer_name.empty())
            {
                organizer_line += ";CN=\"" + serialize_to_ics_string_escape_param_value(event.organizer_name) + "\"";
            }
            organizer_line += ":mailto:" + event.organizer + "\r\n";
            result += organizer_line;
        }

        // 重复规则
        if (!event.rrule_rawdata.empty())
        {
            result += "RRULE:" + event.rrule_rawdata + "\r\n";
        }
        else if (event.rrule.is_valid())
        {
            result += "RRULE:" + event.rrule.build() + "\r\n";
        }

        // 参与者
        for (auto &attendee : event.attendees)
        {
            std::string attendee_line = "ATTENDEE";
            if (!attendee.cn.empty())
            {
                attendee_line += ";CN=\"" + serialize_to_ics_string_escape_param_value(attendee.cn) + "\"";
            }
            if (!attendee.role.empty())
            {
                attendee_line += ";ROLE=" + serialize_to_ics_string_get_upper(attendee.role);
            }
            if (!attendee.partstat.empty())
            {
                attendee_line += ";PARTSTAT=" + serialize_to_ics_string_get_upper(attendee.partstat);
            }
            if (!attendee.rsvp.empty())
            {
                attendee_line += ";RSVP=" + serialize_to_ics_string_get_upper(attendee.rsvp);
            }
            if (!attendee.cutype.empty())
            {
                attendee_line += ";CUTYPE=" + serialize_to_ics_string_get_upper(attendee.cutype);
            }
            attendee_line += ":mailto:" + attendee.email + "\r\n";
            result += attendee_line;
        }

        // 提醒
        for (auto &alarm : event.alarms)
        {
            result += "BEGIN:VALARM\r\n";

            if (!alarm.action.empty())
            {
                result += "ACTION:" + alarm.action + "\r\n";
            }

            if (alarm.trigger.stamp != -1)
            {
                std::string trigger_line = "TRIGGER";
                if (alarm.trigger.isTimePoint)
                {
                    trigger_line += ";VALUE=DATE-TIME";
                    trigger_line += ":" + unix_to_iso8601_utc(alarm.trigger.stamp) + "\r\n";
                }
                else if (alarm.trigger.isRelated)
                {
                    trigger_line += ";RELATED=" + std::string(alarm.trigger.isBeginOrEnd ? "START" : "END");
                    trigger_line += ":";
                    if (alarm.trigger.isBeforeOrAfter)
                    {
                        trigger_line += "-";
                    }
                    if (alarm.trigger.stamp < 60)
                    {
                        trigger_line += "PT1M";
                    }
                    else if (alarm.trigger.stamp < 3600)
                    {
                        trigger_line += "PT" + std::to_string(alarm.trigger.stamp / 60) + "M";
                    }
                    else if (alarm.trigger.stamp < 3600 * 24)
                    {
                        trigger_line += "PT" + std::to_string(alarm.trigger.stamp / 3600) + "H";
                    }
                    else
                    {
                        trigger_line += "P" + std::to_string(alarm.trigger.stamp / (3600 * 24)) + "D";
                    }
                }
                result += trigger_line + "\r\n";
            }

            if (!alarm.description.empty())
            {
                result += "DESCRIPTION:" + serialize_to_ics_string_escape_description(alarm.description, false) + "\r\n";
            }

            if (!alarm.duration.empty())
            {
                result += "DURATION:" + alarm.duration + "\r\n";
            }

            if (alarm.repeat != -1)
            {
                result += "REPEAT:" + std::to_string(alarm.repeat) + "\r\n";
            }

            if (!alarm.attach.empty())
            {
                result += "ATTACH:" + serialize_to_ics_string_escape_text_value(alarm.attach) + "\r\n";
            }

            result += "END:VALARM\r\n";
        }

        result += "END:VEVENT\r\n";
    }

    result += "END:VCALENDAR\r\n";

    return result;
}

zcc_namespace_end;
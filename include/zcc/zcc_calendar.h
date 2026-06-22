/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2025-08-13
 * ================================
 */

#pragma once

#ifndef ZCC_LIB_INCLUDE_ICS__
#define ZCC_LIB_INCLUDE_ICS__

#include "./zcc_begin_end_text.h"

#ifdef __cplusplus
#pragma pack(push, 4)
zcc_namespace_begin;

struct ics_calendar_event_rrule_byweekday
{
    int day; // by week day, 0:SU/1:MO/2:TU/3:WE/4:TH/5:FR/6:SA
    int sn{0};
};

struct ics_calendar_event_rrule
{
    enum freq_type
    {
        none = 0,
        daily,
        weekly,
        monthly,
        yearly,
    };
    static ics_calendar_event_rrule parse(const std::string &rrule_rawata, int64_t dtstart, const std::string &tzid);
    static std::string build(const ics_calendar_event_rrule &rrule);
    static std::string get_freq_string(freq_type freq);
    inline bool is_valid() { return freq != freq_type::none; }
    inline std::string build() { return build(*this); }
    std::string get_digest();
    freq_type freq{none};
    int interval{1};
    int count{-1};
    int64_t until{-1};
    std::vector<int> bysetpos;
    std::vector<ics_calendar_event_rrule_byweekday> byweekday;
    std::vector<int> bymonthday; // by month day, 1-31
    std::vector<int> bymonth;    // by month, 1-12
    std::vector<int> byyearday;  // by year day, 1-366
    std::vector<int> byweekno;   // by week no, 1-53
};

struct ics_calendar_timezone_node
{
    std::string type; // standard, daylight
    std::string name;
    int tzoffsetto{-1};
    int tzoffsetfrom{-1};
    int64_t dtstart{-1};
    std::string rrule_rawdata;
    ics_calendar_event_rrule rrule;
};

struct ics_calendar_timezone
{
    void serialize_to_json(json &js);
    void unserialize_from_json(json &js);
    std::string tzid;
    std::vector<ics_calendar_timezone_node> nodes;
};

struct ics_calendar_event_alarm
{
    void serialize_to_json(json &js);
    void unserialize_from_json(json &js);
    std::string get_digest();
    struct
    {
        int64_t stamp{-1};
        bool isTimePoint{false};
        bool isRelated{true};
        bool isBeforeOrAfter{true};
        bool isBeginOrEnd{true};
    } trigger;
    std::string action; // audio, display, email, procedure
    std::string description;
    std::string duration;
    std::string attach;
    std::vector<std::string> attendees;
    int repeat{-1};
};

struct ics_calendar_event_attendee
{
    void serialize_to_json(json &js);
    void unserialize_from_json(json &js);
    std::string email;
    std::string partstat;
    std::string cn;
    std::string role;
    std::string rsvp;
    std::string cutype;
    std::string delegated_from;
    std::string delegated_to;
    std::string sent_by;
};

struct ics_calendar_event
{
    void serialize_to_json(json &js);
    void unserialize_from_json(json &js);
    std::string serialize_to_json_string();
    void unserialize_from_json_string(const std::string &json_string);
    std::string get_time_digest();
    std::string get_alarm_digest();
    std::string get_rrule_digest();
    std::string get_attendee_digest(const ics_calendar_event_attendee &attendee);
    std::string ics_calender_prodid;   // from ics_calendar
    std::string ics_calender_version;  // from ics_calendar
    std::string ics_calender_method;   // from ics_calendar, lowercase
    std::string ics_calender_calscale; // from ics_calendar, lowercase
    std::string uid;
    std::string organizer;
    std::string organizer_name;
    std::string summary;
    std::string location;
    std::string description_plain;
    std::string description_html;
    std::string transp;
    std::string status;
    std::string tzid;
    int64_t dtstamp{-1};
    int64_t dtstart{-1};
    int64_t dtend{-1};
    int64_t created{-1};
    int64_t last_modified{-1};
    int sequence{-1};
    int priority{-1};
    std::string CLASS;
    std::string categories;
    std::string rrule_rawdata;
    ics_calendar_event_rrule rrule;
    std::vector<ics_calendar_event_attendee> attendees;
    std::vector<ics_calendar_event_alarm> alarms;
    bool isMeeting{false};
    bool isMeetingOwner{false};
    bool isAllDay{false};
    bool isRecurrence{false};
};

struct ics_calendar_todo
{
    std::string uid;
};

struct ics_calendar_journal
{
    std::string uid;
};

class ZCC_LIB_API ics_calendar
{
public:
    ics_calendar();
    ~ics_calendar();
    inline void set_ignore_parse_timezone(bool ignore = true) { ignore_parse_timezone_ = ignore; }
    inline void set_ignore_parse_rrule(bool ignore = true) { ignore_parse_rrule_ = ignore; }
    inline void set_ignore_parse_alarm(bool ignore = true) { ignore_parse_alarm_ = ignore; }
    void parse(const char *text, int text_len);
    inline void parse(const std::string &text)
    {
        parse(text.c_str(), (int)text.size());
    }
    std::string debug_info();
    std::string serialize_to_ics_string();

public:
    std::string prodid_;
    std::string version_;
    std::string method_;   // lowercase
    std::string calscale_; // lowercase
    std::vector<ics_calendar_timezone> timezone_;
    std::vector<ics_calendar_event> events_;

protected:
    void parse_vtimezone(begin_end_text_node *node);
    void parse_timezone_node(begin_end_text_node *node, ics_calendar_timezone &timezone, const std::string &type);
    void parse_vevent(begin_end_text_node *node);
    void parse_vtodo(begin_end_text_node *node);
    void parse_vjournal(begin_end_text_node *node);
    void parse_valarm(begin_end_text_node *node, ics_calendar_event &event);
    void parse_valarm_trigger(begin_end_text_node *node, ics_calendar_event_alarm &alarm);
    void parse_check_is_meeting(ics_calendar_event &event);
    void parse_check_is_recurrence(ics_calendar_event &event);
    bool parse_is_all_day(begin_end_text_node *node);
    //
    std::string parse_simple_string(begin_end_text_node *node);
    int64_t parse_simple_time(begin_end_text_node *node);
    void parse_description(begin_end_text_node *node, ics_calendar_event &event);
    void parse_rrule(begin_end_text_node *node, ics_calendar_event &event);
    void parse_attendee(begin_end_text_node *node, ics_calendar_event &event);

private:
    bool ignore_parse_timezone_{false};
    bool ignore_parse_rrule_{false};
    bool ignore_parse_alarm_{false};
};

class ics_calendar_event_matcher
{
public:
    ics_calendar_event_matcher();
    ~ics_calendar_event_matcher();
    inline ics_calendar_event_rrule &get_rrule() { return rrule_; }
    bool match_second_scope(int64_t unix_start, int64_t unix_end);
    bool match_day(int day, const std::string &tzid = "" /*get_current_timezone()*/);

public:
    void load_from_event(const ics_calendar_event &event);

public:
    void set_dtstart_and_rrule(int64_t dtstart, const std::string &tzid, const std::string &rrule_rawdata);
    void set_dtend(int64_t dtend);

private:
    std::string event_tzid_;
    int64_t dtstart_{-1};
    int64_t dtend_{-1};
    ics_calendar_event_rrule rrule_;
};

zcc_namespace_end;
#pragma pack(pop)
#endif // __cplusplus

#endif // ZCC_LIB_INCLUDE_ICS__

/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2026-04-09
 * ================================
 */

#include "zcc/zcc_stdlib.h"
#include <ctime>

zcc_namespace_begin;

const char *get_lunar_month_name(int month)
{
    static const char *lunar_month_names[12] = {"一月", "二月", "三月", "四月", "五月", "六月", "七月", "八月", "九月", "十月", "冬月", "腊月"};
    if (month < 0)
    {
        month = 0;
    }
    else if (month > 11)
    {
        month = 11;
    }
    return lunar_month_names[month];
}

const char *get_lunar_day_name(int day)
{
    static const char *lunar_day_names[30] = {"初一", "初二", "初三", "初四", "初五", "初六", "初七", "初八", "初九", "初十", "十一", "十二", "十三", "十四", "十五", "十六", "十七", "十八", "十九", "二十", "廿一", "廿二", "廿三", "廿四", "廿五", "廿六", "廿七", "廿八", "廿九", "三十"};
    if (day < 0)
    {
        day = 0;
    }
    else if (day > 29)
    {
        day = 29;
    }
    return lunar_day_names[day];
}

// 农历1900-2100年的润大小信息表
static const std::vector<int> lunarInfo = {
    0x04bd8, 0x04ae0, 0x0a570, 0x054d5, 0x0d260, 0x0d950, 0x16554, 0x056a0, 0x09ad0, 0x055d2, // 1900-1909
    0x04ae0, 0x0a5b6, 0x0a4d0, 0x0d250, 0x1d255, 0x0b540, 0x0d6a0, 0x0ada2, 0x095b0, 0x14977, // 1910-1919
    0x04970, 0x0a4b0, 0x0b4b5, 0x06a50, 0x06d40, 0x1ab54, 0x02b60, 0x09570, 0x052f2, 0x04970, // 1920-1929
    0x06566, 0x0d4a0, 0x0ea50, 0x16a95, 0x05ad0, 0x02b60, 0x186e3, 0x092e0, 0x1c8d7, 0x0c950, // 1930-1939
    0x0d4a0, 0x1d8a6, 0x0b550, 0x056a0, 0x1a5b4, 0x025d0, 0x092d0, 0x0d2b2, 0x0a950, 0x0b557, // 1940-1949
    0x06ca0, 0x0b550, 0x15355, 0x04da0, 0x0a5b0, 0x14573, 0x052b0, 0x0a9a8, 0x0e950, 0x06aa0, // 1950-1959
    0x0aea6, 0x0ab50, 0x04b60, 0x0aae4, 0x0a570, 0x05260, 0x0f263, 0x0d950, 0x05b57, 0x056a0, // 1960-1969
    0x096d0, 0x04dd5, 0x04ad0, 0x0a4d0, 0x0d4d4, 0x0d250, 0x0d558, 0x0b540, 0x0b6a0, 0x195a6, // 1970-1979
    0x095b0, 0x049b0, 0x0a974, 0x0a4b0, 0x0b27a, 0x06a50, 0x06d40, 0x0af46, 0x0ab60, 0x09570, // 1980-1989
    0x04af5, 0x04970, 0x064b0, 0x074a3, 0x0ea50, 0x06b58, 0x05ac0, 0x0ab60, 0x096d5, 0x092e0, // 1990-1999
    0x0c960, 0x0d954, 0x0d4a0, 0x0da50, 0x07552, 0x056a0, 0x0abb7, 0x025d0, 0x092d0, 0x0cab5, // 2000-2009
    0x0a950, 0x0b4a0, 0x0baa4, 0x0ad50, 0x055d9, 0x04ba0, 0x0a5b0, 0x15176, 0x052b0, 0x0a930, // 2010-2019
    0x07954, 0x06aa0, 0x0ad50, 0x05b52, 0x04b60, 0x0a6e6, 0x0a4e0, 0x0d260, 0x0ea65, 0x0d530, // 2020-2029
    0x05aa0, 0x076a3, 0x096d0, 0x04afb, 0x04ad0, 0x0a4d0, 0x1d0b6, 0x0d250, 0x0d520, 0x0dd45, // 2030-2039
    0x0b5a0, 0x056d0, 0x055b2, 0x049b0, 0x0a577, 0x0a4b0, 0x0aa50, 0x1b255, 0x06d20, 0x0ada0, // 2040-2049
    0x14b63, 0x09370, 0x049f8, 0x04970, 0x064b0, 0x168a6, 0x0ea50, 0x06b20, 0x1a6c4, 0x0aae0, // 2050-2059
    0x092e0, 0x0d2e3, 0x0c960, 0x0d557, 0x0d4a0, 0x0da50, 0x05d55, 0x056a0, 0x0a6d0, 0x055d4, // 2060-2069
    0x052d0, 0x0a9b8, 0x0a950, 0x0b4a0, 0x0b6a6, 0x0ad50, 0x055a0, 0x0aba4, 0x0a5b0, 0x052b0, // 2070-2079
    0x0b273, 0x06930, 0x07337, 0x06aa0, 0x0ad50, 0x14b55, 0x04b60, 0x0a570, 0x054e4, 0x0d160, // 2080-2089
    0x0e968, 0x0d520, 0x0daa0, 0x16aa6, 0x056d0, 0x04ae0, 0x0a9d4, 0x0a2d0, 0x0d150, 0x0f252, // 2090-2099
    0x0d520                                                                                   // 2100
};

// 天干地支
static const std::vector<std::string> lunar_date_Gan = {"甲", "乙", "丙", "丁", "戊", "己", "庚", "辛", "壬", "癸"};
static const std::vector<std::string> lunar_date_Zhi = {"子", "丑", "寅", "卯", "辰", "巳", "午", "未", "申", "酉", "戌", "亥"};
static const std::vector<std::string> lunar_date_ChineseZodiac = {"鼠", "牛", "虎", "兔", "龙", "蛇", "马", "羊", "猴", "鸡", "狗", "猪"};

// 农历节日
static const std::map<std::string, std::string> lunar_date_Festival = {
    {"12-30", "除夕"},
    {"1-1", "春节"},
    {"1-15", "元宵"},
    // {"2-2", "龙抬头"},
    {"5-5", "端午"},
    {"7-7", "七夕"},
    // {"7-15", "中元节"},
    {"8-15", "中秋"},
    {"9-9", "重阳"},
    // {"10-1", "寒衣节"},
    // {"10-15", "下元节"},
    {"12-8", "腊八"},
    // {"12-23", "北方小年"},
    // {"12-24", "南方小年"}

};

// 辅助函数实现
static int get_lunar_date_leapMonth(int y)
{
    return lunarInfo[y - 1900] & 0xf;
}

static int get_lunar_date_leapDays(int y)
{
    if (get_lunar_date_leapMonth(y))
    {
        return (lunarInfo[y - 1900] & 0x10000) ? 30 : 29;
    }
    return 0;
}

static int get_lunar_date_lYearDays(int y)
{
    int sum = 348;
    for (int i = 0x8000; i > 0x8; i >>= 1)
    {
        sum += (lunarInfo[y - 1900] & i) ? 1 : 0;
    }
    return sum + get_lunar_date_leapDays(y);
}

static int get_lunar_date_monthDays(int y, int m)
{
    if (m > 12 || m < 1)
    {
        return -1;
    }
    return (lunarInfo[y - 1900] & (0x10000 >> m)) ? 30 : 29;
}

static std::string get_lunar_date_anAnimal(int y)
{
    return lunar_date_ChineseZodiac[(y - 4) % 12];
}

lunar_date get_lunar_date(int yyyymmdd)
{
    lunar_date result;
    result.is_error = true;

    if (yyyymmdd < 19000101 || yyyymmdd > 20991231)
    {
        return result;
    }

    int solar_year = yyyymmdd / 10000;
    int solar_month = (yyyymmdd % 10000) / 100;
    int solar_day = yyyymmdd % 100;

    // 检查日期有效性
    if (solar_month < 1 || solar_month > 12 || solar_day < 1 || solar_day > 31)
    {
        return result;
    }

    // 计算与1900年1月31日的天数差
    struct std::tm timeinfo = {};
    timeinfo.tm_year = solar_year - 1900;
    timeinfo.tm_mon = solar_month - 1;
    timeinfo.tm_mday = solar_day;

    struct std::tm base_timeinfo = {};
    base_timeinfo.tm_year = 0; // 1900
    base_timeinfo.tm_mon = 0;  // January
    base_timeinfo.tm_mday = 31;

    std::time_t solar_time = std::mktime(&timeinfo);
    std::time_t base_time = std::mktime(&base_timeinfo);

    int64_t offset = (solar_time - base_time) / (60 * 60 * 24);

    // 查找对应的农历年
    int lunar_year = 1900;
    int temp = 0;
    for (; lunar_year < 2101 && offset > 0; lunar_year++)
    {
        temp = get_lunar_date_lYearDays(lunar_year);
        offset -= temp;
    }

    if (offset < 0)
    {
        offset += temp;
        lunar_year--;
    }

    // 查找对应的农历月
    int leap = get_lunar_date_leapMonth(lunar_year);
    bool isLeap = false;
    int lunar_month = 1;

    for (; lunar_month < 13 && offset > 0; lunar_month++)
    {
        // 处理闰月
        if (leap > 0 && lunar_month == leap + 1 && !isLeap)
        {
            lunar_month--;
            isLeap = true;
            temp = get_lunar_date_leapDays(lunar_year);
        }
        else
        {
            temp = get_lunar_date_monthDays(lunar_year, lunar_month);
        }

        // 解除闰月
        if (isLeap && lunar_month == leap + 1)
        {
            isLeap = false;
        }

        offset -= temp;
    }

    // 闰月导致数组下标重叠取反
    if (offset == 0 && leap > 0 && lunar_month == leap + 1)
    {
        if (isLeap)
        {
            isLeap = false;
        }
        else
        {
            isLeap = true;
            lunar_month--;
        }
    }

    if (offset < 0)
    {
        offset += temp;
        lunar_month--;
    }

    int lunar_day = offset + 1;

    // 构建结果
    result.is_error = false;
    result.month = lunar_month;
    result.day = lunar_day;

    // 农历月份名称
    result.is_leap_month = isLeap;
    result.month_name = zcc::get_lunar_month_name(lunar_month - 1);

    // 农历日名称
    result.day_name = zcc::get_lunar_day_name(lunar_day - 1);

    // 生肖
    result.zodiac_name = get_lunar_date_anAnimal(lunar_year);

    // 检查农历节日
    std::string lunarFestivalKey = std::to_string(lunar_month) + "-" + std::to_string(lunar_day);
    auto it = lunar_date_Festival.find(lunarFestivalKey);
    if (it != lunar_date_Festival.end())
    {
        result.festival_name = it->second;
    }

    // 年的天干地支
    int ganKey = (lunar_year - 3) % 10;
    int zhiKey = (lunar_year - 3) % 12;
    if (ganKey == 0)
    {
        ganKey = 10;
    }
    if (zhiKey == 0)
    {
        zhiKey = 12;
    }
    result.year_tiangan = lunar_date_Gan[ganKey - 1];
    result.year_dizhi = lunar_date_Zhi[zhiKey - 1];

    //

    return result;
}

zcc_namespace_end;

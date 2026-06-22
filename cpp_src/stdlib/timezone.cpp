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
#include <poll.h>
#include <limits.h>
#include <unistd.h>
#endif // _WIN64
#ifdef __APPLE__
#include <sys/param.h>
#include <sys/sysctl.h>
#endif

zcc_namespace_begin;

/**
 * 获取当前程序的时区（IANA格式）
 * 优先级：1. TZ环境变量  2. 系统配置的时区
 *
 * @return 时区字符串，例如 "Asia/Shanghai" 或 "America/New_York"
 *         如果获取失败，返回空字符串
 */
std::string get_current_timezone(const std::string &default_tzid)
{
    // ========== 优先级1：检查 TZ 环境变量 ==========
    const char *tz_env = std::getenv("TZ");
    if (tz_env != nullptr && tz_env[0] != '\0')
    {
        std::string result(tz_env);
        // 处理格式 ":Asia/Shanghai" 或 ":/usr/share/zoneinfo/Asia/Shanghai"
        if (result.size() > 1 && result[0] == ':')
        {
            result = result.substr(1);
            // 尝试提取 "zoneinfo/" 后面的部分
            size_t zoneinfo_pos = result.find("zoneinfo/");
            if (zoneinfo_pos != std::string::npos)
            {
                result = result.substr(zoneinfo_pos + 9);
            }
        }
        return result;
    }

    // ========== 优先级2：获取系统配置的时区 ==========
#if defined(_WIN64)
    // Windows：通过注册表获取时区，然后映射到 IANA 名称
    // 这是一个简化的实现，完整映射表很长，这里只返回 Windows 时区名
    TIME_ZONE_INFORMATION tzInfo;
    DWORD result = GetTimeZoneInformation(&tzInfo);
    if (result != TIME_ZONE_ID_INVALID)
    {
        // 将宽字符转换为 UTF-8 字符串
        std::wstring wname(tzInfo.StandardName);
        std::string name(wname.begin(), wname.end());
        return name; // 返回 "China Standard Time" 等 Windows 格式
    }
#else
    // 方法1：读取 /etc/localtime 符号链接
    char link_path[PATH_MAX] = {0};
    ssize_t len = ::readlink("/etc/localtime", link_path, sizeof(link_path) - 1);

    if (len != -1)
    {
        link_path[len] = '\0';
        std::string tz_path(link_path);

        // 查找 "zoneinfo/" 后面的部分作为时区名
        size_t pos = tz_path.find("zoneinfo/");
        if (pos != std::string::npos)
        {
            return tz_path.substr(pos + 9);
        }

        // 备选：查找 "posix/" 或 "right/" 等前缀
        pos = tz_path.rfind('/');
        if (pos != std::string::npos && pos + 1 < tz_path.length())
        {
            // 返回最后一部分作为时区名（不完全准确，但作为备选）
            return tz_path.substr(pos + 1);
        }
    }

    // 方法2：尝试读取 /etc/timezone (Debian/Ubuntu 等)
    FILE *fp = fopen("/etc/timezone", "r");
    if (fp != nullptr)
    {
        char buffer[256] = {0};
        if (fgets(buffer, sizeof(buffer), fp) != nullptr)
        {
            // 去除末尾的换行符
            char *newline = strchr(buffer, '\n');
            if (newline)
            {
                *newline = '\0';
            }
            fclose(fp);
            if (strlen(buffer) > 0)
            {
                return std::string(buffer);
            }
        }
        fclose(fp);
    }

    // 方法3：尝试读取 /etc/sysconfig/clock (RHEL/CentOS 等)
    fp = fopen("/etc/sysconfig/clock", "r");
    if (fp != nullptr)
    {
        char line[256];
        while (fgets(line, sizeof(line), fp) != nullptr)
        {
            // 查找 "ZONE=" 或 "TIMEZONE=" 行
            if (strncmp(line, "ZONE=", 5) == 0 || strncmp(line, "TIMEZONE=", 9) == 0)
            {
                char *value = strchr(line, '=');
                if (value != nullptr)
                {
                    value++; // 跳过 '='
                    // 去除引号
                    if (*value == '"' || *value == '\'')
                    {
                        value++;
                        char *end = strrchr(value, '"');
                        if (end == nullptr)
                            end = strrchr(value, '\'');
                        if (end != nullptr)
                            *end = '\0';
                    }
                    // 去除换行符
                    char *newline = strchr(value, '\n');
                    if (newline)
                        *newline = '\0';
                    fclose(fp);
                    if (strlen(value) > 0)
                    {
                        return std::string(value);
                    }
                }
            }
        }
        fclose(fp);
    }
#endif

#ifdef __APPLE__
    // ---------- 方法4：macOS 专用 - 通过 systemsetup 命令获取 ----------
    FILE *fp = popen("/usr/sbin/systemsetup -gettimezone 2>/dev/null", "r");
    if (fp != nullptr)
    {
        char buffer[256] = {0};
        if (fgets(buffer, sizeof(buffer), fp) != nullptr)
        {
            // 输出格式："Time Zone: Asia/Shanghai"
            std::string line(buffer);
            size_t colon_pos = line.find(':');
            if (colon_pos != std::string::npos)
            {
                std::string tz_name = line.substr(colon_pos + 1);
                // 去除前后空白和换行符
                size_t start = tz_name.find_first_not_of(" \t\n\r");
                if (start != std::string::npos)
                {
                    tz_name = tz_name.substr(start);
                }
                size_t end = tz_name.find_last_not_of(" \t\n\r");
                if (end != std::string::npos)
                {
                    tz_name = tz_name.substr(0, end + 1);
                }
                pclose(fp);
                if (!tz_name.empty())
                {
                    return tz_name;
                }
            }
        }
        pclose(fp);
    }

    // ---------- 方法5：macOS 专用 - 通过 sysctl 读取内核时区 ----------
    char tz_name[256] = {0};
    size_t size = sizeof(tz_name);
    if (sysctlbyname("kern.timezone", tz_name, &size, nullptr, 0) == 0)
    {
        if (strlen(tz_name) > 0)
        {
            return std::string(tz_name);
        }
    }
#endif

    // ========== 无法获取 ==========
    return default_tzid;
}

// 独立函数：解析 tzfile 并返回完整数据
timezone_filedata parse_tzfile(const std::string &path)
{
    timezone_filedata data;
    data.error = true;
    std::ifstream file(path, std::ios::binary);
    if (!file)
    {
        return data;
    }

    char magic[5] = {0};
    file.read(magic, 4);
    if (std::string(magic) != "TZif")
    {
        data.file_magic_not_match = true;
        return data;
    }

    char version;
    file.read(&version, 1);
    file.seekg(15, std::ios::cur); // reserved

    auto read_int32 = [&file]()
    {
        int32_t val;
        file.read(reinterpret_cast<char *>(&val), 4);
        val = ((val & 0xFF) << 24) | ((val & 0xFF00) << 8) | ((val & 0xFF0000) >> 8) | ((val & 0xFF000000) >> 24);
        return val;
    };

    int32_t ttisgmtcnt = read_int32();
    int32_t ttisstdcnt = read_int32();
    int32_t leapcnt = read_int32();
    int32_t timecnt = read_int32();
    int32_t typecnt = read_int32();
    int32_t charcnt = read_int32();

    data.transitions.resize(timecnt);
    for (int i = 0; i < timecnt; ++i)
    {
        data.transitions[i] = read_int32();
    }

    data.types.resize(timecnt);
    file.read(reinterpret_cast<char *>(data.types.data()), timecnt);

    data.ttinfos.resize(typecnt);
    for (int i = 0; i < typecnt; ++i)
    {
        data.ttinfos[i].gmtoff = read_int32();
        file.read(reinterpret_cast<char *>(&data.ttinfos[i].isdst), 1);
        file.read(reinterpret_cast<char *>(&data.ttinfos[i].abbrind), 1);
    }
    if (data.ttinfos.empty())
    {
        return data;
    }

    data.error = false;
    return data;
}

const timezone_filedata &get_timezone_filedata(const std::string &timezone)
{
    // static std::map<std::string, timezone_filedata> timezone_data_map;
    // static std::map<std::string, std::string> timezone_alias_map;
    static timezone_filedata nonexistent_data = {{}, {}, {}, true, true, false};
#include "timezone_data.hpp"
    auto tzname = timezone;
    zcc::tolower(tzname);
    auto it_map = timezone_data_map.find(tzname);
    if (it_map != timezone_data_map.end())
    {
        return it_map->second;
    }
    auto it_alias = timezone_alias_map.find(tzname);
    if (it_alias != timezone_alias_map.end())
    {
        it_map = timezone_data_map.find(it_alias->second);
        if (it_map != timezone_data_map.end())
        {
            return it_map->second;
        }
    }
    if (tzname.find("中国") != std::string::npos)
    {
        tzname = "asia/shanghai";
    }
    else if (tzname.find("china") != std::string::npos)
    {
        tzname = "asia/shanghai";
    }
    else if (tzname.find("北京") != std::string::npos)
    {
        tzname = "asia/shanghai";
    }
    else if (tzname.find("beijing") != std::string::npos)
    {
        tzname = "asia/shanghai";
    }
    else
    {
        return nonexistent_data;
    }
    it_map = timezone_data_map.find(tzname);
    if (it_map != timezone_data_map.end())
    {
        return it_map->second;
    }
    return nonexistent_data;
}

const timezone_info &get_timezone_info(const timezone_filedata &data, int64_t unix_second)
{
    static timezone_info nonexistent_info = {};
    if (data.error || data.ttinfos.empty())
    {
        return nonexistent_info;
    }
    if (data.transitions.empty())
    {
        return data.ttinfos[0];
    }
    else
    {
        int type_index = 0;
        for (size_t i = 0; i < data.transitions.size(); ++i)
        {
            if (data.transitions[i] <= unix_second)
            {
                type_index = data.types[i];
            }
            else
            {
                break;
            }
        }
        return data.ttinfos[type_index];
    }
}

const timezone_info &get_timezone_info(const std::string &timezone, int64_t unix_second)
{
    return get_timezone_info(get_timezone_filedata(timezone), unix_second);
}

extern thread_local struct tm gmtime_static_buf;
struct tm *gmtime_with_timezone(int64_t unix_second, const std::string &tzid)
{
    if (tzid.empty())
    {
        return gmtime(unix_second);
    }
    const timezone_filedata &data = get_timezone_filedata(tzid);
    if (data.error)
    {
        return nullptr;
    }
    timezone_info tzinfo = get_timezone_info(data, unix_second);
    int64_t local_time = unix_second + tzinfo.gmtoff;
    time_t lt = local_time;
#ifdef _WIN64
    ::gmtime_s(tm, &lt);
#else
    if (!::gmtime_r(&lt, &gmtime_static_buf))
    {
        return nullptr;
    }
#endif
    gmtime_static_buf.tm_isdst = tzinfo.isdst;
#ifdef _WIN64
#else
    gmtime_static_buf.tm_gmtoff = tzinfo.gmtoff;
#endif
    return &gmtime_static_buf;
}

bool gmtime_with_timezone(int64_t unix_second, const std::string &tzid, struct tm *tm)
{
    auto *p = gmtime_with_timezone(unix_second, tzid);
    if (!p)
    {
        return false;
    }
    std::memcpy(tm, p, sizeof(struct tm));
    return true;
}

int timezone_0800_offset(const std::string &timezone_0800)
{
    int r = 0;
    if (timezone_0800.empty())
    {
        return r;
    }
    const char *ps = timezone_0800.c_str();
    bool tf = true;
    if (*ps == '-')
    {
        tf = false;
        ps++;
    }
    else if (*ps == '+')
    {
        ps++;
    }
    int len = (int)strlen(ps);
    if (len != 4 && len != 6)
    {
        return 0;
    }
    r = ((ps[0] - '0') * 10 + (ps[1] - '0')) * 3600 + (ps[2] - '0') * 10 + (ps[3] - '0') * 60;
    if (!tf)
    {
        r = -r;
    }
    return r;
}

zcc_namespace_end;

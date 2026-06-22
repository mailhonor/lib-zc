/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2026-04-07
 * ================================
 */

#include "zcc/zcc_stdlib.h"
#include <set>

static std::string zoneinfo_path = "/usr/share/zoneinfo/";

static std::vector<std::string> find_timezone_file(const std::string &file_type)
{
    std::vector<std::string> r;
#ifdef _WIN64
#else
    char buf[4096 + 1];
    std::snprintf(buf, sizeof(buf), "find %s -type %s", zoneinfo_path.c_str(), file_type.c_str());
    // 执行查找命令
    FILE *fp = popen(buf, "r");
    if (!fp)
    {
        zcc_fatal("popen error: %s", buf);
    }
    while (fgets(buf, 4096, fp))
    {
        char *p = strchr(buf, '\n');
        if (p)
        {
            *p = 0;
        }
        p = strchr(buf, '\r');
        if (p)
        {
            *p = 0;
        }
        r.push_back(buf);
    }
    fclose(fp);
#endif

    return r;
}

static std::string preapre_timezone_true_data()
{
    std::string out;

    std::map<std::string, zcc::timezone_filedata> timezone_data;

    auto file_paths = find_timezone_file("f");

    for (const auto &path : file_paths)
    {
        std::string tz_name = path.substr(zoneinfo_path.length()); // remove /usr/share/zoneinfo/
        if (tz_name.empty())
        {
            continue;
        }
        if (zcc::starts_with(tz_name, "posix/"))
        {
            continue;
        }
        if (zcc::starts_with(tz_name, "right/"))
        {
            continue;
        }

        zcc::timezone_filedata data = zcc::parse_tzfile(path);
        if (data.error)
        {
            if (data.file_magic_not_match)
            {
                continue;
            }
            zcc_fatal("parse tzfile failed: %s", path.c_str());
        }
        zcc::tolower(tz_name);
        timezone_data[tz_name] = data;
    }

    for (const auto &path : file_paths)
    {
        std::string tz_name = path.substr(zoneinfo_path.length()); // remove /usr/share/zoneinfo/
        if (tz_name.empty())
        {
            continue;
        }
        if (!zcc::starts_with(tz_name, "posix/") && !zcc::starts_with(tz_name, "right/"))
        {
            continue;
        }

        zcc::timezone_filedata data = zcc::parse_tzfile(path);
        if (data.error)
        {
            if (data.file_magic_not_match)
            {
                continue;
            }
            zcc_fatal("parse tzfile failed: %s", path.c_str());
        }
        auto pos = tz_name.find_first_of('/');
        std::string name = tz_name.substr(pos + 1);
        zcc::tolower(name);
        if (timezone_data.find(name) != timezone_data.end())
        {
            continue;
        }
        timezone_data[name] = data;
    }

    out += "static std::map<std::string, timezone_filedata> timezone_data_map = {\n";

    for (const auto &pair : timezone_data)
    {
        const std::string &tz_name = pair.first;
        const zcc::timezone_filedata &data = pair.second;

        out += "{\"";
        out += tz_name;
        out += "\",{";
        out += "{";
        for (size_t i = 0; i < data.transitions.size(); ++i)
        {
            out += "INT64_C(" + std::to_string(data.transitions[i]) + ")";
            if (i < data.transitions.size() - 1)
                out += ",";
        }
        out += "},{";
        for (size_t i = 0; i < data.types.size(); ++i)
        {
            out += std::to_string((int)data.types[i]);
            if (i < data.types.size() - 1)
                out += ",";
        }
        out += "},{";
        for (size_t i = 0; i < data.ttinfos.size(); ++i)
        {
            const auto &info = data.ttinfos[i];
            out += "{";
            out += std::to_string(info.gmtoff);
            out += ",";
            out += std::to_string((int)info.isdst);
            out += ",";
            out += std::to_string((int)info.abbrind);
            out += "}";
            if (i < data.ttinfos.size() - 1)
                out += ",";
        }
        out += "},false,false,false}},\n";
    }
    out += "};\n";
    return out;
}

static std::string preapre_timezone_alias_map()
{
    std::string out;
    std::map<std::string, std::string> timezone_alias_map;
    char buf[256 + 1];

    auto file_paths = find_timezone_file("l");

    for (const auto &path : file_paths)
    {
        if (path.find("/posix/") != std::string::npos)
        {
            continue;
        }
        if (path.find("/right/") != std::string::npos)
        {
            continue;
        }
        std::string real_path = zcc::realpath(path);
        if (real_path.empty())
        {
            continue;
        }
        if (real_path == path)
        {
            continue;
        }
        if (!zcc::starts_with(real_path, zoneinfo_path))
        {
            continue;
        }
        if (real_path.find("/posix/") != std::string::npos)
        {
            continue;
        }
        if (real_path.find("/right/") != std::string::npos)
        {
            continue;
        }
        std::string alias_tz_name = path.substr(zoneinfo_path.length());
        std::string real_tz_name = real_path.substr(zoneinfo_path.length());
        zcc::tolower(alias_tz_name);
        zcc::tolower(real_tz_name);
        timezone_alias_map[alias_tz_name] = real_tz_name;
    }
    out += "static std::map<std::string, std::string> timezone_alias_map = {\n";
    for (const auto &pair : timezone_alias_map)
    {
        const std::string &alias_tz_name = pair.first;
        const std::string &real_tz_name = pair.second;
        out += "{\"";
        out += alias_tz_name;
        out += "\",\"";
        out += real_tz_name;
        out += "\"},\n";
    }
    out += "};\n";
    return out;
}

int main(int argc, char **argv)
{
    std::string out;
    out += preapre_timezone_true_data();
    out += preapre_timezone_alias_map();
    zcc::file_put_contents("timezone_data.hpp", out);
    //
    out = zcc::file_get_contents_sample("timezone_data.hpp");
    auto vs = zcc::split(out, '\n');
    if (vs.size() < 600)
    {
        zcc_fatal("timezone_data.hpp line count is %d, maybe failed", (int)vs.size());
    }
    zcc_info("timezone_data.hpp created success");
    return 0;
}
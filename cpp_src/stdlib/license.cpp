/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2016-09-09
 * ================================
 */

#include "zcc/zcc_stdlib.h"

zcc_namespace_begin;

std::string license_build(const char *salt, const char *mac)
{
    std::string tmpstr;
    tmpstr.append(salt).append(",");
    for (auto *p = mac; *p; p++)
    {
        tmpstr.push_back(tolower(*p));
    }

    int64_t crc = crc64(tmpstr);
    char new_license[32 + 1];
    std::sprintf(new_license, "%016zX", crc);
    return new_license;
}

int license_check(const char *salt, const char *license)
{
    std::vector<std::string> mac_vector;
    const char *ps = std::strchr(license, ',');
    if (ps)
    {
        std::string key(license, ps - license);
        if (license_build(salt, key.c_str()) == ps + 1)
        {
            return 1;
        }
        return 0;
    }

    int r = get_mac_address(mac_vector);
    if (r < 1)
    {
        return r;
    }
    for (auto it = mac_vector.begin(); it != mac_vector.end(); it++)
    {
        const char *mac = it->c_str();
        if (license_build(salt, mac) == license)
        {
            return 1;
        }
    }
    return 0;
}

zcc_namespace_end;

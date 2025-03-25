/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2016-09-09
 * ================================
 */

#include "zcc/zcc_stdlib.h"

zcc_namespace_begin;

/**
 * @brief 根据给定的盐值和MAC地址生成许可证字符串
 * 
 * @param salt 用于生成许可证的盐值字符串
 * @param mac 设备的MAC地址字符串
 * @return std::string 生成的许可证字符串
 */
std::string license_build(const char *salt, const char *mac)
{
    // 用于存储拼接后的临时字符串
    std::string tmpstr;
    // 将盐值和逗号拼接到临时字符串中
    tmpstr.append(salt).append(",");
    // 遍历MAC地址字符串，将每个字符转换为小写并添加到临时字符串中
    for (auto *p = mac; *p; p++)
    {
        tmpstr.push_back(tolower(*p));
    }

    // 计算临时字符串的64位CRC校验值
    int64_t crc = crc64(tmpstr);
    // 用于存储格式化后的许可证字符串
    char new_license[32 + 1];
    // 将CRC校验值格式化为16位十六进制字符串
    std::sprintf(new_license, "%016zX", crc);
    return new_license;
}

/**
 * @brief 检查给定的许可证是否有效
 * 
 * @param salt 用于验证许可证的盐值字符串
 * @param license 待验证的许可证字符串
 * @return int 验证结果，1表示有效，0表示无效，小于0表示获取MAC地址失败
 */
int license_check(const char *salt, const char *license)
{
    // 用于存储设备的MAC地址列表
    std::vector<std::string> mac_vector;
    // 查找许可证字符串中逗号的位置
    const char *ps = std::strchr(license, ',');
    if (ps)
    {
        // 提取逗号之前的部分作为密钥
        std::string key(license, ps - license);
        // 生成许可证并与逗号之后的部分进行比较
        if (license_build(salt, key.c_str()) == ps + 1)
        {
            return 1; // 许可证有效
        }
        return 0; // 许可证无效
    }

    // 获取设备的MAC地址列表
    int r = get_mac_address(mac_vector);
    if (r < 1)
    {
        return r; // 获取MAC地址失败，返回错误码
    }
    // 遍历MAC地址列表，逐一验证许可证
    for (auto it = mac_vector.begin(); it != mac_vector.end(); it++)
    {
        const char *mac = it->c_str();
        if (license_build(salt, mac) == license)
        {
            return 1; // 许可证有效
        }
    }
    return 0; // 许可证无效
}

zcc_namespace_end;

/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2017-09-30
 * ================================
 */

#include "zcc/zcc_stdlib.h"
#ifdef _WIN64
#include <winsock2.h>
#include <iphlpapi.h>
#endif // _WIN64
#ifdef __linux__
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#endif // __linux__

zcc_namespace_begin;

#ifdef _WIN64
/**
 * @brief 获取Windows 64位系统下的MAC地址列表
 *
 * 该函数通过调用Windows API获取网络适配器信息，并从中提取有效的MAC地址，
 * 过滤掉重复的MAC地址后存储到传入的向量中。
 *
 * @param mac_list 用于存储获取到的MAC地址的向量
 * @return int 获取到的有效MAC地址的数量，若出现错误则可能返回 -1
 */
int get_mac_address(std::vector<std::string> &mac_list)
{
    // 清空存储MAC地址的向量
    mac_list.clear();
    // 用于存储已出现的MAC地址，避免重复添加
    std::map<std::string, bool> mac_dict;
    // 记录获取到的有效MAC地址的数量
    int ret_num = 0;
    // 分配内存以存储适配器信息
    PIP_ADAPTER_INFO pIpAdapterInfo = (PIP_ADAPTER_INFO) new char[sizeof(IP_ADAPTER_INFO) + 1];
    // 存储适配器信息缓冲区的大小
    ULONG stSize = sizeof(IP_ADAPTER_INFO);
    // 调用GetAdaptersInfo函数获取适配器信息
    int nRel = ::GetAdaptersInfo(pIpAdapterInfo, &stSize);
    // 如果缓冲区大小不足
    if (ERROR_BUFFER_OVERFLOW == nRel)
    {
        // 释放之前分配的内存
        delete[] (char *)pIpAdapterInfo;
        // 重新分配足够大小的内存
        pIpAdapterInfo = (PIP_ADAPTER_INFO) new char[stSize + 1];
        // 再次调用GetAdaptersInfo函数
        nRel = GetAdaptersInfo(pIpAdapterInfo, &stSize);
    }
    // 如果成功获取到适配器信息
    if (ERROR_SUCCESS == nRel)
    {
        // 指向下一个适配器信息的指针
        PIP_ADAPTER_INFO next;
        // 遍历所有适配器信息
        for (; pIpAdapterInfo; pIpAdapterInfo = next)
        {
            // 获取下一个适配器信息的指针
            next = pIpAdapterInfo->Next;
            // 标记是否忽略当前适配器
            int ignore = 1;
            // 根据适配器类型判断是否忽略
            switch (pIpAdapterInfo->Type)
            {
            case MIB_IF_TYPE_ETHERNET:
            case MIB_IF_TYPE_FDDI:
            case MIB_IF_TYPE_PPP:
            case MIB_IF_TYPE_SLIP:
                // 这些类型的适配器不忽略
                ignore = 0;
                break;
            default:
                break;
            }
            // 如果需要忽略当前适配器，则跳过
            if (ignore)
            {
                continue;
            }
            // 遍历适配器的地址信息
            for (DWORD i = 0; i < pIpAdapterInfo->AddressLength; i++)
            {
                // 只处理长度为6的MAC地址
                if (pIpAdapterInfo->AddressLength != 6)
                {
                    continue;
                }
                // 获取适配器的MAC地址
                unsigned char *ar = (unsigned char *)(pIpAdapterInfo->Address);
                // 用于存储格式化后的MAC地址
                char mbuf[32];
                // 将MAC地址格式化为字符串
                std::sprintf(mbuf, "%02X:%02X:%02X:%02X:%02X:%02X", ar[0], ar[1], ar[2], ar[3], ar[4], ar[5]);
                // 将格式化后的MAC地址转换为字符串
                std::string m = mbuf;
                // 检查该MAC地址是否已经存在
                if (mac_dict.find(m) != mac_dict.end())
                {
                    continue;
                }
                // 标记该MAC地址已存在
                mac_dict[m] = true;
                // 将MAC地址添加到向量中
                mac_list.push_back(m);
                // 有效MAC地址数量加1
                ret_num++;
            }
        }
    }
    // 释放适配器信息的内存
    if (pIpAdapterInfo)
    {
        delete[] (char *)pIpAdapterInfo;
    }

    return ret_num;
}

#endif // _WIN64

#ifdef __linux__
/**
 * @brief 获取Linux系统下的MAC地址列表
 *
 * 该函数通过创建套接字并调用ioctl系统调用获取网络接口信息，
 * 从中提取MAC地址并存储到传入的向量中。
 *
 * @param mac_list 用于存储获取到的MAC地址的向量
 * @return int 获取到的有效MAC地址的数量，若出现错误则可能返回 -1
 */
int get_mac_address(std::vector<std::string> &mac_list)
{
    // 套接字文件描述符
    int sock_fd;
    // 指向网络接口请求结构体的指针
    struct ifreq *buf;
    // 网络接口配置结构体
    struct ifconf ifc;
    // 记录获取到的有效MAC地址的数量
    int ret_num = 0, interface_num;

    // 创建一个UDP套接字
    sock_fd = ::socket(AF_INET, SOCK_DGRAM, 0);
    // 如果套接字创建失败
    if (sock_fd < 0)
    {
        return -1;
    }
    // 初始化网络接口配置结构体的缓冲区大小
    ifc.ifc_len = sizeof(struct ifreq) * 128;
    // 分配内存以存储网络接口请求结构体
    buf = (struct ifreq *)new char[sizeof(struct ifreq) * 128];
    // 将分配的内存地址赋值给网络接口配置结构体
    ifc.ifc_req = buf;
    // 调用ioctl函数获取网络接口配置信息
    if (ioctl(sock_fd, SIOCGIFCONF, (char *)&ifc) < 0)
    {
        // 释放分配的内存
        delete[] (char *)buf;
        //
        close(sock_fd);
        return -1;
    }
    // 计算网络接口的数量
    interface_num = ifc.ifc_len / sizeof(struct ifreq);
    // 遍历所有网络接口
    for (int ii = 0; ii < interface_num; ii++)
    {
        // 调用ioctl函数获取网络接口的硬件地址
        if (ioctl(sock_fd, SIOCGIFHWADDR, buf + ii) < 0)
        {
            continue;
        }
        // 获取网络接口的MAC地址
        unsigned char *ar = (unsigned char *)(buf[ii].ifr_hwaddr.sa_data);
        // 用于存储格式化后的MAC地址
        char mbuf[32];
        // 将MAC地址格式化为字符串
        std::sprintf(mbuf, "%02X:%02X:%02X:%02X:%02X:%02X", ar[0], ar[1], ar[2], ar[3], ar[4], ar[5]);
        // 将格式化后的MAC地址添加到向量中
        mac_list.push_back(mbuf);
        // 有效MAC地址数量加1
        ret_num++;
    }

    // 关闭套接字
    ::close(sock_fd);
    // 释放分配的内存
    delete[] (char *)buf;

    return ret_num;
}
#endif // __linux__

#ifndef _WIN64
#ifndef __linux__
/**
 * @brief 在非Windows和非Linux系统下获取MAC地址列表
 *
 * 由于该函数不支持非Windows和非Linux系统，因此直接返回 -1 表示不支持。
 *
 * @param mac_list 用于存储获取到的MAC地址的向量
 * @return int 固定返回 -1，表示不支持该系统
 */
int get_mac_address(std::vector<std::string> &mac_list)
{
    return -1;
}
#endif // __linux__
#endif // _WIN64

zcc_namespace_end;

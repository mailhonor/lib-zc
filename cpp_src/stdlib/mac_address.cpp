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
int get_mac_address(std::vector<std::string> &mac_list)
{
    mac_list.clear();
    std::map<std::string, bool> mac_dict;
    int ret_num = 0;
    PIP_ADAPTER_INFO pIpAdapterInfo = (PIP_ADAPTER_INFO) new char[sizeof(IP_ADAPTER_INFO) + 1];
    ULONG stSize = sizeof(IP_ADAPTER_INFO);
    int nRel = ::GetAdaptersInfo(pIpAdapterInfo, &stSize);
    if (ERROR_BUFFER_OVERFLOW == nRel)
    {
        delete[] (char *)pIpAdapterInfo;
        PIP_ADAPTER_INFO pIpAdapterInfo = (PIP_ADAPTER_INFO) new char[stSize + 1];
        nRel = GetAdaptersInfo(pIpAdapterInfo, &stSize);
    }
    if (ERROR_SUCCESS == nRel)
    {
        PIP_ADAPTER_INFO next;
        for (; pIpAdapterInfo; pIpAdapterInfo = next)
        {
            next = pIpAdapterInfo->Next;
            int ignore = 1;
            switch (pIpAdapterInfo->Type)
            {
            case MIB_IF_TYPE_ETHERNET:
            case MIB_IF_TYPE_FDDI:
            case MIB_IF_TYPE_PPP:
            case MIB_IF_TYPE_SLIP:
                ignore = 0;
                break;
            default:
                break;
            }
            if (ignore)
            {
                continue;
            }
            for (DWORD i = 0; i < pIpAdapterInfo->AddressLength; i++)
            {
                if (pIpAdapterInfo->AddressLength != 6)
                {
                    continue;
                }
                unsigned char *ar = (unsigned char *)(pIpAdapterInfo->Address);
                char mbuf[32];
                std::sprintf(mbuf, "%02X:%02X:%02X:%02X:%02X:%02X", ar[0], ar[1], ar[2], ar[3], ar[4], ar[5]);
                std::string m = mbuf;
                if (mac_dict.find(m) != mac_dict.end())
                {
                    continue;
                }
                mac_dict[m] = true;
                mac_list.push_back(m);
                ret_num++;
            }
        }
    }
    if (pIpAdapterInfo)
    {
        delete[] (char *)pIpAdapterInfo;
    }

    return ret_num;
}

#endif // _WINM64

#ifdef __linux__
int get_mac_address(std::vector<std::string> &mac_list)
{
    int sock_fd;
    struct ifreq *buf;
    struct ifconf ifc;
    int ret_num = 0, interface_num;

    sock_fd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0)
    {
        return -1;
    }
    ifc.ifc_len = sizeof(struct ifreq) * 128;
    buf = (struct ifreq *)new char[sizeof(struct ifreq) * 128];
    ifc.ifc_req = buf;
    if (ioctl(sock_fd, SIOCGIFCONF, (char *)&ifc) < 0)
    {
        delete[] (char *)buf;
        return -1;
    }
    interface_num = ifc.ifc_len / sizeof(struct ifreq);
    for (int ii = 0; ii < interface_num; ii++)
    {
        if (ioctl(sock_fd, SIOCGIFHWADDR, buf + ii) < 0)
        {
            continue;
        }
        unsigned char *ar = (unsigned char *)(buf[ii].ifr_hwaddr.sa_data);
        char mbuf[32];
        std::sprintf(mbuf, "%02X:%02X:%02X:%02X:%02X:%02X", ar[0], ar[1], ar[2], ar[3], ar[4], ar[5]);
        mac_list.push_back(mbuf);
        ret_num++;
    }

    ::close(sock_fd);
    delete[] (char *)buf;
    
    return ret_num;
}
#endif // __linux__

#ifndef _WIN64
#ifndef __linux__
int get_mac_address(std::vector<std::string> &mac_list)
{
    return -1;
}
#endif // __linux__
#endif // _WIN64

zcc_namespace_end;

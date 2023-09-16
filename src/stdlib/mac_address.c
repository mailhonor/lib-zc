/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2017-09-30
 * ================================
 */

#include "zc.h"

#ifdef __linux__
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
int zget_mac_address(zargv_t *mac_list)
{
    int sock_fd;
    struct ifreq *buf;
    struct ifconf ifc;
    int ret_num = 0, interface_num;

    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0)
    {
        return -1;
    }
    ifc.ifc_len = sizeof(struct ifreq) * 128;
    buf = (struct ifreq *)malloc(sizeof(struct ifreq) * 128);
    ifc.ifc_req = buf;
    if (ioctl(sock_fd, SIOCGIFCONF, (char *)&ifc) < 0)
    {
        free(buf);
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
        zsprintf(mbuf, "%02X:%02X:%02X:%02X:%02X:%02X", ar[0], ar[1], ar[2], ar[3], ar[4], ar[5]);
        zargv_add(mac_list, mbuf);
        ret_num++;
    }

    close(sock_fd);

    free(buf);
    return ret_num;
}
#endif // __linux__

#ifdef _WIN32
#include <WinSock2.h>
#include <Iphlpapi.h>
// #pragma comment(lib, "Iphlpapi.lib")

int zget_mac_address(zargv_t *mac_list)
{
    zmap_t *mac_dict = zmap_create();
    int ret_num = 0;
    PIP_ADAPTER_INFO pIpAdapterInfo = (PIP_ADAPTER_INFO)zcalloc(1, sizeof(IP_ADAPTER_INFO));
    unsigned long stSize = sizeof(IP_ADAPTER_INFO);
    int nRel = GetAdaptersInfo(pIpAdapterInfo, &stSize);
    if (ERROR_BUFFER_OVERFLOW == nRel)
    {
        zfree(pIpAdapterInfo);
        pIpAdapterInfo = (PIP_ADAPTER_INFO)zcalloc(1, stSize);
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
            if (ignore) {
                continue;
            }
            for (DWORD i = 0; i < pIpAdapterInfo->AddressLength; i++) {
                if (pIpAdapterInfo->AddressLength != 6) {
                    continue;
                }
                unsigned char *ar = (unsigned char *)(pIpAdapterInfo->Address);
                char mbuf[32];
                zsprintf(mbuf, "%02X:%02X:%02X:%02X:%02X:%02X", ar[0], ar[1], ar[2], ar[3], ar[4], ar[5]);
                if (zmap_find(mac_dict, mbuf, 0))
                {
                    continue;
                }
                zmap_update(mac_dict, mbuf, 0, 0);
                zargv_add(mac_list, mbuf);
                ret_num++;
            }
        }
    }
    if (pIpAdapterInfo)
    {
        zfree(pIpAdapterInfo);
    }
    zmap_free(mac_dict);

    return 0;
}
#endif // WIN32

#ifdef __APPLE__
int zget_mac_address(zargv_t *mac_list)
{
    return -1;
}
#endif // __APPLE__


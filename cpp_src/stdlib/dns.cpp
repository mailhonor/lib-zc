/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-10-13
 * ================================
 */

#include "zcc/zcc_errno.h"
#ifdef _WIN64
#include <WinSock2.h>
#include <ws2tcpip.h>
#else // _WIN64
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <sys/types.h>
#endif // _WIN64

zcc_namespace_begin;

#if (defined _WIN64) || (defined __APPLE__)
int get_hostaddr(const char *host, std::vector<std::string> &addrs)
{
    WSAStartup();
    struct in_addr **addr_list_tmp;
    struct hostent *htr = 0;
    char *ips;
    int ret_count = 0;

    if (INADDR_NONE != inet_addr(host))
    {
        addrs.push_back(host);
        return 1;
    }

    htr = gethostbyname(host);
    if (htr)
    {
        addr_list_tmp = (struct in_addr **)htr->h_addr_list;
        for (int i = 0; addr_list_tmp[i] != 0; i++)
        {
            ips = inet_ntoa(*(addr_list_tmp[i]));
            if (!ips)
            {
                continue;
            }
            addrs.push_back(ips);
            ret_count++;
        }
    }
    return ret_count;
}
#else  // _WIN64
int get_localaddr(std::vector<std::string> &addrs)
{
    struct ifaddrs *ifaddr, *ifa;
    struct sockaddr_in *scin;
    char ipbuf[32];
    int ret_count = 0;

    if (getifaddrs(&ifaddr) == -1)
    {
        return 0;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
        {
            continue;
        }
        if (ifa->ifa_addr->sa_family != AF_INET)
        {
            continue;
        }
        scin = (struct sockaddr_in *)(ifa->ifa_addr);
        inet_ntop(AF_INET, &(scin->sin_addr), ipbuf, 16);
        addrs.push_back(ipbuf);
        ret_count++;
    }

    freeifaddrs(ifaddr);

    return ret_count;
}

int get_hostaddr(const char *host, std::vector<std::string> &addrs)
{
    struct in_addr **addr_list_tmp;
    struct hostent htt, *htr = 0;
    char *tmpbuf, ipbuf[32];
    int tmpbuflen = 4096, hterror;
    int ret_count = 0;

    if (empty(host))
    {
        return get_localaddr(addrs);
    }

    if (INADDR_NONE != inet_addr(host))
    {
        addrs.push_back(host);
        return 1;
    }

    tmpbuf = new char[tmpbuflen + 1];
    while (gethostbyname_r(host, &htt, tmpbuf, tmpbuflen, &htr, &hterror))
    {
        if (hterror == NETDB_INTERNAL && get_errno() == ZCC_ERANGE)
        {
            tmpbuflen *= 2;
            delete[] tmpbuf;
            tmpbuf = new char[tmpbuflen + 1];
        }
        else
        {
            break;
        }
    }
    if (htr)
    {
        addr_list_tmp = (struct in_addr **)htr->h_addr_list;
        for (int i = 0; addr_list_tmp[i] != 0; i++)
        {
            inet_ntop(AF_INET, (void *)(addr_list_tmp[i]), ipbuf, 16);
            addrs.push_back(ipbuf);
            ret_count++;
        }
    }
    delete[] tmpbuf;
    return ret_count;
}
#endif // _WIN64

bool is_ip(const char *ip)
{
    if (INADDR_NONE != inet_addr(ip))
    {
        return true;
    }
    return false;
}

char *get_ipstring(int ip, char *ipstr)
{
#ifdef __linux__
    return (char *)(void *)inet_ntop(AF_INET, &ip, ipstr, 16);
#endif // __linux__

#ifdef _WIN64
    struct in_addr addr;
    addr.s_addr = ip;
    return (char *)(void *)inet_ntoa(addr);
#endif // _WIN64
    std::sprintf(ipstr, "%d.%d.%d.%d", (ip >> 24) & 0XFF, (ip >> 16) & 0XFF, (ip >> 8) & 0XFF, ip & 0XFF);
    return ipstr;
}

std::string get_ipstring(int ip)
{
    char ipstr[18];
    return get_ipstring(ip, ipstr);
}

int get_ipint(const char *ipstr)
{
    int ip = inet_addr(ipstr);
    if ((unsigned int)ip == INADDR_NONE)
    {
        return 0;
    }
    return ip;
}

static int ___ip_switch(int ip)
{
    int ip_switch;
    char *p1 = (char *)&ip;
    char *p2 = (char *)&ip_switch;
    p2[0] = p1[3];
    p2[1] = p1[2];
    p2[2] = p1[1];
    p2[3] = p1[0];
    return ip_switch;
}

static int ___get_netmask(int masklen)
{
    if ((masklen < 1) || (masklen > 32))
    {
        return 0;
    }
    int mask = 0;
    for (int mi = masklen; mi < 32; mi++)
    {
        mask = mask << 1;
        mask += 1;
    }
    mask = ~mask;
    return mask;
}

int get_network(int ip, int masklen)
{
    int nip = ___ip_switch(ip);
    int mask = ___get_netmask(masklen);
    return ___ip_switch(nip & mask);
}

int get_netmask(int masklen)
{
    return ___ip_switch(___get_netmask(masklen));
}

int get_broadcast(int ip, int masklen)
{
    int nip = ___ip_switch(ip);
    int mask = ___get_netmask(masklen);
    return ___ip_switch(nip | (~mask));
}

int get_ip_min(int ip, int masklen)
{
    int nip = ___ip_switch(ip);
    int mask = ___get_netmask(masklen);
    return ___ip_switch((nip & mask) + 1);
}

int get_ip_max(int ip, int masklen)
{
    int nip = ___ip_switch(ip);
    int mask = ___get_netmask(masklen);
    return ___ip_switch((nip | (~mask)) - 1);
}

int is_intranet(int ip)
{
    int a = ((unsigned char *)&ip)[0];
    int b = ((unsigned char *)&ip)[1];

    if ((a == 127) || (a == 10))
    {
        return 1;
    }
    if ((a == 192) && (b == 168))
    {
        return 1;
    }
    if ((a == 172) && (15 < b) && (b < 32))
    {
        return 1;
    }

    return 0;
}

int is_intranet2(const char *ip)
{
    if (!ip)
    {
        return 0;
    }
    if ((!strncmp(ip, "127.", 4)) || (!strncmp(ip, "10.", 3)) || (!strncmp(ip, "192.168.", 8)))
    {
        return 1;
    }
    if ((!strncmp(ip, "172.", 4)) && ip[4] && ip[5] && (ip[6] == '.'))
    {
        int a = (ip[4] - '0') * 10 + (ip[5] - '0');
        if ((a > 15) && (a < 32))
        {
            return 1;
        }
    }
    return 0;
}

zcc_namespace_end;

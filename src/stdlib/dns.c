/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-10-13
 * ================================
 */

#include "zc.h"
#ifdef _WIN32
#include <ws2tcpip.h>
#else // _WIN32
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <errno.h>
#endif // _WIN32
#include <sys/types.h>

#if (defined _WIN32) || (defined __APPLE__)
int zget_hostaddr(const char *host, zargv_t *addrs)
{
    zWSAStartup();
    struct in_addr **addr_list_tmp;
    struct hostent *htr = 0;
    char *ips;
    int ret_count = 0;

    if (INADDR_NONE != inet_addr(host))
    {
        zargv_add(addrs, host);
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
            zargv_add(addrs, ips);
            ret_count++;
        }
    }
    return ret_count;
}
#else  // _WIN32
int zget_localaddr(zargv_t *addrs)
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
        zargv_add(addrs, ipbuf);
        ret_count++;
    }

    freeifaddrs(ifaddr);

    return ret_count;
}

int zget_hostaddr(const char *host, zargv_t *addrs)
{
    struct in_addr **addr_list_tmp;
    struct hostent htt, *htr = 0;
    char *tmpbuf, ipbuf[32];
    int tmpbuflen = 4096, hterror;
    int ret_count = 0;

    if (zempty(host))
    {
        return zget_localaddr(addrs);
    }

    if (INADDR_NONE != inet_addr(host))
    {
        zargv_add(addrs, host);
        return 1;
    }

    tmpbuf = (char *)zmalloc(tmpbuflen + 1);
    while (gethostbyname_r(host, &htt, tmpbuf, tmpbuflen, &htr, &hterror))
    {
        if (hterror == NETDB_INTERNAL && errno == ERANGE)
        {
            tmpbuflen *= 2;
            tmpbuf = (char *)zrealloc(tmpbuf, tmpbuflen + 1);
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
            zargv_add(addrs, ipbuf);
            ret_count++;
        }
    }
    zfree(tmpbuf);
    return ret_count;
}
#endif // _WIN32

int zis_ip(const char *ip)
{
    if (INADDR_NONE != inet_addr(ip))
    {
        return 1;
    }
    return 0;
}

int zget_peername(int sockfd, int *host, int *port)
{
    zWSAStartup();
    struct sockaddr_in sa;
    socklen_t sa_length = sizeof(struct sockaddr);

    if (getpeername(sockfd, (struct sockaddr *)&sa, &sa_length) < 0)
    {
        return 0;
    }

    if (host)
    {
        *host = *((int *)&(sa.sin_addr));
    }

    if (port)
    {
        *port = ntohs(sa.sin_port);
    }

    return 1;
}

char *zget_ipstring(int ip, char *ipstr)
{
#ifdef __linux__
    return (char *)(void *)inet_ntop(AF_INET, &ip, ipstr, 16);
#endif // __linux__

#ifdef _WIN32
    struct in_addr addr;
    addr.s_addr = ip;
    return (char *)(void *)inet_ntoa(addr);
#endif // _WIN32
    zsprintf(ipstr, "%d.%d.%d.%d", (ip >> 24) & 0XFF, (ip >> 16) & 0XFF, (ip >> 8) & 0XFF, ip & 0XFF);
    return ipstr;
}

int zget_ipint(const char *ipstr)
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

int zget_network(int ip, int masklen)
{
    int nip = ___ip_switch(ip);
    int mask = ___get_netmask(masklen);
    return ___ip_switch(nip & mask);
}

int zget_netmask(int masklen)
{
    return ___ip_switch(___get_netmask(masklen));
}

int zget_broadcast(int ip, int masklen)
{
    int nip = ___ip_switch(ip);
    int mask = ___get_netmask(masklen);
    return ___ip_switch(nip | (~mask));
}

int zget_ip_min(int ip, int masklen)
{
    int nip = ___ip_switch(ip);
    int mask = ___get_netmask(masklen);
    return ___ip_switch((nip & mask) + 1);
}

int zget_ip_max(int ip, int masklen)
{
    int nip = ___ip_switch(ip);
    int mask = ___get_netmask(masklen);
    return ___ip_switch((nip | (~mask)) - 1);
}

int zip_is_intranet(int ip)
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

int zip_is_intranet2(const char *ip)
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

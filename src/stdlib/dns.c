/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-10-13
 * ================================
 */

#include "libzc.h"
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <sys/types.h>

int zgetlocaladdr(zaddr_t * addr_list, int max_count)
{
    struct ifaddrs *ifaddr, *ifa;
    struct sockaddr_in *scin;
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
        inet_ntop(AF_INET, &(scin->sin_addr), (addr_list++)->addr, 16);
        ret_count++;
    }

    freeifaddrs(ifaddr);

    return ret_count;
}

int zgetaddr(char *host, zaddr_t * addr_list, int max_count)
{
    struct in_addr **addr_list_tmp;
    struct hostent htt, *htr = 0;
    char hbuf[4096], *tmpbuf;
    int tmpbuflen = 4096, hterror;
    int alloc_flag = 0, i;
    int ret_count = 0;

    if (ZEMPTY(host))
    {
        return zgetlocaladdr(addr_list, max_count);
    }

    tmpbuf = hbuf;
    while (gethostbyname_r(host, &htt, tmpbuf, tmpbuflen, &htr, &hterror))
    {
        if (hterror == NETDB_INTERNAL && errno == ERANGE)
        {
            tmpbuflen *= 2;
            if (alloc_flag)
            {
                tmpbuf = (char *)zrealloc(tmpbuf, tmpbuflen);
            }
            else
            {
                tmpbuf = (char *)zmalloc(tmpbuflen);
                alloc_flag = 1;
            }
        }
        else
        {
            break;
        }
    }
    if (htr)
    {
        addr_list_tmp = (struct in_addr **)htr->h_addr_list;
        for (i = 0; addr_list_tmp[i] != 0 && i < max_count; i++)
        {
            inet_ntop(AF_INET, addr_list_tmp[i], addr_list[i].addr, 16);
            ret_count++;
        }
    }
    if (alloc_flag)
    {
        zfree(tmpbuf);
    }

    return ret_count;
}

int zgetpeer(int sockfd, int *host, int *port)
{
    struct sockaddr_in sa;
    socklen_t sa_length = sizeof(struct sockaddr);

    if (getpeername(sockfd, (struct sockaddr *)&sa, &sa_length) < 0)
    {
        return -1;
    }

    if (host)
    {
        *host = *((int *)&(sa.sin_addr));
    }

    if (port)
    {
        *port = ntohs(sa.sin_port);
    }

    return 0;
}

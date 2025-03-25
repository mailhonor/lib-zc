/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-10-13
 * ================================
 */

//  该文件包含了与网络地址处理相关的函数实现，支持不同操作系统（Windows、Linux、macOS 等）。

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

/**
 * @brief 在 Windows 或 macOS 系统下，根据主机名获取对应的 IPv4 地址列表。
 *
 * @param host 主机名或 IP 地址字符串。
 * @param addrs 用于存储获取到的 IPv4 地址的向量。
 * @return int 返回成功获取到的 IPv4 地址的数量。
 */
#if (defined _WIN64) || (defined __APPLE__)
int get_hostaddr(const char *host, std::vector<std::string> &addrs)
{
    // 初始化 Windows 套接字库
    WSAStartup();
    // 用于临时存储地址列表的指针
    struct in_addr **addr_list_tmp;
    // 指向主机信息结构体的指针
    struct hostent *htr = 0;
    // 用于存储转换后的 IP 地址字符串
    char *ips;
    // 记录成功获取到的 IP 地址数量
    int ret_count = 0;

    // 检查输入的是否为有效的 IPv4 地址
    if (INADDR_NONE != inet_addr(host))
    {
        // 如果是有效 IP 地址，直接添加到向量中
        addrs.push_back(host);
        return 1;
    }

    // 根据主机名获取主机信息
    htr = gethostbyname(host);
    if (htr)
    {
        // 获取地址列表
        addr_list_tmp = (struct in_addr **)htr->h_addr_list;
        // 遍历地址列表
        for (int i = 0; addr_list_tmp[i] != 0; i++)
        {
            // 将二进制 IP 地址转换为点分十进制字符串
            ips = inet_ntoa(*(addr_list_tmp[i]));
            if (!ips)
            {
                // 转换失败，跳过
                continue;
            }
            // 将转换后的 IP 地址添加到向量中
            addrs.push_back(ips);
            // 成功获取到一个 IP 地址，计数器加 1
            ret_count++;
        }
    }
    return ret_count;
}
#else  // _WIN64
/**
 * @brief 获取本地主机的所有 IPv4 地址，并将这些地址存储在传入的向量中。
 *
 * @param addrs 用于存储本地 IPv4 地址的向量的引用。
 * @return int 返回成功获取到的 IPv4 地址的数量。如果获取失败，返回 0。
 */
int get_localaddr(std::vector<std::string> &addrs)
{
    // 定义指向 ifaddrs 结构体链表的指针，ifaddr 指向链表头，ifa 用于遍历链表
    struct ifaddrs *ifaddr, *ifa;
    // 定义指向 sockaddr_in 结构体的指针，用于处理 IPv4 地址
    struct sockaddr_in *scin;
    // 用于存储转换后的点分十进制 IPv4 地址字符串的缓冲区
    char ipbuf[32];
    // 记录成功获取到的 IPv4 地址的数量
    int ret_count = 0;

    // 调用 getifaddrs 函数获取本地网络接口的地址信息
    // 如果返回值为 -1，表示获取失败，直接返回 0
    if (getifaddrs(&ifaddr) == -1)
    {
        return 0;
    }

    // 遍历 ifaddrs 结构体链表
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        // 如果当前接口的地址信息为空，跳过该接口
        if (ifa->ifa_addr == NULL)
        {
            continue;
        }
        // 如果当前接口的地址族不是 IPv4 地址族（AF_INET），跳过该接口
        if (ifa->ifa_addr->sa_family != AF_INET)
        {
            continue;
        }
        // 将 ifa->ifa_addr 强制转换为 sockaddr_in 类型，以便访问 IPv4 地址信息
        scin = (struct sockaddr_in *)(ifa->ifa_addr);
        // 将二进制的 IPv4 地址转换为点分十进制的字符串，并存储在 ipbuf 中
        inet_ntop(AF_INET, &(scin->sin_addr), ipbuf, 16);
        // 将转换后的 IP 地址字符串添加到 addrs 向量中
        addrs.push_back(ipbuf);
        // 成功获取到一个 IP 地址，计数器加 1
        ret_count++;
    }

    // 释放 getifaddrs 函数分配的内存
    freeifaddrs(ifaddr);

    // 返回成功获取到的 IPv4 地址的数量
    return ret_count;
}

/**
 * @brief 在非 Windows 系统下，根据主机名获取对应的 IPv4 地址列表。
 *
 * @param host 主机名或 IP 地址字符串。如果为空，则获取本地地址。
 * @param addrs 用于存储获取到的 IPv4 地址的向量。
 * @return int 返回成功获取到的 IPv4 地址的数量。
 */
int get_hostaddr(const char *host, std::vector<std::string> &addrs)
{
    // 用于临时存储地址列表的指针
    struct in_addr **addr_list_tmp;
    // 主机信息结构体
    struct hostent htt, *htr = 0;
    // 临时缓冲区指针
    char *tmpbuf;
    // 用于存储转换后的 IP 地址字符串的缓冲区
    char ipbuf[32];
    // 临时缓冲区的初始长度
    int tmpbuflen = 4096;
    // 错误码
    int hterror;
    // 记录成功获取到的 IP 地址数量
    int ret_count = 0;

    // 检查输入的主机名是否为空
    if (empty(host))
    {
        // 如果为空，调用 get_localaddr 函数获取本地地址
        return get_localaddr(addrs);
    }

    // 检查输入的是否为有效的 IPv4 地址
    if (INADDR_NONE != inet_addr(host))
    {
        // 如果是有效 IP 地址，直接添加到向量中
        addrs.push_back(host);
        return 1;
    }

    // 分配临时缓冲区
    tmpbuf = new char[tmpbuflen + 1];
    // 循环调用 gethostbyname_r 函数，直到成功或出现不可恢复的错误
    while (gethostbyname_r(host, &htt, tmpbuf, tmpbuflen, &htr, &hterror))
    {
        if (hterror == NETDB_INTERNAL && get_errno() == ZCC_ERANGE)
        {
            // 如果缓冲区长度不足，扩大缓冲区
            tmpbuflen *= 2;
            delete[] tmpbuf;
            tmpbuf = new char[tmpbuflen + 1];
        }
        else
        {
            // 出现其他错误，跳出循环
            break;
        }
    }
    if (htr)
    {
        // 获取地址列表
        addr_list_tmp = (struct in_addr **)htr->h_addr_list;
        // 遍历地址列表
        for (int i = 0; addr_list_tmp[i] != 0; i++)
        {
            // 将二进制 IP 地址转换为点分十进制字符串
            inet_ntop(AF_INET, (void *)(addr_list_tmp[i]), ipbuf, 16);
            // 将转换后的 IP 地址添加到向量中
            addrs.push_back(ipbuf);
            // 成功获取到一个 IP 地址，计数器加 1
            ret_count++;
        }
    }
    // 释放临时缓冲区
    delete[] tmpbuf;
    return ret_count;
}
#endif // _WIN64

/**
 * @brief 检查输入的字符串是否为有效的 IPv4 地址。
 *
 * @param ip 待检查的字符串。
 * @return bool 如果是有效的 IPv4 地址，返回 true；否则返回 false。
 */
bool is_ip(const char *ip)
{
    // 检查输入的是否为有效的 IPv4 地址
    if (INADDR_NONE != inet_addr(ip))
    {
        return true;
    }
    return false;
}

/**
 * @brief 将 32 位整数表示的 IPv4 地址转换为点分十进制字符串。
 *
 * @param ip 32 位整数表示的 IPv4 地址。
 * @param ipstr 用于存储转换结果的字符数组。
 * @return char* 返回指向转换结果的指针。
 */
char *get_ipstring(int ip, char *ipstr)
{
#ifdef __linux__
    // 在 Linux 系统下，使用 inet_ntop 函数进行转换
    return (char *)(void *)inet_ntop(AF_INET, &ip, ipstr, 16);
#endif // __linux__

#ifdef _WIN64
    // 在 Windows 系统下，使用 inet_ntoa 函数进行转换
    struct in_addr addr;
    addr.s_addr = ip;
    return (char *)(void *)inet_ntoa(addr);
#endif // _WIN64
    // 在其他系统下，使用 sprintf 函数手动转换
    std::sprintf(ipstr, "%d.%d.%d.%d", (ip >> 24) & 0XFF, (ip >> 16) & 0XFF, (ip >> 8) & 0XFF, ip & 0XFF);
    return ipstr;
}

/**
 * @brief 将 32 位整数表示的 IPv4 地址转换为点分十进制字符串。
 *
 * @param ip 32 位整数表示的 IPv4 地址。
 * @return std::string 返回转换后的点分十进制字符串。
 */
std::string get_ipstring(int ip)
{
    // 定义一个字符数组用于存储转换结果
    char ipstr[18];
    // 调用 get_ipstring 函数进行转换
    return get_ipstring(ip, ipstr);
}

/**
 * @brief 将点分十进制字符串表示的 IPv4 地址转换为 32 位整数。
 *
 * @param ipstr 点分十进制字符串表示的 IPv4 地址。
 * @return int 返回转换后的 32 位整数。如果转换失败，返回 0。
 */
int get_ipint(const char *ipstr)
{
    // 使用 inet_addr 函数进行转换
    int ip = inet_addr(ipstr);
    if ((unsigned int)ip == INADDR_NONE)
    {
        // 转换失败，返回 0
        return 0;
    }
    return ip;
}

/**
 * @brief 交换 32 位整数的字节顺序。
 *
 * @param ip 32 位整数表示的 IPv4 地址。
 * @return int 返回字节顺序交换后的 32 位整数。
 */
static int ___ip_switch(int ip)
{
    // 用于存储交换后的结果
    int ip_switch;
    // 指向输入整数的指针
    char *p1 = (char *)&ip;
    // 指向输出整数的指针
    char *p2 = (char *)&ip_switch;
    // 交换字节顺序
    p2[0] = p1[3];
    p2[1] = p1[2];
    p2[2] = p1[1];
    p2[3] = p1[0];
    return ip_switch;
}

/**
 * @brief 根据子网掩码长度生成对应的 32 位整数表示的子网掩码。
 *
 * @param masklen 子网掩码长度，范围为 1 到 32。
 * @return int 返回 32 位整数表示的子网掩码。如果输入无效，返回 0。
 */
static int ___get_netmask(int masklen)
{
    // 检查输入的子网掩码长度是否有效
    if ((masklen < 1) || (masklen > 32))
    {
        return 0;
    }
    // 用于存储生成的子网掩码
    int mask = 0;
    // 生成子网掩码
    for (int mi = masklen; mi < 32; mi++)
    {
        mask = mask << 1;
        mask += 1;
    }
    // 取反得到最终的子网掩码
    mask = ~mask;
    return mask;
}

/**
 * @brief 根据 IP 地址和子网掩码长度计算网络地址。
 *
 * @param ip 32 位整数表示的 IPv4 地址。
 * @param masklen 子网掩码长度，范围为 1 到 32。
 * @return int 返回 32 位整数表示的网络地址。
 */
int get_network(int ip, int masklen)
{
    // 交换 IP 地址的字节顺序
    int nip = ___ip_switch(ip);
    // 生成子网掩码
    int mask = ___get_netmask(masklen);
    // 计算网络地址并交换字节顺序
    return ___ip_switch(nip & mask);
}

/**
 * @brief 根据子网掩码长度获取对应的 32 位整数表示的子网掩码。
 *
 * @param masklen 子网掩码长度，范围为 1 到 32。
 * @return int 返回 32 位整数表示的子网掩码。
 */
int get_netmask(int masklen)
{
    // 生成子网掩码并交换字节顺序
    return ___ip_switch(___get_netmask(masklen));
}

/**
 * @brief 根据 IP 地址和子网掩码长度计算广播地址。
 *
 * @param ip 32 位整数表示的 IPv4 地址。
 * @param masklen 子网掩码长度，范围为 1 到 32。
 * @return int 返回 32 位整数表示的广播地址。
 */
int get_broadcast(int ip, int masklen)
{
    // 交换 IP 地址的字节顺序
    int nip = ___ip_switch(ip);
    // 生成子网掩码
    int mask = ___get_netmask(masklen);
    // 计算广播地址并交换字节顺序
    return ___ip_switch(nip | (~mask));
}

/**
 * @brief 根据 IP 地址和子网掩码长度计算子网内的最小可用 IP 地址。
 *
 * @param ip 32 位整数表示的 IPv4 地址。
 * @param masklen 子网掩码长度，范围为 1 到 32。
 * @return int 返回 32 位整数表示的最小可用 IP 地址。
 */
int get_ip_min(int ip, int masklen)
{
    // 交换 IP 地址的字节顺序
    int nip = ___ip_switch(ip);
    // 生成子网掩码
    int mask = ___get_netmask(masklen);
    // 计算最小可用 IP 地址并交换字节顺序
    return ___ip_switch((nip & mask) + 1);
}

/**
 * @brief 根据 IP 地址和子网掩码长度计算子网内的最大可用 IP 地址。
 *
 * @param ip 32 位整数表示的 IPv4 地址。
 * @param masklen 子网掩码长度，范围为 1 到 32。
 * @return int 返回 32 位整数表示的最大可用 IP 地址。
 */
int get_ip_max(int ip, int masklen)
{
    // 交换 IP 地址的字节顺序
    int nip = ___ip_switch(ip);
    // 生成子网掩码
    int mask = ___get_netmask(masklen);
    // 计算最大可用 IP 地址并交换字节顺序
    return ___ip_switch((nip | (~mask)) - 1);
}

/**
 * @brief 检查 32 位整数表示的 IPv4 地址是否为内网地址。
 *
 * @param ip 32 位整数表示的 IPv4 地址。
 * @return int 如果是内网地址，返回 1；否则返回 0。
 */
int is_intranet(int ip)
{
    // 获取 IP 地址的前两个字节
    int a = ((unsigned char *)&ip)[0];
    int b = ((unsigned char *)&ip)[1];

    // 检查是否为 127.x.x.x 或 10.x.x.x
    if ((a == 127) || (a == 10))
    {
        return 1;
    }
    // 检查是否为 192.168.x.x
    if ((a == 192) && (b == 168))
    {
        return 1;
    }
    // 检查是否为 172.16.x.x 到 172.31.x.x
    if ((a == 172) && (15 < b) && (b < 32))
    {
        return 1;
    }

    return 0;
}

/**
 * @brief 检查点分十进制字符串表示的 IPv4 地址是否为内网地址。
 *
 * @param ip 点分十进制字符串表示的 IPv4 地址。
 * @return int 如果是内网地址，返回 1；否则返回 0。
 */
int is_intranet2(const char *ip)
{
    // 检查输入是否为空
    if (!ip)
    {
        return 0;
    }
    // 检查是否为 127.x.x.x、10.x.x.x 或 192.168.x.x
    if ((!strncmp(ip, "127.", 4)) || (!strncmp(ip, "10.", 3)) || (!strncmp(ip, "192.168.", 8)))
    {
        return 1;
    }
    // 检查是否为 172.16.x.x 到 172.31.x.x
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

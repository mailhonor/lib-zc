/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2019-11-14
 * ================================
 */

#include "zc.h"

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("USAGE: %s ip [ mask-length ]\n", argv[0]);
        return 1;
    }
    char *ipstr = argv[1];
    int masklen = 32;
    int ipint;

    if (argc > 2) {
        masklen = atoi(argv[2]);
    }

    printf("输入: %s %d\n\n", ipstr, masklen);
    ipint = zget_ipint(ipstr);
    printf("ipint: %u\n", (unsigned int)ipint);
    printf("masklen: %d\n", masklen);

    if (masklen == -1) {
        return 0;
    }
    if ((masklen < 1) || (masklen > 32)) {
        printf("ERR unknown mask-length: %s\n", argv[2]);
        return 1;
    }

    char ipbuf[20];
    printf("网络地址: %s\n", zget_ipstring(zget_network(ipint, masklen), ipbuf));
    printf("子网掩码: %s\n", zget_ipstring(zget_netmask(masklen), ipbuf));
    printf("广播地址: %s\n", zget_ipstring(zget_broadcast(ipint, masklen), ipbuf));
    printf("最小地址: %s\n", zget_ipstring(zget_ip_min(ipint, masklen), ipbuf));
    printf("最大地址: %s\n", zget_ipstring(zget_ip_max(ipint, masklen), ipbuf));
    printf("保留地址: %s\n", zip_is_intranet(ipint)?"yes":"no");

    return 0;
}

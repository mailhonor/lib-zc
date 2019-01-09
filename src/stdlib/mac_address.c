/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-09-30
 * ================================
 */

#include "zc.h"
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
    if (sock_fd < 0) {
        return -1;
    }
    ifc.ifc_len = sizeof(struct ifreq) * 128;
    buf = (struct ifreq *)malloc(sizeof(struct ifreq) * 128);
    ifc.ifc_req = buf;
    if (ioctl(sock_fd, SIOCGIFCONF, (char *)&ifc) < 0) {
        free(buf);
        return -1;
    }
    interface_num = ifc.ifc_len / sizeof(struct ifreq);
    for (int ii = 0; ii < interface_num; ii++) {
        if (ioctl(sock_fd, SIOCGIFHWADDR, buf+ii) < 0) {
            continue;
        }
        unsigned char *ar = (unsigned char *)(buf[ii].ifr_hwaddr.sa_data);
        char mbuf[32];
        sprintf(mbuf, "%02X:%02X:%02X:%02X:%02X:%02X", ar[0], ar[1], ar[2], ar[3], ar[4], ar[5]);
        zargv_add(mac_list, mbuf);
        ret_num++;
    }

    close(sock_fd);

    free(buf);
    return ret_num;
}

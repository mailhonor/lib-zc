/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2016-09-09
 * ================================
 */

#include "libzc.h"
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>

typedef struct {
    char val[20];
} __MAC;

static int get_mac_list(__MAC * mac_list)
{
    int sock_fd;
    struct ifreq buf[128];
    struct ifconf ifc;
    int ret_num = 0, interface_num;

    if ((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        return -1;
    }
    ifc.ifc_len = sizeof(buf);
    ifc.ifc_req = buf;
    if (ioctl(sock_fd, SIOCGIFCONF, (char *)&ifc) < 0) {
        return -1;
    }
    interface_num = ifc.ifc_len / sizeof(struct ifreq);

    while (interface_num--) {

        if (ioctl(sock_fd, SIOCGIFHWADDR, (char *)&buf[interface_num]) < 0) {
            continue;
        }

        sprintf(mac_list[ret_num].val, "%02X:%02X:%02X:%02X:%02X:%02X", (unsigned char)buf[interface_num].ifr_hwaddr.sa_data[0], (unsigned char)buf[interface_num].ifr_hwaddr.sa_data[1], (unsigned char)buf[interface_num].ifr_hwaddr.sa_data[2], (unsigned char)buf[interface_num].ifr_hwaddr.sa_data[3], (unsigned char)buf[interface_num].ifr_hwaddr.sa_data[4], (unsigned char)buf[interface_num].ifr_hwaddr.sa_data[5]);
        ret_num++;
    }

    close(sock_fd);

    return ret_num;
}

int zlicense_mac_check(char *salt, char *license)
{
    char *mac;
    char license_c[20];
    __MAC mac_list[128];
    int mac_num, i;

    if (!license || !*license) {
        return 0;
    }

    mac_num = get_mac_list(mac_list);
    for (i = 0; i < mac_num; i++) {
        mac = mac_list[i].val;
        printf("SSS:%s\n", mac);
        zlicense_mac_build(salt, mac, license_c);
        if (!strncasecmp(license_c, license, 12)) {
            return 1;
        }
    }

    return 0;
}

void zlicense_mac_build(char *salt, char *_mac, char *rbuf)
{
    char mac[128];
    char buf[512];
    unsigned char *p;
    int i;
    unsigned long crc;

    strcpy(mac, _mac);
    ztoupper(mac);
    sprintf(buf, "%s,%s", salt, mac);
    crc = zcrc64(buf, strlen(buf), 0);

    p = (unsigned char *)(&crc);
    for (i = 0; i < 8; i++) {
        sprintf(rbuf + i * 2, "%02X", *p++);
    }
    rbuf[16] = 0;
}

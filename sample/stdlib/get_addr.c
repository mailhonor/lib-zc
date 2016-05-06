/*
 * ================================
 * eli960@163.com
 * http://www.mailhonor.com/
 * 2015-10-21
 * ================================
 */

#include "libzc.h"

static void get_host_addr(char *host)
{
    zaddr_t addr_list[128];
    int count, i;

    count = zgetaddr(host, addr_list, 128);
    if (count == 0) {
        zinfo("%s'addr none", host);
        return;
    }
    zinfo("%s'addr list(%d):", host, count);
    for (i = 0; i < count; i++) {
        zinfo("        %s", addr_list[i].addr);
    }
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        zinfo("USAGE: %s host_or_domain\n", argv[0]);
        return 0;
    }
    get_host_addr(argv[1]);

    return 0;
}

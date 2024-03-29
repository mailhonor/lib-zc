/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-10-21
 * ================================
 */

#include "zc.h"

static void get_host_addr(char *host)
{
    zargv_t *addr_list =  zargv_create(0);
    int count;

    count = zget_hostaddr(host, addr_list);
    if (count == 0) {
        zprintf("%s'addr none\n", host);
    } else if (count < 0) {
        zprintf("%s'addr error\n", host);
    } else {
        zprintf("%s'addr list(%d):\n", host, count);
        ZARGV_WALK_BEGIN(addr_list, ip) {
            zprintf("    %s\n", ip);
        } ZARGV_WALK_END;
    }
    zargv_free(addr_list);
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        zprintf("USAGE: %s host_or_domain\n", argv[0]);
        return 0;
    }
    get_host_addr(argv[1]);

    return 0;
}

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
    zargv_t *ms = zargv_create(-1);
    if (zget_mac_address(ms) < 0) {
        printf("ERR can not get mac address(%m)\n");
        zargv_free(ms);
        return 1;
    }
    printf("found %d mac address\n", zargv_len(ms));
    ZARGV_WALK_BEGIN(ms, mac) {
        printf("%s\n", mac);
    } ZARGV_WALK_END;
    zargv_free(ms);
    return 0;
}

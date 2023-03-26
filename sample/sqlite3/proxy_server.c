/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2017-08-02
 * ================================
 */

#ifdef _LIB_ZC_SQLITE3_

#include "zc.h"

int main(int argc, char **argv)
{
    return zsqlite3_proxy_server_main(argc, argv);
}

#else
#include <stdio.h>
int main(int argc, char **argv)
{
    printf("unsupported; cmake ../ -DENABLE_SQLITE=yes\n");
    return 0;
}
#endif

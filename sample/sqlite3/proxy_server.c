/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
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
    printf("need defined _LIB_ZC_SQLITE3_\n");
    printf("cat makefiles/defined.include\n");
    printf("\n");
    printf("EXTRA_CFLAGS = -D_LIB_ZC_SQLITE3_\n");
    return 0;
}
#endif

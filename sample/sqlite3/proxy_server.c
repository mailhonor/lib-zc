/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2017-08-02
 * ================================
 */

#include "zc.h"

#ifdef _LIB_ZC_SQLITE3_
int main(int argc, char **argv)
{
    return zsqlite3_proxy_server_main(argc, argv);
}

#else
int main(int argc, char **argv)
{
    zprintf("unsupported; cmake ../ -DENABLE_SQLITE=yes\n");
    return 0;
}
#endif

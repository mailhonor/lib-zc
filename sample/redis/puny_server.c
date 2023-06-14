/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2018-01-23
 * ================================
 */

#include "zc.h"

int main(int argc, char **argv)
{
#ifdef __linux__
    return zredis_puny_server_main(argc, argv);
#else // __linux__
    zfatal("only run on linux");
#endif // __linux__
}

/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-11-24
 * ================================
 */

#include "zcc/zcc_server.h"

int main(int argc, char **argv)
{
    zcc::master_server ms;
    ms.main_run(argc, argv);
    return 0;
}

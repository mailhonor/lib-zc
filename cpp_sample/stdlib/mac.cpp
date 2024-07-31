/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2019-11-14
 * ================================
 */

#include "zcc/zcc_stdlib.h"

int main(int argc, char **argv)
{
    std::vector<std::string> ms;
    if (zcc::get_mac_address(ms) < 0)
    {
        zcc_error("can not get mac address");
        return 1;
    }
    zcc_info("found %d mac address", ms.size());
    for (auto it = ms.begin(); it != ms.end(); it++)
    {
        zcc_info("%s", it->c_str());
    }

    return 0;
}

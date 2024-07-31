/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-10-21
 * ================================
 */

#include "zcc/zcc_stdlib.h"

static void get_host_addr(char *host)
{
    std::vector<std::string> addr_list;
    int count;

    count = zcc::get_hostaddr(host, addr_list);
    if (count == 0)
    {
        std::printf("%s'addr none\n", host);
    }
    else if (count < 0)
    {
        std::printf("%s'addr error\n", host);
    }
    else
    {
        std::printf("%s'addr list(%d):\n", host, count);
        for (auto it = addr_list.begin(); it != addr_list.end(); it++)
        {
            std::printf("    %s\n", it->c_str());
        }
    }
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::printf("USAGE: %s host_or_domain\n", argv[0]);
        return 0;
    }
    get_host_addr(argv[1]);

    return 0;
}

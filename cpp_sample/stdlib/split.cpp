/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2024-04-25
 * ================================
 */

#include "zcc/zcc_stdlib.h"
#include <iostream>

int main(int argc, char **argv)
{
    zcc::main_argument::run(argc, argv);
    if (argc < 3)
    {
        zcc_fatal("USAGE: %s 'str1' 'str2'", zcc::progname);
    }
    auto r = zcc::split(argv[1], argv[2]);
    for (auto it = r.begin(); it != r.end(); it++)
    {
        std::cout << "::: " << *it << std::endl;
    }
}
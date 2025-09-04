/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2025-08-15
 * ================================
 */

#include "zcc/zcc_stdlib.h"

int main(int argc, char **argv)
{
    auto id = zcc::build_unique_id();
    zcc_info("id: %s", id.c_str());

    return 0;
}
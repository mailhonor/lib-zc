/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2024-04-27
 * ================================
 */

#include <random>
#include "zcc/zcc_stdlib.h"

static bool ignore_info = false;
static void test_one(std::string s)
{
    int64_t t;

    t = zcc::iso8601_2004_time_from_time(s);
    if (!ignore_info)
    {
        zcc_info("only time: %zd", t);
    }

    t = zcc::iso8601_2004_time_from_date(s, true);
    if (!ignore_info)
    {
        zcc_info("date1: %zd", t);
    }

    t = zcc::iso8601_2004_time_from_date(s, false);
    if (!ignore_info)
    {
        zcc_info("date2: %zd", t);
    }
}

static void test_file(const std::string &filename)
{
    int64_t t;
    std::string content = zcc::file_get_contents_sample(filename);
    size_t i = 0;
    while (1)
    {
        size_t len = i++ % 32;
        if (content.size() < len)
        {
            test_one(content);
            break;
        }
        auto s = content.substr(content.size() - len, len);
        test_one(s);
        content.resize(content.size() - 1);
    }
}

int main(int argc, char **argv)
{
    zcc::main_argument::run(argc, argv);
    if (zcc::main_argument::var_parameter_argc < 1)
    {
        zcc_info("Usage: %s time [ --f ]", argv[0]);
        return 0;
    }
    std::string s = zcc::main_argument::var_parameter_argv[0];
    if (zcc::var_main_config.get_bool("f"))
    {
        ignore_info = true;
        test_file(s);
        zcc_info("test file done");
    }
    else
    {
        test_one(s);
    }

    return 0;
}
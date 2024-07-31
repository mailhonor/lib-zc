/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-11-25
 * ================================
 */

#include "zcc/zcc_redis.h"
#include "zcc/zcc_json.h"

static int64_t nval;
static std::string sval, sval2;
static std::list<std::string> lval;
static zcc::json jval;

static void _test_test(zcc::redis_client &rc, const char *cmd, int cmd_ret, int64_t line, int test_type)
{
    std::printf("\n%s\n%-8d", cmd, cmd_ret);
    if (cmd_ret < 0)
    {
        std::printf("%s  ### line:%zd", rc.get_info_msg().c_str(), line);
    }
    else if (test_type == 'r')
    {
        if (cmd_ret == 0)
        {
            std::printf("none/no/not");
        }
        else
        {
            std::printf("exists/yes/ok/count");
        }
    }
    else if (test_type == 'n')
    {
        std::printf("number: %zd", nval);
    }
    else if (test_type == 's')
    {
        if (cmd_ret == 0)
        {
            std::printf("none");
        }
        else
        {
            std::printf("string: %s", sval.c_str());
        }
    }
    else if (test_type == 'l')
    {
        if (cmd_ret == 0)
        {
            std::printf("none");
        }
        else
        {
            std::printf("list: ");
            for (auto it = lval.begin(); it != lval.end(); it++)
            {
                std::printf("%s, ", it->c_str());
            }
        }
    }
    else if (test_type == 'j')
    {
        if (cmd_ret == 0)
        {
            std::printf("none");
        }
        else
        {
            std::string out;
            jval.serialize(out);
            std::printf("json: %s", out.c_str());
        }
    }
    std::printf("\n");
    fflush(stdout);
    nval = -1000;
    sval.clear();
    sval2.clear();
    lval.clear();
    jval.reset();
}

#define _test_return(cmd) _test_test(rc, #cmd, cmd, __LINE__, 'r')
#define _test_number(cmd) _test_test(rc, #cmd, cmd, __LINE__, 'n')
#define _test_string(cmd) _test_test(rc, #cmd, cmd, __LINE__, 's')
#define _test___list(cmd) _test_test(rc, #cmd, cmd, __LINE__, 'l')
#define _test___json(cmd) _test_test(rc, #cmd, cmd, __LINE__, 'j')

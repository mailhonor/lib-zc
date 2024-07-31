/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-11-25
 * ================================
 */

#include "./lib.hpp"

int main(int argc, char **argv)
{
    std::printf("%s [ --cluster ][ -server 127.0.0.1:6379 ] [ -auth password ] redis_cmd arg1 arg2 ...\n", argv[0]);
    zcc::main_argument::run(argc, argv);
    zcc::redis_client rc;
    std::string server = zcc::var_main_config.get_string("server", "127.0.0.1:6379");
    rc.set_password(zcc::var_main_config.get_string("auth"));
    int c_ret;
    if (zcc::var_main_config.get_bool("cluster", false))
    {
        c_ret = rc.cluster_connect(server);
    }
    else
    {
        c_ret = rc.connect(server);
    }
    if (c_ret < 1)
    {
        if (c_ret == 0)
        {
            std::printf("auth failed\n");
        }
        else if (c_ret == -1)
        {
            std::printf("unknown error\n");
        }
        else
        {
            std::printf("open error\n");
        }
        zcc::exit(1);
    }
    if (!zcc::main_argument::var_parameters.empty())
    {
        std::list<std::string> query_tokens;
        for (const char *p : zcc::main_argument::var_parameters)
        {
            query_tokens.push_back(p);
        }
        _test___json(rc.exec_command(jval, query_tokens));
        if (zcc::var_main_config.get_bool("loop"))
        {
            while (1)
            {
                if (std::getchar() != '\n')
                {
                    break;
                }
                _test___json(rc.exec_command(jval, query_tokens));
            }
        }
        return 0;
    }

    _test_return(rc.exec_command({"SET", "", "ssssss"}));
    _test_return(rc.exec_command({"GET", "abc"}));
    _test_string(rc.exec_command(sval, {"GET", "abc"}));
    _test_string(rc.exec_command(sval, {"HGET", "xxx.com_u", "ac.tai"}));

    _test_return(rc.exec_command({"STRLEN", "abc"}));
    _test_number(rc.exec_command({"STRLEN", "abc"}));

    _test___list(rc.exec_command(lval, {"mget", "abc", "fasdfdsaf"}));

    _test___json(rc.exec_command(jval, {"MGET", "abc", "sss"}));
    _test___json(rc.exec_command(jval, {"SCAN", "0"}));
    _test___json(rc.exec_command(jval, {"EVAL", "return {1,2,{3,'Hello World!'}}", "0"}));
    _test___json(rc.exec_command(jval, {"fffSCAN", "0"}));
    for (int i = 0; i < 10000; i++)
    {
        rc.exec_command(sval, {"GET", std::to_string(i)});
    }

    std::printf("\n");

    return 0;
}

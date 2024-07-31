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
    std::printf("%s [ -server 127.0.0.1:6379 ] [ -channel news.a1,news.a2] -times 100\n", argv[0]);

    zcc::main_argument::run(argc, argv);

    zcc::redis_client rc;
    if (!rc.connect(zcc::var_main_config.get_cstring("server", "127.0.0.1:6379")))
    {
        std::printf("open error\n");
        zcc::exit(1);
    }

    const char *channels = zcc::var_main_config.get_cstring("channel", "news.a1,news.a2");
    std::vector<std::string> channel_vector = zcc::split(channels, ",;");

    for (auto it = channel_vector.begin(); it != channel_vector.end(); it++)
    {
        rc.subscribe(*it);
    }

    int times = zcc::var_main_config.get_int("times", 100);
    for (int i = 0; i < times; i++)
    {
        std::string type, channel, data;
        int ret = rc.fetch_channel_message(type, channel, data, 10);
        if (ret < 0)
        {
            std::printf("No.%03d found error\n", i);
            break;
        }
        if (ret == 0)
        {
            std::printf("No.%03d no message, timeout\n", i);
        }
        else
        {
            std::printf("No.%03d type: %s, channel: %s, data: %s\n", i, data.c_str(), channel.c_str(), data.c_str());
        }
    }

    printf("\n");
    return 0;
}

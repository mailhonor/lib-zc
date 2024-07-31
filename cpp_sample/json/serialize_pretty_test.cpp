/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2022-12-19
 * ================================
 */

#include "zcc/zcc_json.h"

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("USAGE: %s json_file\n", argv[0]);
        return 1;
    }
    std::string con;
    zcc::json js;

    if (!js.load_from_file(argv[1]))
    {
        printf("ERR load json from %s\n", argv[1]);
        return 1;
    }

    js.serialize(con, zcc::json_serialize_pretty);
    printf("JSON:\n%s\n", con.c_str());
    return 0;
}

/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2017-08-23
 * ================================
 */

#include "zcc_json.h"

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("USAGE: %s json_file\n", argv[0]);
        return 1;
    }
    std::string con;
    zcc::json js;

    if (!js.load_from_pathname(argv[1])) {
        printf("ERROR load json from %s(%m)\n", argv[1]);
        return 1;
    }

    js.serialize(con, 0);
    printf("JSON:\n%s\n", con.c_str());
    return 0;
}


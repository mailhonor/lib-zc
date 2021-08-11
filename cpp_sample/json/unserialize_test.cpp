/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2017-08-23
 * ================================
 */

#include "zcc_json.h"

static void test_loop(int argc, char **argv)
{
    int loops = atoi(argv[2]);
    if (loops < 1000) {
        loops = 1000;
    }
    std::string str;
    int ch;
    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        printf("ERR load json from %s(%m)\n", argv[1]);
        return;
    }
    while ((ch = fgetc(fp)) != EOF) {
        str.push_back(ch);
    }
    fclose(fp);
    for (int i = 0; i < loops; i++) {
        zcc::json js;
        js.unserialize(str.c_str(), (int)str.size());
    }
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("USAGE: %s json_file\n", argv[0]);
        return 1;
    }
    if (argc > 2) {
        test_loop(argc, argv);
        return 0;
    }
    std::string con;
    zcc::json js;

    if (!js.load_from_pathname(argv[1])) {
        printf("ERR load json from %s(%m)\n", argv[1]);
        return 1;
    }

    js.serialize(con, 0);
    printf("JSON:\n%s\n", con.c_str());
    return 0;
}


/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2016-12-01
 * ================================
 */

#include "zc.h"

static char *server = 0;
static void ___usage()
{
    printf("%s -server smtp_server:port\n", zvar_progname);
    exit(1);
}

static void  write_line_read_line(zcc::iostream &fp, std::string &tmpline, const char *query)
{
    fp.puts(query);
    fp.puts("\r\n");
    printf("C: %s\r\n", query);

    tmpline.clear();
    fp.gets(tmpline, 10240);
    printf("S: %s", tmpline.c_str());
}

int main(int argc, char **argv)
{
    zmain_argument_run(argc, argv);
    server = zconfig_get_str(zvar_default_config, "server", 0);
    if (zempty(server)) {
        ___usage();
    }

    printf("\n##############################\n\n");
    zcc::iostream fp;
    if (!fp.connect(server, 0)) {
        printf("ERR open %s error, (%m)\n", server);
        exit(1);
    }
    std::string tmpline;

    printf("connected\n");
    fp.gets(tmpline, 10240);
    printf("S: %s", tmpline.c_str());

    write_line_read_line(fp, tmpline, "helo goodtest");
    write_line_read_line(fp, tmpline, "mail from: <xxx@163.com>");
    write_line_read_line(fp, tmpline, "quit");

    return 0;
}

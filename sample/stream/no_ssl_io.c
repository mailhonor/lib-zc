/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
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

static void  write_line_read_line(zstream_t *fp, zbuf_t *tmpline, const char *query)
{
    zstream_puts(fp, query);
    zstream_puts(fp, "\r\n");
    printf("C: %s\r\n", query);

    zbuf_reset(tmpline);
    zstream_gets(fp, tmpline, 10240);
    printf("S: %s", zbuf_data(tmpline));
}

int main(int argc, char **argv)
{
    zmain_argument_run(argc, argv, 0);
    server = zconfig_get_str(zvar_default_config, "server", 0);
    if (zempty(server)) {
        ___usage();
    }

    int fd = -1;
    zstream_t *fp = 0;
    zbuf_t *tmpline = 0;

    printf("\n##############################\n\n");
    fd = zconnect(server, 10);
    if (fd < 0) {
        printf("ERR open %s error, (%m)\n", server);
        exit(1);
    }
    znonblocking(fd, 1);

    fp = zstream_open_fd(fd);
    printf("connected\n");
    tmpline = zbuf_create(0);
    zstream_gets(fp, tmpline, 10240);
    printf("S: %s", zbuf_data(tmpline));

    write_line_read_line(fp, tmpline, "helo goodtest");
    write_line_read_line(fp, tmpline, "mail from: <xxx@163.com>");
    write_line_read_line(fp, tmpline, "quit");

    if (fp) {
        zstream_close(fp, 1);
        fd = -1;
    }
    if (tmpline) {
        zbuf_free(tmpline);
    }

    return 0;
}

/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2016-12-01
 * ================================
 */

#include "zc.h"

static char *server = 0;
static int ssl_mode = 0;
static int tls_mode = 0;
static void ___usage()
{
    printf("%s -server smtp_server:port [--ssl ] [ --tls]\n", zvar_progname);
    exit(1);
}

static void  write_line_read_line(zcc::iostream &fp, std::string &tmpline, const char *query)
{
    fp.puts(query);
    fp.puts( "\r\n");
    printf("C: %s\r\n", query);

    while(1) {
        tmpline.clear();
        fp.gets(tmpline, 10240);
        const char *p = tmpline.c_str();
        printf("S: %s", p);
        if (tmpline.size()< 3) {
            break;
        }
        if (p[3] == ' ') {
            break;
        }
    }
}

int main(int argc, char **argv)
{
    zmain_argument_run(argc, argv, 0);
    server = zconfig_get_str(zvar_default_config, "server", 0);
    ssl_mode = zconfig_get_bool(zvar_default_config, "ssl", 0);
    tls_mode = zconfig_get_bool(zvar_default_config, "tls", 0);
    if (zempty(server)) {
        ___usage();
    }

    SSL_CTX *ssl_ctx = 0;
    int fd = -1;
    zcc::iostream fp;
    std::string tmpline;

    if (tls_mode || ssl_mode) {
        zopenssl_init();
        ssl_ctx = zopenssl_SSL_CTX_create_client();
    }

    printf("\n##############################\n\n");
    fd = zconnect(server, 10);
    if (fd < 0) {
        printf("ERR open %s error, (%m)\n", server);
        exit(1);
    }
    znonblocking(fd, 1);

    if (ssl_mode) {
        SSL *ssl = zopenssl_SSL_create(ssl_ctx, fd);
        if (zopenssl_timed_connect(ssl, 10, 10) < 0) {
            unsigned long ecode;
            char buf[1024];
            zopenssl_get_error(&ecode, buf, 1024);
            printf("ERR ssl initialization error:%s\n", buf);
            goto over;
        }
        fp.open_ssl(ssl);
    } else {
        fp.open_fd(fd);
    }
    printf("connected\n");
    fp.gets(tmpline, 10240);
    printf("S: %s", tmpline.c_str());

    write_line_read_line(fp, tmpline, "ehlo xxx1");
    if (tls_mode && !ssl_mode) {
        write_line_read_line(fp, tmpline, "STARTTLS");
        if (fp.tls_connect(ssl_ctx) < 0) {
            printf("ERR STARTTLS error (%m)\n");
            goto over;
        }
    }
    write_line_read_line(fp, tmpline, "ehlo xxx2");
    write_line_read_line(fp, tmpline, "mail from: <xxx@163.com>");
    write_line_read_line(fp, tmpline, "quit");

over:
    if (tls_mode || ssl_mode) {
        zopenssl_SSL_CTX_free(ssl_ctx);
        zopenssl_fini();
    }

    return 0;
}

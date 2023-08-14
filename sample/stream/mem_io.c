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
    zprintf("%s -server smtp_server:port [--ssl ] [ --tls]\n", zvar_progname);
    exit(1);
}

static void  write_line_read_line(zstream_t *fp, char *tmpline, const char *query)
{
    zstream_puts(fp, query);
    zstream_puts(fp, "\r\n");
    zprintf("C: %s\r\n", query);

    while(1) {
        tmpline[0] = 0;
        zstream_gets_to_mem(fp, tmpline, 10240);
        char *p = tmpline;
        zprintf("S: %s", p);
        if (strlen(tmpline)< 3) {
            break;
        }
        if (p[3] == ' ') {
            break;
        }
    }
}

int main(int argc, char **argv)
{
    zmain_argument_run(argc, argv);
    server = zconfig_get_str(zvar_default_config, "server", 0);
    ssl_mode = zconfig_get_bool(zvar_default_config, "ssl", 0);
    tls_mode = zconfig_get_bool(zvar_default_config, "tls", 0);
    if (zempty(server)) {
        ___usage();
    }

    SSL_CTX *ssl_ctx = 0;
    int fd = -1;
    zstream_t *fp = 0;
    char *tmpline = 0;

    if (tls_mode || ssl_mode) {
        zopenssl_init();
        ssl_ctx = zopenssl_SSL_CTX_create_client();
    }

    zprintf("\n##############################\n\n");
    fd = zconnect(server, 10);
    if (fd < 0) {
        zprintf("ERROR open %s error\n", server);
        exit(1);
    }
    znonblocking(fd, 1);

    if (ssl_mode) {
        SSL *ssl = zopenssl_SSL_create(ssl_ctx, fd);
        if (zopenssl_timed_connect(ssl, 10, 10) < 0) {
            unsigned long ecode;
            char buf[1024];
            zopenssl_get_error(&ecode, buf, 1024);
            zprintf("ERROR ssl initialization error:%s\n", buf);
            goto over;
        }
        fp = zstream_open_ssl(ssl);
    } else {
        fp = zstream_open_fd(fd);
    }
    zprintf("connected\n");
    tmpline = (char *)malloc(1024*1024);
    tmpline[0] = 0;
    zstream_gets_to_mem(fp, tmpline, 10240);
    zprintf("S: %s", tmpline);

    write_line_read_line(fp, tmpline, "ehlo xxx1");
    if (tls_mode && !ssl_mode) {
        write_line_read_line(fp, tmpline, "STARTTLS");
        if (zstream_tls_connect(fp, ssl_ctx) < 0) {
            zprintf("ERROR STARTTLS error\n");
            goto over;
        }
    }
    write_line_read_line(fp, tmpline, "ehlo xxx2");
    write_line_read_line(fp, tmpline, "mail from: <xxx@163.com>");
    write_line_read_line(fp, tmpline, "quit");

over:
    if (fp) {
        zstream_close(fp, 1);
        fd = -1;
    }
    zfree(tmpline);
    if (tls_mode || ssl_mode) {
        zopenssl_SSL_CTX_free(ssl_ctx);
        zopenssl_fini();
    }

    return 0;
}

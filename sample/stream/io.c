/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
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
    ssl_mode = zconfig_get_bool(zvar_default_config, "ssl", 0);
    tls_mode = zconfig_get_bool(zvar_default_config, "tls", 0);
    if (zempty(server)) {
        ___usage();
    }

    SSL_CTX *ssl_ctx = 0;
    int fd = -1;
    zstream_t *fp = 0;
    zbuf_t *tmpline = 0;

    if (tls_mode || ssl_mode) {
        zopenssl_init();
        ssl_ctx = zopenssl_SSL_CTX_create_client();
    }

    printf("\n##############################\n\n");
    fd = zconnect(server, 1, 10);
    if (fd < 0) {
        printf("ERR open %s error, (%m)\n", server);
        exit(1);
    }

    if (ssl_mode) {
        SSL *ssl = zopenssl_SSL_create(ssl_ctx, fd);
        if (zopenssl_timed_connect(ssl, 10 * 1000) < 0) {
            printf("ERR ssl initialization error (%m)\n");
            goto over;
        }
        fp = zstream_open_ssl(ssl);
    } else {
        fp = zstream_open_fd(fd);
    }
    printf("connected\n");
    tmpline = zbuf_create(0);
    zstream_gets(fp, tmpline, 10240);
    printf("S: %s", zbuf_data(tmpline));

    write_line_read_line(fp, tmpline, "helo goodtest");
    if (tls_mode && !ssl_mode) {
        write_line_read_line(fp, tmpline, "STARTTLS");
        if (zstream_tls_connect(fp, ssl_ctx) < 0) {
            printf("ERR STARTTLS error (%m)\n");
            goto over;
        }
    }
    write_line_read_line(fp, tmpline, "mail from: <xxx@163.com>");
    write_line_read_line(fp, tmpline, "quit");

over:
    if (fp) {
        zstream_close(fp, 1);
        fd = -1;
    }
    if (tmpline) {
        zbuf_free(tmpline);
    }
    if (tls_mode || ssl_mode) {
        zopenssl_SSL_CTX_free(ssl_ctx);
        zopenssl_fini();
    }

    return 0;
}

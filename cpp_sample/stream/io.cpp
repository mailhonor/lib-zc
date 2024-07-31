/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2016-12-01
 * ================================
 */

#include "zcc/zcc_stream.h"

static const char *server = 0;
static int ssl_mode = 0;
static int tls_mode = 0;
static int timeout = 10;
static void ___usage()
{
    std::printf("%s -server smtp_server:port [ -timeout 10 ]  [--ssl ] [ --tls]\n", zcc::progname);
    exit(1);
}

static void write_line_read_line(zcc::iostream &fp, std::string &tmpline, const char *query)
{
    fp.puts(query);
    fp.puts("\r\n");
    std::printf("C: %s\r\n", query);

    while (1)
    {
        tmpline.clear();
        fp.gets(tmpline, 10240);
        const char *p = tmpline.c_str();
        std::printf("S: %s", p);
        if (tmpline.size() < 3)
        {
            break;
        }
        if (p[3] == ' ')
        {
            break;
        }
    }
}

int main(int argc, char **argv)
{
    zcc::main_argument::run(argc, argv);
    server = zcc::var_main_config.get_cstring("server");
    ssl_mode = zcc::var_main_config.get_bool("ssl");
    tls_mode = zcc::var_main_config.get_bool("tls");
    timeout = zcc::var_main_config.get_int("timeout", 10);
    if (zcc::empty(server))
    {
        ___usage();
    }

    SSL_CTX *ssl_ctx = 0;
    int fd = -1;
    zcc::iostream fp;
    std::string tmpline;

    if (tls_mode || ssl_mode)
    {
        zcc::openssl::env_init();
        ssl_ctx = zcc::openssl::SSL_CTX_create_client();
    }

    std::printf("\n##############################\n\n");
    fd = zcc::netpath_connect(server, timeout);
    if (fd < 0)
    {
        std::printf("ERROR open %s error\n", server);
        exit(1);
    }
    zcc::nonblocking(fd, true);

    if (ssl_mode)
    {
        SSL *ssl = zcc::openssl::SSL_create(ssl_ctx, fd);
        if (zcc::openssl::timed_connect(ssl, timeout) < 0)
        {
            unsigned long ecode;
            char buf[1024];
            zcc::openssl::get_error(&ecode, buf, 1024);
            std::printf("ERROR ssl initialization error:%s\n", buf);
            goto over;
        }
        fp.open_ssl(ssl);
    }
    else
    {
        fp.open_socket(fd);
    }
    std::printf("connected\n");
    fp.set_timeout(timeout);
    fp.gets(tmpline, 10240);
    std::printf("S: %s", tmpline.c_str());

    write_line_read_line(fp, tmpline, "ehlo xxx1");
    if (tls_mode && !ssl_mode)
    {
        write_line_read_line(fp, tmpline, "STARTTLS");
        if (fp.tls_connect(ssl_ctx) < 0)
        {
            std::printf("ERROR STARTTLS error\n");
            goto over;
        }
    }
    write_line_read_line(fp, tmpline, "ehlo xxx2");
    write_line_read_line(fp, tmpline, "mail from: <xxx@163.com>");
    std::printf("is_closed: %d\n", fp.is_closed());
    write_line_read_line(fp, tmpline, "quit");
    std::printf("is_closed: %d\n", fp.is_closed());
    fp.close();
    std::printf("is_closed: %d\n", fp.is_closed());

over:
    if (tls_mode || ssl_mode)
    {
        zcc::openssl::SSL_CTX_free(ssl_ctx);
        zcc::openssl::env_fini();
    }

    return 0;
}

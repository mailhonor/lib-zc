/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2017-07-18
 * ================================
 */

#include "zc.h"
#include <errno.h>
#include <poll.h>
#include <openssl/ssl.h>

static char *proxy_address = 0;
static int proxy_is_ssl = 0;

static char *dest_address = 0;
static int dest_is_ssl = 0;

static char *ssl_key = 0;
static char *ssl_cert = 0;

static SSL_CTX * ssl_proxy_ctx = 0;
static SSL_CTX * ssl_dest_ctx = 0;

static void ___usage()
{

    printf("USAGE: %s -proxy host:port -dest host:port [ -times 9999991 ]\n", zvar_progname);
    printf("USAGE: %s -proxy host:port -ssl-dest host:port\n", zvar_progname);
    printf("USAGE: %s -ssl-proxy host:port -dest host:port -ssl-cert filename -ssl-key filename\n", zvar_progname);
    exit(1);
}

int times = 999999;
static int ___times = 0;
static int ___stop = 0;
static void after_close(void *ctx)
{
    ___times++;
    fprintf(stderr, "times: %d\n", ___times);
    if (___times == times) {
        ___stop = 1;
        zcoroutine_base_stop_notify(0);
        fprintf(stderr, "... stop\n");
    }
}

static void parameters_do(int argc, char **argv)
{
    zmain_argument_run(argc, argv, 0);

    proxy_address = zconfig_get_str(zvar_default_config, "proxy", 0);
    if (zempty(proxy_address)) {
        proxy_address = zconfig_get_str(zvar_default_config, "ssl-proxy", 0);
        proxy_is_ssl = 1;
    }

    dest_address = zconfig_get_str(zvar_default_config, "dest", 0);
    if (zempty(dest_address)) {
        dest_address = zconfig_get_str(zvar_default_config, "ssl-dest", 0);
        dest_is_ssl = 1;
    }

    ssl_key = zconfig_get_str(zvar_default_config, "ssl-key", 0);
    ssl_cert = zconfig_get_str(zvar_default_config, "ssl-cert", 0);
    times = zconfig_get_int(zvar_default_config, "times", 9999991);

    if (proxy_is_ssl && dest_is_ssl) {
        proxy_is_ssl = 0;
        dest_is_ssl = 0;
    }
    if (zempty(proxy_address)) {
        printf("ERR proxy'address is null\n");
        ___usage();
    }
    if (zempty(dest_address)) {
        printf("ERR dest'address is null\n");
        ___usage();
    }
}

static void ssl_do()
{
    zopenssl_init();

    ssl_dest_ctx = zopenssl_SSL_CTX_create_client();

    if (proxy_is_ssl) {
        if (zempty(ssl_key) || zempty(ssl_cert)) {
            printf("ERR ssl-proxy mode, need -ssl-key, -ssl-cert\n");
            ___usage();
        }
        ssl_proxy_ctx = zopenssl_SSL_CTX_create_server(ssl_cert, ssl_key);
        if (!ssl_proxy_ctx) {
            printf("ERR can load ssl err: %s, %s\n", ssl_cert, ssl_key);
            exit(1);
        }
    }
}

static void ssl_fini()
{
    zopenssl_SSL_CTX_free(ssl_proxy_ctx);
    zopenssl_SSL_CTX_free(ssl_dest_ctx);
    zopenssl_fini();
}

void * do_after_accept(void *arg)
{
    int proxy_fd = (int)(long)(arg);
    SSL * proxy_ssl = 0;
    int dest_fd = -1;
    SSL *dest_ssl = 0;
    int err = 0;

    do {
        if (proxy_is_ssl) {
            proxy_ssl = zopenssl_SSL_create(ssl_proxy_ctx, proxy_fd);
            if (zopenssl_timed_accept(proxy_ssl, 10, 10) < 1) {
                printf("ERR openssl_accept error\n");
                err = 1;
                break;
            }
        }
        dest_fd = zconnect(dest_address, 10);
        if (dest_fd == -1) {
            printf("ERR can not connect %s\n", dest_address);
            err = 1;
            break;
        }
        znonblocking(dest_fd, 1);
        if (dest_is_ssl) {
            dest_ssl = zopenssl_SSL_create(ssl_dest_ctx, dest_fd);
            if (zopenssl_timed_connect(dest_ssl, 10, 10) < 1) {
                printf("ERR openssl_connect error\n");
                err = 1;
                break;
            }
        }
    } while(0);

    if (err) {
        if (proxy_ssl) {
            zopenssl_SSL_free(proxy_ssl);
        }
        if (proxy_fd != -1) {
            close(proxy_fd);
        }
        if (dest_ssl) {
            zopenssl_SSL_free(dest_ssl);
        }
        if (dest_fd != -1) {
            close(dest_fd);
        }
        return 0;
    }
    zcoroutine_go_iopipe(proxy_fd, proxy_ssl, dest_fd, dest_ssl, 0, after_close, 0);
    return 0;
}


static void *do_listen(void * arg)
{
    int sock_type;
    int listen_fd = zlisten(proxy_address, &sock_type, 10);
    if (listen_fd < 0) {
        printf("ERR: can not open %s (%m), proxy_address\n", proxy_address);
        exit(1);
    }
    while(1) {
        if (___stop) {
            break;
        }
        if (ztimed_read_wait(listen_fd, 100) < 0) {
            printf("accept error (%m)");
            exit(1);
        }
        int fd = zaccept(listen_fd, sock_type);
        if (fd < 0) {
            continue;
        }
        znonblocking(fd, 1);
        zcoroutine_go(do_after_accept, (void *)(long)(fd), 0);
    }

    close(listen_fd);
    return arg;
}

int main(int argc, char **argv)
{
    parameters_do(argc, argv);

    ssl_do();

    zcoroutine_base_init();

    zcoroutine_go(do_listen, 0, 0);

    zcoroutine_base_run(0);
    zcoroutine_base_fini();

    ssl_fini();

    return 0;
}

/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2015-11-08
 * ================================
 */

#include <openssl/ssl.h>
#include "zc.h"
#include <errno.h>
#include <signal.h>

static zaio_base_t *aiobase;
static zaio_t *listen_aio;

static int client_count = 0;
static char *proxy_address = 0;
static int proxy_ssl = 0;

static char *server_address = 0;
static int server_ssl = 0;

static char *ssl_key = 0;
static char *ssl_cert = 0;

static SSL_CTX * ssl_proxy_ctx = 0;
static SSL_CTX * ssl_server_ctx = 0;

typedef struct fd_to_fd_linker fd_to_fd_linker;
struct fd_to_fd_linker
{
    zaio_t *proxy;
    zaio_t *server;
};

static void ___usage(char *parameter)
{

    printf("USAGE: %s -proxy host:port -dest host:port\n", zvar_progname);
    printf("USAGE: %s -proxy host:port -ssl-dest host:port\n", zvar_progname);
    printf("USAGE: %s -ssl-proxy host:port -server host:port -ssl-cert filename -ssl-key filename\n", zvar_progname);
    exit(1);
}

static char *get_current_time(char *tbuf)
{
    time_t t = time(0);
    ctime_r(&t, tbuf);
    char *p = strchr(tbuf, '\n');
    if (p) {
        *p = 0;
    }
    return tbuf;
}

static void timer_cb(zaio_t *tm)
{
    char tbuf[128];
    zinfo("%s client count: %d", get_current_time(tbuf), client_count);
    if (zvar_sigint_flag == 1) {
        zaio_free(tm, 1);
        zaio_free(listen_aio, 1);
        zaio_base_stop_notify(aiobase);
        zinfo("signal SIGINT, then sleep 3, then EXIT");
        sleep(3);
    } else {
        zaio_sleep(tm, timer_cb, 1);
    }
}

static void after_close(void *ctx)
{
    client_count--;
}

static void parameters_do(int argc, char **argv)
{
    zmain_argument_run(argc, argv, 0);

    proxy_address = zconfig_get_str(zvar_default_config, "proxy", 0);
    if (zempty(proxy_address)) {
        proxy_address = zconfig_get_str(zvar_default_config, "ssl-proxy", 0);
        proxy_ssl = 1;
    }

    server_address = zconfig_get_str(zvar_default_config, "dest", 0);
    if (zempty(server_address)) {
        server_address = zconfig_get_str(zvar_default_config, "ssl-dest", 0);
        server_ssl = 1;
    }

    ssl_key = zconfig_get_str(zvar_default_config, "ssl-key", 0);
    ssl_cert = zconfig_get_str(zvar_default_config, "ssl-cert", 0);

    if (proxy_ssl && server_ssl) {
        proxy_ssl = 0;
        server_ssl = 0;
    }
    if (zempty(proxy_address)) {
        printf("ERR: proxy'address is null\n");
        ___usage(0);
    }
    if (zempty(server_address)) {
        printf("ERR: server'address is null\n");
        ___usage(0);
    }
}

static void ssl_do()
{
    zopenssl_init();

    ssl_server_ctx = zopenssl_SSL_CTX_create_client();

    if (proxy_ssl) {
        if (zempty(ssl_key) || zempty(ssl_cert)) {
            printf("ERR: ssl-proxy mode, need --ssl-key, --ssl-cert\n");
            ___usage(0);
        }
        ssl_proxy_ctx = zopenssl_SSL_CTX_create_server(ssl_cert, ssl_key);
        if (!ssl_proxy_ctx) {
            printf("ERR: can load ssl err: %s, %s\n", ssl_cert, ssl_key);
            exit(1);
        }
    }
}

static void after_connect(zaio_t *aio)
{
    fd_to_fd_linker *jctx = (fd_to_fd_linker *)zaio_get_context(aio);
    if (zaio_get_result(aio) < 0) {
        zaio_free(jctx->proxy, 1);
        zaio_free(jctx->server, 1);
        zfree(jctx);
        return;
    }

    client_count++;
    zaio_iopipe_enter(jctx->proxy, jctx->server, aiobase, after_close, 0);
    zfree(jctx);
}

static void after_accept(zaio_t *aio)
{
    fd_to_fd_linker *jctx = (fd_to_fd_linker *)zaio_get_context(aio);
    if (zaio_get_result(aio) < 0) {
        zaio_free(jctx->proxy, 1);
        zfree(jctx);
        return;
    }

    int server_fd = zconnect(server_address, 10);
    if (server_fd < 0) {
        zaio_free(jctx->proxy, 1);
        zfree(jctx);
        return;
    }
    znonblocking(server_fd, 1);

    jctx->server = zaio_create(server_fd, aiobase);
    zaio_set_context(jctx->server, jctx);
    if (server_ssl) {
        zaio_tls_connect(jctx->server, ssl_server_ctx, after_connect);
        return;
    }
    after_connect(jctx->server);
}

static void start_one(zaio_t *ev)
{
    int fd = zaio_get_fd(ev);
    int proxy_fd = zinet_accept(fd);
    if (proxy_fd < 0) {
        return;
    }
    znonblocking(proxy_fd, 1);

    fd_to_fd_linker *jctx = (fd_to_fd_linker *)zcalloc(1, sizeof(fd_to_fd_linker));

    jctx->proxy = zaio_create(proxy_fd, aiobase);
    zaio_set_context(jctx->proxy, jctx);
    if (proxy_ssl) {
        zaio_tls_accept(jctx->proxy, ssl_proxy_ctx, after_accept);
        return;
    }
    after_accept(jctx->proxy);
}

int main(int argc, char **argv)
{
    parameters_do(argc, argv);
    ssl_do();
    aiobase = zaio_base_create();

    do {
        zaio_t *tm = zaio_create(-1, aiobase);
        zaio_sleep(tm, timer_cb, 1);
    } while(0);

    do {
        int fd_type;
        int fd = zlisten(proxy_address, &fd_type, 5);
        if (fd < 0) {
            printf("ERR: can not open %s (%m)\n", proxy_address);
            exit(1);
        }
        znonblocking(fd, 1);
        listen_aio = zaio_create(fd, aiobase);
        zaio_readable(listen_aio, start_one);
    } while(0);

    zaio_base_run(aiobase);
    zaio_base_free(aiobase);
    return 0;
}
/* Local variables:
* End:
* vim600: fdm=marker
*/

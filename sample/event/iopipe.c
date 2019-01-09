/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-02-07
 * ================================
 */

#include "zc.h"
#include <pthread.h>
#include <openssl/ssl.h>

static zevent_base_t *main_evbase;
static char *proxy_address = 0;
static int proxy_ssl = 0;

static char *dest_address = 0;
static int dest_ssl = 0;

static char *ssl_key = 0;
static char *ssl_cert = 0;

static SSL_CTX * ssl_proxy_ctx = 0;
static SSL_CTX * ssl_dest_ctx = 0;

static ziopipe_base_t *iop;

typedef struct fd_to_fd_linker fd_to_fd_linker;
struct fd_to_fd_linker
{
    zaio_t *proxy;
    zaio_t *dest;
};

static void ___usage(char *parameter)
{

    printf("USAGE: %s -proxy host:port -dest host:port\n", zvar_progname);
    printf("USAGE: %s -proxy host:port -ssl-dest host:port\n", zvar_progname);
    printf("USAGE: %s -ssl-proxy host:port -dest host:port --ssl-cert filename -ssl-key filename\n", zvar_progname);
    exit(1);
}

static void parameters_do(int argc, char **argv)
{
    zmain_parameter_run(argc, argv);

    proxy_address = zconfig_get_str(zvar_default_config, "proxy", 0);
    if (zempty(proxy_address)) {
        proxy_address = zconfig_get_str(zvar_default_config, "ssl-proxy", 0);
        proxy_ssl = 1;
    }

    dest_address = zconfig_get_str(zvar_default_config, "dest", 0);
    if (zempty(dest_address)) {
        dest_address = zconfig_get_str(zvar_default_config, "ssl-dest", 0);
        dest_ssl = 1;
    }

    ssl_key = zconfig_get_str(zvar_default_config, "ssl-key", 0);
    ssl_cert = zconfig_get_str(zvar_default_config, "ssl-cert", 0);

    if (proxy_ssl && dest_ssl) {
        proxy_ssl = 0;
        dest_ssl = 0;
    }
    if (zempty(proxy_address)) {
        printf("ERR: proxy'address is null\n");
        ___usage(0);
    }
    if (zempty(dest_address)) {
        printf("ERR: dest'address is null\n");
        ___usage(0);
    }
}

static void ssl_do()
{
    zopenssl_init();

    ssl_proxy_ctx = zopenssl_SSL_CTX_create_server();
    ssl_dest_ctx = zopenssl_SSL_CTX_create_client();

    if (proxy_ssl) {
        if (zempty(ssl_key) || zempty(ssl_cert)) {
            printf("ERR: ssl-proxy mode, need --ssl-key, --ssl-cert\n");
            ___usage(0);
        }
        if (zopenssl_SSL_CTX_set_cert(ssl_proxy_ctx, ssl_cert, ssl_key) < 0) {
            printf("ERR: can load ssl err: %s, %s\n", ssl_cert, ssl_key);
            exit(1);
        }
    }
}

static void after_connect(zaio_t *aio)
{
    fd_to_fd_linker *jctx = (fd_to_fd_linker *)zaio_get_context(aio);
    int proxy_fd = zaio_get_fd(jctx->proxy);
    int dest_fd = zaio_get_fd(jctx->dest);
    if (zaio_get_result(aio) < 0) {
        zaio_free(jctx->proxy, 1);
        zaio_free(jctx->dest, 1);
        zfree(jctx);
        return;
    }

    SSL *proxy_SSL = zaio_get_ssl(jctx->proxy);
    SSL *dest_SSL = zaio_get_ssl(jctx->dest);
    ziopipe_enter(iop, proxy_fd, proxy_SSL, dest_fd, dest_SSL, 0, 0);
    zaio_free(jctx->proxy, 0);
    zaio_free(jctx->dest, 0);
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

    int dest_fd = zconnect(dest_address, 1, 10);
    if (dest_fd < 0) {
        zaio_free(jctx->proxy, 1);
        zfree(jctx);
        return;
    }

    jctx->dest = zaio_create(dest_fd, main_evbase);
    zaio_set_context(jctx->dest, jctx);
    if (dest_ssl) {
        zaio_tls_connect(jctx->dest, ssl_dest_ctx, after_connect, 10);
        return;
    }
    after_connect(jctx->dest);
}

static void start_one(zeio_t *ev)
{
    int fd = zeio_get_fd(ev);
    int proxy_fd = zinet_accept(fd);
    if (proxy_fd < 0) {
        return;
    }
    znonblocking(proxy_fd, 1);

    fd_to_fd_linker *jctx = (fd_to_fd_linker *)zcalloc(1, sizeof(fd_to_fd_linker));

    jctx->proxy = zaio_create(proxy_fd, main_evbase);
    zaio_set_context(jctx->proxy, jctx);
    if (proxy_ssl) {
        zaio_tls_accept(jctx->proxy, ssl_proxy_ctx, after_accept, 10);
        return;
    }
    after_accept(jctx->proxy);
}

static void *before_accept_incoming(void *arg)
{
    int fd_type;
    int fd = zlisten(proxy_address, &fd_type, 5, 1);
    if (fd < 0) {
        printf("ERR: can not open %s (%m)\n", proxy_address);
        exit(1);
    }

    zeio_t *eio = zeio_create(fd, main_evbase);
    zeio_enable_read(eio, start_one);

    while(zevent_base_dispatch(main_evbase)) {
    }

    return arg;
}

void * iop_run(void *arg)
{
    iop = ziopipe_base_create();
    while(1) {
        ziopipe_base_run(iop);
    }
    return arg;
}

int main(int argc, char **argv)
{
    parameters_do(argc, argv);
    ssl_do();
    main_evbase = zevent_base_create();
    pthread_t pth;

#if 0
    pthread_create(&pth, 0, before_accept_incoming, 0);
    iop_run();
#else
    pthread_create(&pth, 0, iop_run, 0);
    before_accept_incoming(0);
#endif

    return 0;
}

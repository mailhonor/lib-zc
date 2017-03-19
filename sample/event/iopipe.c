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

static zevbase_t *main_evbase;
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
    zaio_t proxy;
    zaio_t dest;
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
    ZPARAMETER_BEGIN() {
        if (optval == 0) {
            ___usage(0);
        }
        if (!strcmp(optname, "-proxy")) {
            proxy_address = optval;
            opti += 2;
            continue;
        }
        if (!strcmp(optname, "-ssl-proxy")) {
            proxy_address = optval;
            proxy_ssl = 1;
            opti += 2;
            continue;
        }
        if (!strcmp(optname, "-dest")) {
            dest_address = optval;
            opti += 2;
            continue;
        }
        if (!strcmp(optname, "-ssl-dest")) {
            dest_address = optval;
            dest_ssl = 1;
            opti += 2;
            continue;
        }
        if (!strcmp(optname, "-ssl-key")) {
            ssl_key = optval;
            opti += 2;
            continue;
        }
        if (!strcmp(optname, "-ssl-cert")) {
            ssl_cert = optval;
            opti += 2;
            continue;
        }
    }
    ZPARAMETER_END;

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

    ssl_proxy_ctx = zopenssl_create_SSL_CTX_server();
    ssl_dest_ctx = zopenssl_create_SSL_CTX_client();

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
    int ret = zaio_get_ret(aio);
    fd_to_fd_linker *jctx = (fd_to_fd_linker *)zaio_get_context(aio);
    int proxy_fd = zaio_get_fd(&(jctx->proxy));
    int dest_fd = zaio_get_fd(&(jctx->dest));
    if (ret < 0) {
        zaio_fini(&(jctx->proxy));
        zaio_fini(&(jctx->dest));
        close(proxy_fd);
        close(dest_fd);
        zfree(jctx);
        return;
    }

    SSL *proxy_SSL = zaio_detach_SSL(&(jctx->proxy));
    SSL *dest_SSL = zaio_detach_SSL(&(jctx->dest));
    ziopipe_enter(iop, proxy_fd, proxy_SSL, dest_fd, dest_SSL, 0, 0);
    zaio_fini(&(jctx->proxy));
    zaio_fini(&(jctx->dest));
    zfree(jctx);
}

static void after_accept(zaio_t *aio)
{
    int proxy_fd = zaio_get_fd(aio);
    int ret = zaio_get_fd(aio);
    fd_to_fd_linker *jctx = (fd_to_fd_linker *)zaio_get_context(aio);
    if (ret < 0) {
        zaio_fini(&(jctx->proxy));
        close(proxy_fd);
        zfree(jctx);
        return;
    }

    int dest_fd = zconnect(dest_address, 0);
    if (dest_fd < 0) {
        zaio_fini(&(jctx->proxy));
        close(proxy_fd);
        zfree(jctx);
        return;
    }

    zaio_init(&(jctx->dest), main_evbase, dest_fd);
    zaio_set_context(&(jctx->dest), jctx);
    if (dest_ssl) {
        zaio_ssl_init_client(&(jctx->dest), ssl_dest_ctx, after_connect, 10 * 1000);
        return;
    }
    after_connect(&(jctx->dest));
}

static void start_one(zev_t *ev)
{
    int fd = zev_get_fd(ev);
    int proxy_fd = zinet_accept(fd);
    if (proxy_fd < 0) {
        return;
    }
    znonblocking(proxy_fd, 1);

    fd_to_fd_linker *jctx = (fd_to_fd_linker *)zcalloc(1, sizeof(fd_to_fd_linker));

    zaio_init(&(jctx->proxy), main_evbase, proxy_fd);
    zaio_set_context(&(jctx->proxy), jctx);
    if (proxy_ssl) {
        zaio_ssl_init_server(&(jctx->proxy), ssl_proxy_ctx, after_accept, 10 * 1000);
        return;
    }
    after_accept(&(jctx->proxy));
}

static void *before_accept_incoming(void *arg)
{
    int fd_type;
    int fd = zlisten(proxy_address, 5, &fd_type);
    if (fd < 0) {
        printf("ERR: can not open %s (%m)\n", proxy_address);
        exit(1);
    }

    zev_t *eio = zev_create();
    zev_init(eio, main_evbase, fd);
    zev_read(eio, start_one);

    while(1) {
        zevbase_dispatch(main_evbase, 1000);
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
    zvar_progname = argv[0];

    parameters_do(argc, argv);

    ssl_do();

    main_evbase = zevbase_create();

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

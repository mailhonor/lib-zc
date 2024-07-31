/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2015-11-08
 * ================================
 */

#include "zcc/zcc_aio.h"
#include "zcc/zcc_openssl.h"

static zcc::aio_base *aiobase;
static zcc::aio *listen_aio;

static int client_count = 0;
static const char *proxy_address = 0;
static bool proxy_ssl = false;

static const char *server_address = 0;
static bool server_ssl = false;

static const char *ssl_key = 0;
static const char *ssl_cert = 0;

static SSL_CTX *ssl_proxy_ctx = 0;
static SSL_CTX *ssl_server_ctx = 0;

struct fd_to_fd_linker
{
    zcc::aio *proxy{nullptr};
    zcc::aio *server{nullptr};
};

static void ___usage()
{

    zcc_info("USAGE: %s -proxy host:port -dest host:port", zcc::progname);
    zcc_info("USAGE: %s -proxy host:port -ssl-dest host:port", zcc::progname);
    zcc_info("USAGE: %s -ssl-proxy host:port -server host:port -ssl-cert filename -ssl-key filename", zcc::progname);
    zcc::exit(1);
}

static void timer_cb(zcc::aio_timer *tm)
{
    char tbuf[128];
    zcc_info("%s client count: %d", zcc::rfc822_time().c_str(), client_count);
    if (zcc::var_sigint_flag)
    {
        tm->get_aio_base()->stop_notify();
        delete tm;
        zcc_info("signal SIGINT, then sleep 3, then EXIT");
    }
    else
    {
        tm->after(std::bind(timer_cb, tm), 1);
    }
}

static void after_close()
{
    client_count--;
}

static void parameters_do(int argc, char **argv)
{
    zcc::main_argument::run(argc, argv);

    proxy_address = zcc::var_main_config.get_cstring("proxy");
    if (zcc::empty(proxy_address))
    {
        proxy_address = zcc::var_main_config.get_cstring("ssl-proxy");
        proxy_ssl = true;
    }

    server_address = zcc::var_main_config.get_cstring("dest");
    if (zcc::empty(server_address))
    {
        server_address = zcc::var_main_config.get_cstring("ssl-dest");
        server_ssl = true;
    }

    ssl_key = zcc::var_main_config.get_cstring("ssl-key");
    ssl_cert = zcc::var_main_config.get_cstring("ssl-cert");

    if (proxy_ssl && server_ssl)
    {
        proxy_ssl = true;
        server_ssl = true;
    }
    if (zcc::empty(proxy_address))
    {
        zcc_error("proxy'address is null");
        ___usage();
    }
    if (zcc::empty(server_address))
    {
        zcc_error("server'address is null");
        ___usage();
    }
}

static void ssl_do()
{
    zcc::openssl::env_init();

    ssl_server_ctx = zcc::openssl::SSL_CTX_create_client();

    if (proxy_ssl)
    {
        if (zcc::empty(ssl_key) || zcc::empty(ssl_cert))
        {
            zcc_error_and_exit("ssl-proxy mode, need --ssl-key, --ssl-cert");
        }
        ssl_proxy_ctx = zcc::openssl::SSL_CTX_create_server(ssl_cert, ssl_key);
        if (!ssl_proxy_ctx)
        {
            zcc_error_and_exit("can load ssl err: %s, %s", ssl_cert, ssl_key);
        }
    }
}

static void after_connect(zcc::aio *ev, fd_to_fd_linker *jctx)
{
    if (ev->get_result() < 0)
    {
        if (jctx->proxy)
        {
            delete jctx->proxy;
        }
        if (jctx->server)
        {
            delete jctx->server;
        }
        delete jctx;
        return;
    }

    client_count++;
    zcc::aio_iopipe_enter(jctx->proxy, jctx->server, aiobase, after_close);
    delete jctx;
}

static void after_accept(zcc::aio *ev, fd_to_fd_linker *jctx)
{
    if (ev->get_result() < 0)
    {
        if (jctx->proxy)
        {
            delete jctx->proxy;
        }
        if (jctx->server)
        {
            delete jctx->server;
        }
        delete jctx;
        return;
    }

    int server_fd = zcc::netpath_connect(server_address, 10);
    if (server_fd < 0)
    {
        delete jctx->proxy;
        delete jctx;
        return;
    }
    zcc::nonblocking(server_fd);

    jctx->server = new zcc::aio(server_fd, aiobase);
    if (server_ssl)
    {
        jctx->server->tls_connect(ssl_server_ctx, std::bind(after_connect, jctx->server, jctx));
        return;
    }

    after_connect(jctx->server, jctx);
}

static void start_one(zcc::aio *ev)
{
    int fd = ev->get_fd();
    int proxy_fd = zcc::inet_accept(fd);
    if (proxy_fd < 0)
    {
        return;
    }
    zcc::nonblocking(proxy_fd);

    fd_to_fd_linker *jctx = new fd_to_fd_linker();

    jctx->proxy = new zcc::aio(proxy_fd, aiobase);
    if (proxy_ssl)
    {
        jctx->proxy->tls_accept(ssl_proxy_ctx, std::bind(after_accept, jctx->proxy, jctx));
        return;
    }
    after_accept(jctx->proxy, jctx);
}

int main(int argc, char **argv)
{
    parameters_do(argc, argv);
    ssl_do();

    aiobase = new zcc::aio_base();

    do
    {
        zcc::aio_timer *tm = new zcc::aio_timer(aiobase);
        tm->sleep(std::bind(timer_cb, tm), 1);
    } while (0);

    do
    {
        int fd_type;
        int fd = zcc::netpath_listen(proxy_address, 5, &fd_type);
        if (fd < 0)
        {
            zcc_error_and_exit("can not open %s (%m)", proxy_address);
        }
        zcc::nonblocking(fd);
        listen_aio = new zcc::aio(fd, aiobase);
        listen_aio->readable(std::bind(start_one, listen_aio));
    } while (0);

    aiobase->run();
    delete aiobase;

    return 0;
}

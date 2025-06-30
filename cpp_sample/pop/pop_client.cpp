/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2024-02-03
 * ================================
 */

#include "zcc/zcc_pop.h"
#include "zcc/zcc_openssl.h"

static SSL_CTX *ssl_ctx = 0;
static const char *server = 0;
static int ssl_mode = 0;
static int tls_mode = 0;
static const char *user = "";
static const char *pass = "";

static void ___usage()
{
    zcc_fatal("%s -server pop3_server:port -user xxx@a.com -pass 123456 [--ssl ] [ --tls]", zcc::progname);
}

static void run_test()
{
    zcc::pop_client pc;
    pc.set_debug_mode();
    pc.set_verbose_mode();
    pc.set_debug_protocol_fn([](int rw, const void *mem, int len)
                             {
                                 std::string s;
                                 s.append((const char *)mem, len);
                                 zcc_info("debug_protocol_fn, %c: %s", rw, s.c_str());
                                 //
                             });
    pc.set_timeout(10);
    pc.set_ssl_tls(ssl_ctx, ssl_mode, tls_mode);

    if (pc.connect(server) < 1)
    {
        return;
    }

    if (pc.do_auth(user, pass) < 1)
    {
        return;
    }

    std::vector<std::string> capa_vector;
    if (pc.cmd_capa(capa_vector) < 0)
    {
        return;
    }

    if (pc.cmd_capa(capa_vector) < 0)
    {
        return;
    }

    std::map<std::string, int> uidl_result;
    if (pc.cmd_uidl(uidl_result) < 1)
    {
        return;
    }

    if (uidl_result.size())
    {
        std::string data;
        if (pc.cmd_retr(1, data) < 1)
        {
            return;
        }
        zcc_info("eml:\n%s\n", data.c_str());
    }
    pc.disconnect();
}

int main(int argc, char **argv)
{
    zcc::main_argument::run(argc, argv);
    server = zcc::var_main_config.get_cstring("server", 0);
    ssl_mode = zcc::var_main_config.get_bool("ssl", false);
    tls_mode = zcc::var_main_config.get_bool("tls", false);
    user = zcc::var_main_config.get_cstring("user", "");
    pass = zcc::var_main_config.get_cstring("pass", "");
    if (zcc::empty(server) || zcc::empty(user) || zcc::empty(pass))
    {
        ___usage();
    }

    if (tls_mode || ssl_mode)
    {
        zcc::openssl::env_init();
        ssl_ctx = zcc::openssl::SSL_CTX_create_client();
    }

    run_test();

    if (tls_mode || ssl_mode)
    {
        zcc::openssl::SSL_CTX_free(ssl_ctx);
        zcc::openssl::env_fini();
    }

    return 0;
}

/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2024-02-03
 * ================================
 */

#include "zc.h"

static SSL_CTX *ssl_ctx = 0;
static char *server = 0;
static int ssl_mode = 0;
static int tls_mode = 0;
const char *user = "";
const char *pass = "";

static void ___usage()
{
    zprintf("%s -server pop3_server:port -user xxx@a.com -pass 123456 [--ssl ] [ --tls]\n", zvar_progname);
    exit(1);
}

static void debug_protocol_fn(int rw, const void *mem, int len)
{
    std::string s;
    s.append((const char *)mem, len);
    zprintf("debug_protocol_fn, %c: %s\n", rw, s.c_str());
}

static void run_test()
{
    zcc::pop_client pc;
    pc.set_debug_mode();
    pc.set_verbose_mode();
    pc.set_debug_protocol_fn(debug_protocol_fn);
    pc.set_timeout(10);
    pc.set_ssl_tls(ssl_ctx, ssl_mode, tls_mode);

    if (pc.open(server) < 1)
    {
        return;
    }

    if (pc.auth_auto(user, pass) < 1)
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
        zinfo("eml:\n%s\n", data.c_str());
    }
    pc.close();
}

int main(int argc, char **argv)
{
    zmain_argument_run(argc, argv);
    server = zconfig_get_str(zvar_default_config, "server", 0);
    ssl_mode = zconfig_get_bool(zvar_default_config, "ssl", 0);
    tls_mode = zconfig_get_bool(zvar_default_config, "tls", 0);
    user = zconfig_get_str(zvar_default_config, "user", "");
    pass = zconfig_get_str(zvar_default_config, "pass", "");
    if (zempty(server) || zempty(user) || zempty(pass))
    {
        ___usage();
    }

    if (tls_mode || ssl_mode)
    {
        zopenssl_init();
        ssl_ctx = zopenssl_SSL_CTX_create_client();
    }

    run_test();

over:
    if (tls_mode || ssl_mode)
    {
        zopenssl_SSL_CTX_free(ssl_ctx);
        zopenssl_fini();
    }

    return 0;
}

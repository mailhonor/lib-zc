/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2023-06-05
 * ================================
 */

#include "zc.h"

static char *server = 0;
static int ssl_mode = 0;
static int tls_mode = 0;
const char *user = "";
const char *pass = "";
static void ___usage()
{
    zprintf("%s -server imap_server:port -user xxx@a.com -pass 123456 [--ssl ] [ --tls]\n", zvar_progname);
    exit(1);
}

static void debug_protocol_fn(int rw, const void *mem, int len)
{
    std::string s;
    s.append((const char *)mem, len);
    zprintf("debug_protocol_fn, %c: %s\n", rw, s.c_str());
}

int main(int argc, char **argv)
{
    SSL_CTX *ssl_ctx = 0;
    zcc::imap_client::folder_list_result folder_list_result;
    zcc::imap_client::select_result select_result;

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

    zcc::imap_client ic;
    ic.set_debug_mode();
    ic.set_verbose_mode();
    ic.set_debug_protocol_fn(debug_protocol_fn);
    ic.set_timeout(10);
    ic.set_ssl_tls(ssl_ctx, ssl_mode, tls_mode);

    if (ic.open(server) < 1)
    {
        goto over;
    }
    if (ic.do_auth(user, pass) < 1)
    {
        goto over;
    }
    if (ic.cmd_id() < 1) {
        goto over;
    }

    if (ic.get_all_folder_info(folder_list_result) < 1)
    {
        goto over;
    }
    for (auto it = folder_list_result.begin(); it != folder_list_result.end(); it++)
    {
        it->second.debug_show();
    }

    if (ic.cmd_select(select_result, "inbox") < 1)
    {
        goto over;
    }

    ic.close();

    zprintf("\n##############################\n\n");

over:
    if (tls_mode || ssl_mode)
    {
        zopenssl_SSL_CTX_free(ssl_ctx);
        zopenssl_fini();
    }

    return 0;
}

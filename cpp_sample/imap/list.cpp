/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2023-06-05
 * ================================
 */

#include "zcc/zcc_imap.h"

static const char *server = 0;
static int ssl_mode = 0;
static int tls_mode = 0;
static const char *user = "";
static const char *pass = "";

static void ___usage()
{
    zcc_fatal("%s -server imap_server:port -user xxx@a.com -pass 123456 [--ssl ] [ --tls] [ -timeout 10]", zcc::progname);
}

static void debug_protocol_fn(int rw, const void *mem, int len)
{
    std::string s;
    s.append((const char *)mem, len);
    zcc_info("debug_protocol_fn, %c: %s", rw, s.c_str());
}

static void debug_protocol_fn2(const char *ctx, int rw, const void *mem, int len)
{
    std::string s;
    s.append((const char *)mem, len);
    zcc_info("debug_protocol_fn(%s), %c: %s", ctx, rw, s.c_str());
}

int main(int argc, char **argv)
{
    SSL_CTX *ssl_ctx = 0;
    zcc::imap_client::folder_list_result folder_list_result;
    zcc::imap_client::select_result select_result;

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

    zcc::imap_client ic;
    ic.set_debug_mode();
    ic.set_verbose_mode();
    ic.set_debug_protocol_fn(debug_protocol_fn);
    ic.set_debug_protocol_fn(std::bind(debug_protocol_fn2, "abc", std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    ic.set_timeout(zcc::var_main_config.get_second("timeout", 10));
    ic.set_ssl_tls(ssl_ctx, ssl_mode, tls_mode);

    if (ic.connect(server) < 1)
    {
        goto over;
    }
    if (ic.do_auth(user, pass) < 1)
    {
        goto over;
    }
    if (ic.cmd_id() < 1)
    {
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

    ic.cmd_logout();
    ic.disconnect();

    zcc_info("\n##############################\n");

over:
    if (tls_mode || ssl_mode)
    {
        zcc::openssl::SSL_CTX_free(ssl_ctx);
        zcc::openssl::env_fini();
    }

    return 0;
}

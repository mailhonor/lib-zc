/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2023-06-05
 * ================================
 */

#include <time.h>
#include "zcc/zcc_imap.h"

static const char *server = 0;
static int ssl_mode = 0;
static int tls_mode = 0;
static const char *user = "";
static const char *pass = "";
static const char *folder = "";
static const char *eml = "";

static void ___usage()
{
    zcc_fatal("%s -server imap_server:port -user xxx@a.com -pass 123456 -folder abc -eml 1.eml [--ssl ] [ --tls] [ -timeout 10]", zcc::progname);
}

int main(int argc, char **argv)
{
    SSL_CTX *ssl_ctx = 0;
    zcc::imap_client::folder_list_result folder_list_result;
    zcc::imap_client::select_result select_result;
    zcc::imap_client::append_session append;
    zcc::imap_client::uidplus_result uidplus_result;

    zcc::main_argument::run(argc, argv);
    server = zcc::var_main_config.get_cstring("server", 0);
    ssl_mode = zcc::var_main_config.get_bool("ssl", false);
    tls_mode = zcc::var_main_config.get_bool("tls", false);
    user = zcc::var_main_config.get_cstring("user", "");
    pass = zcc::var_main_config.get_cstring("pass", "");
    folder = zcc::var_main_config.get_cstring("folder", "");
    eml = zcc::var_main_config.get_cstring("eml", "");

    if (zcc::empty(server) || zcc::empty(user) || zcc::empty(pass) || zcc::empty(folder) || zcc::empty(eml))
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

    if (ic.cmd_select(select_result, folder) < 1)
    {
        goto over;
    }

    append.to_folder_ = folder;
    append.flags_.flagged_ = true;
    append.time_ = time(0) - 3600 * 24;
    if (ic.append_file(uidplus_result, append, eml) < 1)
    {
        goto over;
    }

    zcc_info("NEW uidvalidity: %d, uid: %d", uidplus_result.uidvalidity_, uidplus_result.uid_);

    zcc_info("\n##############################\n\n");

    ic.cmd_logout();
    ic.disconnect();

over:
    if (tls_mode || ssl_mode)
    {
        zcc::openssl::SSL_CTX_free(ssl_ctx);
        zcc::openssl::env_fini();
    }

    return 0;
}

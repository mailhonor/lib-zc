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
const char *folder = "";
const char *eml = "";
static void ___usage()
{
    printf("%s -server imap_server:port -user xxx@a.com -pass 123456 -folder abc -eml 1.eml [--ssl ] [ --tls]\n", zvar_progname);
    exit(1);
}

int main(int argc, char **argv)
{
    SSL_CTX *ssl_ctx = 0;
    zcc::imap_client::folder_list_result folder_list_result;
    zcc::imap_client::select_result select_result;
    zcc::imap_client::append_session append;
    zcc::imap_client::uidplus_result uidplus_result;

    zmain_argument_run(argc, argv);
    server = zconfig_get_str(zvar_default_config, "server", 0);
    ssl_mode = zconfig_get_bool(zvar_default_config, "ssl", 0);
    tls_mode = zconfig_get_bool(zvar_default_config, "tls", 0);
    user = zconfig_get_str(zvar_default_config, "user", "");
    pass = zconfig_get_str(zvar_default_config, "pass", "");
    folder = zconfig_get_str(zvar_default_config, "folder", "");
    eml = zconfig_get_str(zvar_default_config, "eml", "");

    if (zempty(server) || zempty(user) || zempty(pass) || zempty(folder) || zempty(eml))
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
    ic.set_timeout(10);
    ic.set_destination(server);
    ic.set_user_password(user, pass);
    ic.set_ssl_tls(ssl_mode, tls_mode, ssl_ctx);

    if (!ic.open())
    {
        goto over;
    }

    if (!ic.cmd_select(select_result, folder))
    {
        goto over;
    }

    append.to_folder_ = folder;
    append.flags_.flagged_ = true;
    append.time_ = time(0) - 3600 * 24;
    if (!ic.append_file(uidplus_result, append, eml)) {
        goto over;
    }

    printf("NEW uidvalidity: %d, uid: %d\n", uidplus_result.uidvalidity_, uidplus_result.uid_);

    printf("\n##############################\n\n");

over:
    if (tls_mode || ssl_mode)
    {
        zopenssl_SSL_CTX_free(ssl_ctx);
        zopenssl_fini();
    }

    return 0;
}

/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2025-06-04
 * ================================
 */

#include "zcc/zcc_smtp.h"
#include "zcc/zcc_openssl.h"

static void ___usage()
{
    zcc_info("%s"
             "\n -server smtp_server:port"
             "\n  [ -user xxx@a.com -pass 123456 ] [--ssl ] [ --tls]"
             "\n  -from xxx -to xxx,xxx2,xxx3"
             "\n  -eml xxx.eml"
             "",
             zcc::progname);
}

static void process_callback(int64_t send, int64_t total)
{
    zcc_info("process: %lu/%lu, %lu%%", send, total, (send * 100) / total);
}

int main(int argc, char **argv)
{
    zcc::main_argument::run(argc, argv);
    SSL_CTX *ssl_ctx = nullptr;
    bool debug_mode = zcc::var_main_config.get_bool("debug", false);
    const char *server = zcc::var_main_config.get_cstring("server", "");
    bool ssl = zcc::var_main_config.get_bool("ssl", false);
    bool tls = zcc::var_main_config.get_bool("tls", false);
    const char *user = zcc::var_main_config.get_cstring("user", "");
    const char *pass = zcc::var_main_config.get_cstring("pass", "");
    const char *from = zcc::var_main_config.get_cstring("from", "");
    std::vector<std::string> to_list = zcc::split_and_ignore_empty_token(zcc::var_main_config.get_cstring("to", ""), ",");
    const char *eml_fn = zcc::var_main_config.get_cstring("eml", "");

    if (zcc::empty(server) || to_list.empty() || zcc::empty(eml_fn))
    {
        ___usage();
        return 1;
    }
    zcc::smtp_client smtp;
    smtp.set_debug_protocol_fn([](int sc, const char *line, int len)
                               {
                                   std::string str(line, len);
                                   std::fprintf(stderr, "%c: %s\n", sc, str.c_str());
                                   //
                               });

    smtp.set_debug_mode(debug_mode);
    smtp.set_ehlo_key("xxxtest");
    if (ssl || tls)
    {
        ssl_ctx = zcc::openssl::SSL_CTX_create_client();
    }
    if (ssl)
    {
        smtp.set_ssl_mode(ssl_ctx);
    }
    else if (tls)
    {
        smtp.set_try_tls_mode(ssl_ctx);
    }

    if (smtp.connect(server) < 1)
    {
        zcc_fatal("connect failed: (%s)", smtp.get_last_response_line().c_str());
    }
    if (smtp.do_auth(user, pass) < 1)
    {
        zcc_fatal("auth failed: (%s)", smtp.get_last_response_line().c_str());
    }
    if (smtp.cmd_mail_from(from) < 1)
    {
        zcc_fatal("mail from failed: (%s)", smtp.get_last_response_line().c_str());
    }
    for (auto it = to_list.begin(); it != to_list.end(); it++)
    {
        if (smtp.cmd_rcpt_to(it->c_str()) < 1)
        {
            zcc_fatal("rcpt to failed: (%s)", smtp.get_last_response_line().c_str());
        }
    }
    int64_t size_total = zcc::file_get_size(eml_fn);
    if (smtp.do_quick_send_file_once(eml_fn, [size_total](int64_t send)
                                     { process_callback(send, size_total); }) < 1)
    {
        zcc_fatal("send file failed: (%s)", smtp.get_last_response_line().c_str());
    }
    if (smtp.cmd_quit() < 1)
    {
        zcc_fatal("quit failed: (%s)", smtp.get_last_response_line().c_str());
    }
    zcc_info("ok");
    smtp.disconnect();
    if (ssl_ctx)
    {
        zcc::openssl::SSL_CTX_free(ssl_ctx);
    }
    return 0;
}

/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn
 * 2026-04-14
 * ================================
 */

#include "zcc/zcc_http.h"
#include "zcc/zcc_openssl.h"

int main(int argc, char **argv)
{
    zcc::main_argument::run(argc, argv);
    if (zcc::main_argument::var_parameter_argc < 1)
    {
        zcc_error_and_exit("%s <url> ", zcc::progname);
    }
    auto ssl_ctx = zcc::openssl::SSL_CTX_create_client();
    std::string url = zcc::main_argument::var_parameter_argv[0];
    //
    zcc::http_simple_client client;
    client.set_debug_mode(true);
    client.set_debug_protocol_mode(true);
    client.set_ssl_ctx(ssl_ctx);
    client.set_request_url(url);
    if (!client.send_request_headers() || !client.send_request_flush())
    {
        zcc_error_and_exit("send request data failed");
    }
    if (!client.recv_response_headers())
    {
        zcc_error_and_exit("recv response headers failed");
    }
    std::string data;
    if (!client.recv_response_all_data(data, 1024L * 1024 * 100))
    {
        zcc_error_and_exit("recv response data failed");
    }
    zcc_info("response data: %s", data.c_str());
    std::string location = client.get_response_location();
    zcc_info("status: %d", client.get_response_status_code());
    zcc_info("location: %s", location.c_str());
    //
    zcc::openssl::SSL_CTX_free(ssl_ctx);
    //
    return 0;
}

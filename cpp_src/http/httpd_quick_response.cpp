/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2017-07-22
 * ================================
 */

#include "zcc/zcc_http.h"

zcc_namespace_begin;

void httpd::response_200(const char *data, int64_t size, const response_options &options)
{
    if (size < 0)
    {
        size = std::strlen(data);
    }
    log_info("200 %d", size);
    if (size < 0)
    {
        size = std::strlen(data);
    }
    response_header_initialization("200 content");
    response_header("Server", "LIBZC HTTPD");
    if (!options.content_type.empty())
    {
        response_header_content_type(options.content_type, options.charset);
    }
    else
    {
        response_header_content_type("text/html");
    }
    response_header_content_length(size);
    if (options.is_gzip)
    {
        response_header("Content-Encoding", "gzip");
    }
    if (request_keep_alive_)
    {
        response_header("Connection", "keep-alive");
    }
    if (options.max_age > 0)
    {
        std::string tmp_value = "max-age=";
        tmp_value.append(std::to_string(options.max_age));
        response_header("Cache-Control", tmp_value);
        response_header_date("Expires", options.max_age + 1 + second());
    }
    else if (options.max_age == 0)
    {
        response_header("Cache-Control", "no-cache");
    }
    response_header_over();
    if (size > 0)
    {
        fp_->append(data, size);
    }
    response_flush();
}

void httpd::response_301(const char *url)
{
    log_info("301 -");
    response_header_initialization("301 Moved Permanently");
    fp_->append("Content-Length: 0\r\nServer: LIBZC HTTPD\r\nLocation: ").append(url).append("\r\n");
    if (request_keep_alive_)
    {
        response_header("Connection", "keep-alive");
    }
    response_header_over();
    response_flush();
}

void httpd::response_302(const char *url)
{
    log_info("302 -");
    response_header_initialization("302 Moved Temporarily");
    fp_->append("Content-Length: 0\r\nServer: LIBZC HTTPD\r\nLocation: ").append(url).append("\r\n");
    if (request_keep_alive_)
    {
        response_header("Connection", "keep-alive");
    }
    response_header_over();
    response_flush();
}

void httpd::response_304(const char *etag)
{
    log_info("304 -");
    response_header_initialization("304 Not Modified");
    fp_->append("Server: LIBZC HTTPD\r\nEtag: ").append(etag).append("\r\n");
    if (request_keep_alive_)
    {
        response_header("Connection", "keep-alive");
    }
    response_header_over();
    response_flush();
}

void httpd::response_404()
{
    log_info("404 -");
    response_header_initialization("404 Not Found");
    fp_->append("Server: LIBZC HTTPD\r\nContent-Type: text/html\r\n");
    if (request_keep_alive_)
    {
        response_header("Connection", "keep-alive");
    }
    const char body[] = "404 Not Found";
    fp_->append("Content-Length: ").append(std::to_string(sizeof(body) - 1)).append("\r\n");
    response_header_over();
    fp_->append(body);
    response_flush();
}

void httpd::response_416()
{
    log_info("416 -");
    response_header_initialization("416 Request Range Not Satisfiable");
    fp_->append("Server: LIBZC HTTPD\r\nContent-Type: text/html\r\nConnection: close\r\n");
    const char body[] = "416 Request Range Not Satisfiable";
    fp_->append("Content-Length: ").append(std::to_string(sizeof(body) - 1)).append("\r\n");
    response_header_over();
    fp_->append(body);
    response_flush();
}

void httpd::response_500()
{
    log_info("500 -");
    response_header_initialization("500 Error");
    fp_->append("Server: LIBZC HTTPD\r\nContent-Type: text/html\r\nConnection: close\r\n");
    const char body[] = "500 Internal Server Error";
    fp_->append("Content-Length: ").append(std::to_string(sizeof(body) - 1)).append("\r\n");
    response_header_over();
    fp_->append(body);
    response_flush();
}

void httpd::response_501()
{
    log_info("501 -");
    response_header_initialization("501 Not implemented");
    fp_->append("Server: LIBZC HTTPD\r\nContent-Type: text/html\r\nConnection: close\r\n");
    const char body[] = "501 Not implemented";
    fp_->append("Content-Length: ").append(std::to_string(sizeof(body) - 1)).append("\r\n");
    response_header_over();
    fp_->append(body);
    response_flush();
}

zcc_namespace_end;
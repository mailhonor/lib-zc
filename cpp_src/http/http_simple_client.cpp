/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2026-04-13
 * ================================
 */

#include "zcc/zcc_http.h"
#include "zcc/zcc_stream.h"
#include "zcc/zcc_openssl.h"
#include "zcc/zcc_mime.h"
#include <sstream>

zcc_namespace_begin;

#define zcc_http_simple_client_debug(...) ((debug_mode_) ? zcc_debug_output(__VA_ARGS__) : (void)0)

//
http_simple_client::http_simple_client()
{
}

http_simple_client::~http_simple_client()
{
    delete fp_;
    fp_ = nullptr;
    delete response_header_parser_;
    response_header_parser_ = nullptr;
}

void http_simple_client::reset_ctx()
{
    request_headers_.clear();
    request_cookies_.clear();
    request_basic_authorization_.clear();
    request_content_type_.clear();
    request_range_.clear();
    request_referer_.clear();
    request_content_length_ = -1;
    request_method_.clear();
    request_url_.clear();
    request_path_.clear();
    response_headers_.clear();
    response_cookies_.clear();
    response_content_type_.clear();
    response_location_ = "none";
    response_content_length_ = -1;
    response_header_data_.clear();
    delete response_header_parser_;
    response_header_parser_ = nullptr;
    reconnect_count_ = 1;
    response_is_gzip_ = 0;
    response_is_deflate_ = 0;
    response_is_keep_alive_ = 0;
    response_is_transfer_encoding_chunked_ = 0;
    response_status_code_ = -1;
    cookie_dealed_ = false;
    //
    if (error_ || need_close_)
    {
        delete fp_;
        fp_ = nullptr;
    }
    need_close_ = false;
    error_ = false;
}

void http_simple_client::set_connection_error()
{
    error_ = true;
}

void http_simple_client::set_timeout(int timeout)
{
    timeout_ = timeout;
    if (fp_)
    {
        fp_->set_timeout(timeout);
    }
}

void http_simple_client::add_request_header(const std::string &name, const std::string &value)
{
    std::string header = name + ": " + value;
    request_headers_.push_back(header);
}

void http_simple_client::add_request_cookie(const std::string &name, const std::string &value)
{
    request_cookies_[name] = value;
}

void http_simple_client::set_request_basic_authorization(const std::string &username, const std::string &password)
{
    std::string auth = username + ":" + password;
    std::string encoded;
    http_token_encode(auth.c_str(), auth.size(), encoded);
    request_basic_authorization_ = "Basic " + encoded;
}

void http_simple_client::set_request_content_type(const std::string &content_type)
{
    request_content_type_ = content_type;
}

void http_simple_client::set_request_content_type_application_x_www_form_urlencoded()
{
    request_content_type_ = "application/x-www-form-urlencoded";
}

void http_simple_client::set_request_content_type_multipart_form_data()
{
    request_content_type_ = "multipart/form-data";
}

void http_simple_client::set_request_content_length(int64_t content_length)
{
    request_content_length_ = content_length;
}

void http_simple_client::set_request_range(int64_t start, int64_t end)
{
    std::ostringstream oss;
    oss << "bytes=" << start << "-" << end;
    request_range_ = oss.str();
}

void http_simple_client::set_request_referer(const std::string &referer)
{
    request_referer_ = referer;
}

void http_simple_client::set_request_method(const std::string &method)
{
    request_method_ = method;
}

void http_simple_client::set_request_url(const std::string &url)
{
    http_url url_parser;
    url_parser.parse_url(url);
    auto schema = url_parser.protocol_;
    auto host = url_parser.host_;
    auto port = url_parser.port_;
    if (schema.empty())
    {
        schema = "http";
    }
    auto is_ssl = (schema == "https");
    if (port < 0)
    {
        port = 80;
        if (is_ssl)
        {
            port = 443;
        }
    }
    if (request_scheme_ != schema || request_host_ != host || request_port_ != port || request_is_ssl_ != is_ssl)
    {
        delete fp_;
        fp_ = nullptr;
        reset_ctx();
    }
    //
    request_path_ = url_parser.path_;
    if (request_path_.empty())
    {
        request_path_ = "/";
    }
    if (!url_parser.query_.empty())
    {
        request_path_ += "?" + url_parser.query_;
    }
    //
    request_url_ = url;
    request_scheme_ = schema;
    request_host_ = host;
    request_port_ = port;
    request_is_ssl_ = is_ssl;
}

bool http_simple_client::prepare_connection()
{
    if (fp_)
    {
        return true;
    }
    if (request_is_ssl_ && !ssl_ctx_)
    {
        zcc_http_simple_client_debug("ssl_ctx_ is null");
        set_connection_error();
        delete fp_;
        fp_ = nullptr;
        return false;
    }
    bool ok = false;
    fp_ = new iostream();
    fp_->set_timeout(timeout_);
    std::string destination = request_host_ + ":" + std::to_string(request_port_);
    for (int i = 0; i < reconnect_count_; i++)
    {
        if (!fp_->connect(destination.c_str()))
        {
            zcc_http_simple_client_debug("connect %s failed", destination.c_str());
            continue;
        }
        if (request_is_ssl_)
        {
            if (!fp_->tls_connect(ssl_ctx_))
            {
                zcc_http_simple_client_debug("tls_connect failed");
                set_connection_error();
                delete fp_;
                fp_ = nullptr;
                continue;
            }
        }
        ok = true;
        break;
    }
    if (!ok)
    {
        delete fp_;
        fp_ = nullptr;
    }
    return ok;
}

bool http_simple_client::send_request_headers()
{
    if (!prepare_connection())
    {
        return false;
    }
    if (request_method_.empty())
    {
        request_method_ = "GET";
    }
    //
    std::string headers;
    headers.append(request_method_).append(" ").append(request_path_).append(" HTTP/1.1\r\n");
    headers.append("Host: ").append(request_host_);
    if (request_port_ != 80 && request_port_ != 443)
    {
        headers.append(":").append(std::to_string(request_port_));
    }
    headers.append("\r\n");
    if (!request_basic_authorization_.empty())
    {
        headers.append("Authorization: ").append(request_basic_authorization_).append("\r\n");
    }
    if (accept_encoding_gzip_ && accept_encoding_deflate_)
    {
        headers.append("Accept-Encoding: gzip, deflate\r\n");
    }
    else if (accept_encoding_gzip_)
    {
        headers.append("Accept-Encoding: gzip\r\n");
    }
    else if (accept_encoding_deflate_)
    {
        headers.append("Accept-Encoding: deflate\r\n");
    }
    if (!request_content_type_.empty())
    {
        headers.append("Content-Type: ").append(request_content_type_).append("\r\n");
    }
    if (request_content_length_ >= 0)
    {
        headers.append("Content-Length: ").append(std::to_string(request_content_length_)).append("\r\n");
    }
    if (!request_range_.empty())
    {
        headers.append("Range: ").append(request_range_).append("\r\n");
    }
    if (!request_referer_.empty())
    {
        headers.append("Referer: ").append(request_referer_).append("\r\n");
    }
    if (keep_alive_)
    {
        headers.append("Connection: keep-alive\r\n");
    }
    else
    {
        headers.append("Connection: close\r\n");
    }
    if (!request_cookies_.empty())
    {
        std::string cookie_header = "Cookie: ";
        bool first = true;
        for (const auto &cookie : request_cookies_)
        {
            if (!first)
            {
                cookie_header += "; ";
            }
            cookie_header += http_cookie_build_item(cookie.first, cookie.second);
            first = false;
        }
        headers.append(cookie_header).append("\r\n");
    }
    for (const auto &header : request_headers_)
    {
        headers.append(header).append("\r\n");
    }
    headers.append("\r\n");
    //
    fp_->write(headers.c_str(), headers.size());
    //
    if (debug_protocol_mode_)
    {
        zcc_debug_output("request headers:\n%s", headers.c_str());
    }

    return true;
}

bool http_simple_client::send_request_data(const void *data, int64_t len)
{
    if (!fp_)
    {
        return false;
    }

    if (len == -1)
    {
        len = std::strlen((const char *)data);
    }

    return fp_->write(data, len) == len;
}

bool http_simple_client::send_request_flush()
{
    if (!fp_)
    {
        return false;
    }
    bool ok = fp_->flush() > 0;
    reset_ctx();
    return ok;
}

bool http_simple_client::parse_response_first_header_line(const std::string &line)
{
    if (line.empty())
    {
        return false;
    }
    if (line.size() < 10)
    {
        return false;
    }
    if (line.substr(0, 5) != "HTTP/")
    {
        return false;
    }
    auto pos = line.find(' ');
    if (pos == std::string::npos)
    {
        return false;
    }
    response_status_code_ = atoi(line.c_str() + pos + 1);
    return true;
}

bool http_simple_client::parse_response_headers_data(const std::string &headers_content)
{
    delete response_header_parser_;
    response_header_parser_ = mail_parser::create_from_data(headers_content);
    auto top_mime = response_header_parser_->get_top_mime();
    response_content_type_ = top_mime->get_content_type();
    //
    response_content_length_ = -1;
    auto content_length_str = top_mime->get_header_line_value("Content-Length");
    if (!content_length_str.empty())
    {
        response_content_length_ = atol(content_length_str);
    }
    //
    return true;
}

bool http_simple_client::recv_response_headers()
{
    if (!fp_)
    {
        return false;
    }
    response_headers_.clear();
    response_header_data_.clear();
    std::string line;
    bool first_line = true;
    while (true)
    {
        if (!fp_->gets(line, 10240))
        {
            zcc_http_simple_client_debug("recv_response_headers: read line failed");
            set_connection_error();
            return false;
        }
        trim_line_end_rn(line);
        if (line.empty())
        {
            break;
        }
        response_header_data_.append(line).append("\r\n");
        if (first_line)
        {
            first_line = false;
            if (!parse_response_first_header_line(line))
            {
                zcc_http_simple_client_debug("parse_response_first_header_line: line is empty or not HTTP/1.1");
                set_connection_error();
                return false;
            }
        }
    }
    if (debug_protocol_mode_)
    {
        zcc_debug_output("response headers:\n%s", response_header_data_.c_str());
    }
    response_header_data_.append("\r\n");
    if (!parse_response_headers_data(response_header_data_))
    {
        set_connection_error();
        return false;
    }

    if (!keep_alive_ || !get_response_is_keep_alive())
    {
        need_close_ = true;
    }

    return true;
}

int64_t http_simple_client::get_response_content_length()
{
    return response_content_length_;
}

void http_simple_client::deal_response_content_encoding()
{
    if (!response_header_parser_)
    {
        response_is_gzip_ = 2;
        response_is_deflate_ = 2;
        return;
    }
    auto encoding_str = response_header_parser_->get_header_line_value("Content-Encoding");
    if (encoding_str.empty())
    {
        response_is_gzip_ = 2;
        response_is_deflate_ = 2;
        return;
    }
    if (encoding_str.find("gzip") != std::string::npos)
    {
        response_is_gzip_ = 1;
    }
    else
    {
        response_is_gzip_ = 2;
    }
    if (encoding_str.find("deflate") != std::string::npos)
    {
        response_is_deflate_ = 1;
    }
    else
    {
        response_is_deflate_ = 2;
    }
}

bool http_simple_client::get_response_is_gzip()
{
    if (!response_is_gzip_)
    {
        deal_response_content_encoding();
    }
    return response_is_gzip_ == 1;
}
bool http_simple_client::get_response_is_deflate()
{
    if (!response_is_deflate_)
    {
        deal_response_content_encoding();
    }
    return response_is_deflate_ == 1;
}

int http_simple_client::get_response_status_code()
{
    return response_status_code_;
}

const std::string &http_simple_client::get_response_location()
{
    if (!response_header_parser_)
    {
        return response_location_;
    }
    if (response_location_ == "none")
    {
        response_location_ = response_header_parser_->get_header_line_value("Location");
    }
    return response_location_;
}

bool http_simple_client::get_response_is_transfer_encoding_chunked()
{
    if (response_is_transfer_encoding_chunked_ != 0)
    {
        return response_is_transfer_encoding_chunked_ == 1;
    }
    if (!response_header_parser_)
    {
        return false;
    }
    auto encoding_str = response_header_parser_->get_header_line_value("Transfer-Encoding");
    if (encoding_str.empty())
    {
        response_is_transfer_encoding_chunked_ = 2;
        return false;
    }
    if (encoding_str.find("chunked") != std::string::npos)
    {
        response_is_transfer_encoding_chunked_ = 1;
        return true;
    }
    return false;
}

const std::map<std::string, std::string> &http_simple_client::get_response_cookies()
{
    if (!response_header_parser_ || cookie_dealed_)
    {
        return response_cookies_;
    }
    cookie_dealed_ = true;
    auto cookie_str = response_header_parser_->get_header_line_value("Set-Cookie");
    response_cookies_ = http_cookie_parse(cookie_str);
    return response_cookies_;
}

const std::string &http_simple_client::get_response_filename()
{
    if (!response_header_parser_)
    {
        return var_blank_string;
    }
    return response_header_parser_->get_top_mime()->get_filename_utf8();
}

const std::vector<std::string> &http_simple_client::get_response_headers()
{
    if (!response_headers_.empty() || !response_header_parser_)
    {
        return response_headers_;
    }
    auto top_mime = response_header_parser_->get_top_mime();
    response_content_type_ = top_mime->get_content_type();
    auto &hs = top_mime->get_raw_header_line_vector();
    for (auto &header : hs)
    {
        auto h = mail_parser::header_line_unescape(header.data, header.size);
        response_headers_.push_back(h);
    }
    return response_headers_;
}

bool http_simple_client::get_response_is_keep_alive()
{
    if (response_is_keep_alive_)
    {
        return response_is_keep_alive_ == 1;
    }
    if (!response_header_parser_)
    {
        return false;
    }
    auto connection_str = response_header_parser_->get_header_line_value("Connection");
    if (connection_str.empty())
    {
        response_is_keep_alive_ = 2;
        return false;
    }
    if (connection_str.find("keep-alive") != std::string::npos)
    {
        response_is_keep_alive_ = 1;
        return true;
    }
    return false;
}

bool http_simple_client::recv_response_all_data(std::string *data_buf, FILE *data_fp, int64_t max_len)
{
    if (!fp_)
    {
        return false;
    }
    int64_t readed_len = 0;
    int64_t len, tmplen;
    char buf[4096 + 1];
    auto &fp = *fp_;
    bool is_chunked = get_response_is_transfer_encoding_chunked();

    if (is_chunked)
    {
        std::string line;
        while (1)
        {
            if (!fp.gets(line, 10240))
            {
                zcc_http_simple_client_debug("recv_response_all_data: read chunk size line failed");
                return false;
            }
            len = (int64_t)strtoll(line.c_str(), nullptr, 16);
            if (len < 0)
            {
                zcc_http_simple_client_debug("recv_response_all_data: chunk size line is not valid");
                return false;
            }
            readed_len += len;
            if (max_len > 0 && readed_len > max_len)
            {
                zcc_http_simple_client_debug("recv_response_all_data: readed len is max");
                return false;
            }
            if (len == 0)
            {
                if (fp.readn(buf, 2) != 2)
                {
                    zcc_http_simple_client_debug("recv_response_all_data: read chunk line failed");
                    return false;
                }
                break;
            }
            while (len > 0)
            {
                tmplen = len;
                if (tmplen > 4096)
                {
                    tmplen = 4096;
                }
                len -= tmplen;
                if (fp.readn(buf, tmplen) < tmplen)
                {
                    zcc_http_simple_client_debug("recv_response_all_data: read data failed");
                    return false;
                }
                if (data_buf)
                {
                    data_buf->append(buf, tmplen);
                }
                else if (data_fp)
                {
                    fwrite(buf, tmplen, 1, data_fp);
                }
            }
            if (fp.readn(buf, 2) != 2)
            {
                zcc_http_simple_client_debug("recv_response_all_data: read chunk data CR failed");
                return false;
            }
        }
    }
    else if (response_content_length_ > 0)
    {
        readed_len += response_content_length_;
        if (max_len > 0 && readed_len > max_len)
        {
            zcc_http_simple_client_debug("recv_response_all_data: readed len is max");
            return false;
        }
        len = response_content_length_;
        while (len > 0)
        {
            tmplen = len;
            if (tmplen > 4096)
            {
                tmplen = 4096;
            }
            len -= tmplen;
            if (fp.readn(buf, tmplen) < tmplen)
            {
                zcc_http_simple_client_debug("recv_response_all_data: read data failed");
                return false;
            }
            if (data_buf)
            {
                data_buf->append(buf, tmplen);
            }
            else if (data_fp)
            {
                fwrite(buf, tmplen, 1, data_fp);
            }
        }
    }
    else
    {
        while (1)
        {
            tmplen = fp.read(buf, 4096);
            if (tmplen < 0)
            {
                zcc_http_simple_client_debug("recv_response_all_data: read data failed");
                return false;
            }
            if (tmplen == 0)
            {
                break;
            }
            readed_len += tmplen;
            if (max_len > 0 && readed_len > max_len)
            {
                zcc_http_simple_client_debug("recv_response_all_data: readed len is max");
                return false;
            }
            if (data_buf)
            {
                data_buf->append(buf, tmplen);
            }
            else if (data_fp)
            {
                fwrite(buf, tmplen, 1, data_fp);
            }
        }
    }
    return true;
}

bool http_simple_client::recv_response_all_data(std::string &data_buf, int64_t max_len)
{
    auto ret = recv_response_all_data(&data_buf, nullptr, max_len);
    if (!ret)
    {
        set_connection_error();
    }
    return ret;
}

bool http_simple_client::recv_response_all_data(FILE *data_fp, int64_t max_len)
{
    auto ret = recv_response_all_data(nullptr, data_fp, max_len);
    if (!ret)
    {
        set_connection_error();
    }
    return ret;
}

zcc_namespace_end;
/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2017-07-22
 * ================================
 */

#include "zcc/zcc_http.h"

zcc_namespace_begin;

httpd::response_options httpd::default_response_options;

httpd::httpd(stream &fp)
{
    fp_ = &fp;
}

httpd::~httpd()
{
    loop_clear();
    if (fp_)
    {
        delete fp_;
    }
}

void httpd::loop_clear()
{
    url_.reset();
    method_.clear();
    version_.clear();
    uri_.clear();
    host_.clear();
    request_content_length_ = -1;
    request_post_vars_.clear();
    request_headers_.clear();
    request_cookies_.clear();
    request_uploaded_files_.clear();
    if (post_data_parser_)
    {
        delete post_data_parser_;
        post_data_parser_ = nullptr;
    }
    if (!post_data_pathname_.empty())
    {
        unlink(post_data_pathname_);
    }
    post_data_pathname_.clear();
    version_code_ = -1;
    request_gzip_dealed_ = false;
    request_gzip_ = false;
    request_deflate_dealed_ = false;
    request_deflate_ = false;
    request_cookies_dealed_ = false;
    request_url_dealed_ = false;
    request_keep_alive_ = false;
    first_request_ = false;
}

void httpd::request_read_first_header_line()
{
    if (exception_)
    {
        return;
    }
    std::string linebuf;
    const char *p, *ps;
    int ret;
    bool first = first_request_;

    /* read first header line */
    if (first)
    {
        first_request_ = false;
        ret = fp_->timed_read_wait(keep_alive_timeout_);
        if (ret < 1)
        {
            exception_ = true;
            return;
        }
    }

    ret = fp_->gets(linebuf, header_line_max_size_);
    if (ret < 0)
    {
        exception_ = true;
        log_error("exception: read banner line");
        return;
    }
    if (ret == 0)
    {
        if (first)
        {
            exception_ = true;
            log_error("exception: read banner line");
            return;
        }
        stop_ = true;
        return;
    }
    trim_line_end_rn(linebuf);

    ps = linebuf.c_str();
    p = std::strchr(ps, ' ');
    if (!p)
    {
        exception_ = true;
        log_error("exception: banner no blank");
        return;
    }
    method_.append(ps, p - ps);
    toupper(method_);
    ps = p + 1;

    p = std::strchr(ps, ' ');
    if (p)
    {
        version_ = p + 1;
        toupper(version_);
        uri_.append(ps, p - ps);
    }
    else
    {
        version_ = "HTTP/1.0";
        uri_ = "/";
    }
}

void httpd::request_read_other_header_line(bool &header_over)
{
    header_over = false;
    if (exception_)
    {
        return;
    }
    std::string linebuf, key;
    const char *p, *ps;

    if (fp_->gets(linebuf, header_line_max_size_) < 1)
    {
        exception_ = true;
        log_error("exception: read header line");
        return;
    }
    if (linebuf.back() != '\n')
    {
        exception_ = true;
        log_error("exception:  header line too long");
        return;
    }
    trim_line_end_rn(linebuf);
    if (linebuf.empty())
    {
        header_over = true;
        return;
    }
    ps = linebuf.c_str();
    p = std::strchr(ps, ':');
    if (!p)
    {
        return;
    }
    key.append(ps, p - ps);
    tolower(key);
    ps = p + 1;
    while (*ps == ' ')
    {
        ps++;
    }
    request_headers_[key] = ps;

    p = key.c_str();
    if (p[0] == 'h')
    {
        if (key == "host")
        {
            p = std::strchr(ps, ':');
            if (p)
            {
                host_.clear();
                host_.append(ps, p - ps);
                port_ = std::atoi(p + 1);
            }
            else
            {
                host_ = ps;
                port_ = -1;
            }
            tolower(host_);
        }
    }
    else if (p[0] == 'c')
    {
        if (key == "content-length")
        {
            request_content_length_ = std::atoi(ps);
        }
        else if (key == "connection")
        {
            if (strcasestr(ps, "keep-alive"))
            {
                request_keep_alive_ = true;
            }
        }
    }
}

void httpd::request_read_header()
{
    bool header_over = false;
    request_read_first_header_line();
    if (exception_)
    {
        return;
    }
    while (!header_over)
    {
        request_read_other_header_line(header_over);
        if (exception_)
        {
            return;
        }
    }
}

bool httpd::request_read_body_prepare()
{
    if (exception_ || stop_ || fp_->is_exception())
    {
        return false;
    }
    if (request_content_length_ < 1)
    {
        return false;
    }
    if ((max_length_for_post_ > 0) && (request_content_length_ > max_length_for_post_))
    {
        exception_ = true;
        log_error("exception: Content-Length>max_length_for_post");
        return false;
    }
    return true;
}

void httpd::request_read_body_x_www_form_urlencoded()
{
    std::string linebuf;
    if (fp_->readn(linebuf, request_content_length_) < request_content_length_)
    {
        exception_ = true;
        log_error("exception: read");
        return;
    }
    request_post_vars_ = http_url::parse_query(linebuf);
}

void httpd::request_read_body_disabled_form_data()
{
    // FIXME
    char header[] = " 501 Not implemented\r\nServer: LIBZC HTTPD\r\nConnection: close\r\n"
                    "Content-Length: ";
    char body[] = "501 Not implemented, unsupport multipart/form-data";
    fp_->append(version_);
    fp_->write(header, sizeof(header) - 1);
    fp_->append(std::to_string(sizeof(body) - 1)).append("\r\n\r\n");
    fp_->write(body, sizeof(body) - 1);
    response_flush();
}

std::string httpd::request_read_body_save_tmpfile(const char *content_type)
{
    std::string linebuf;
    std::string r;
    if (tmp_path_.empty())
    {
#ifdef _WIN64
        r = "%userprofile%/AppData/Local/Temp/";
#else  // __linux__
        r = "/tmp/";
#endif // __linux__
    }
    else
    {
        r = tmp_path_;
    }
    if (r.back() != '/')
    {
        r.push_back('/');
    }
    r.append(build_unique_id());
    const char *data_pathname = r.c_str();

    FILE *tmp_fp = fopen(data_pathname, "wb+");
    if (!tmp_fp)
    {
        exception_ = true;
        log_error("exception: open temp file");
        return "";
    }
    fprintf(tmp_fp, "Content-Type: %s\r\n\r\n", content_type);

    int64_t left = request_content_length_;
    while (left > 0)
    {
        int rlen = left;
        if (rlen > 4096)
        {
            rlen = 4096;
        }
        rlen = fp_->readn(linebuf, rlen);
        if (rlen < 1)
        {
            exception_ = true;
            log_error("exception: read post data body");
            break;
        }
        fwrite(linebuf.c_str(), 1, linebuf.size(), tmp_fp);
        left -= rlen;
        if (ferror(tmp_fp))
        {
            exception_ = true;
            log_error("exception: write post data body");
            break;
        }
    }
    if (!exception_)
    {
        if (fflush(tmp_fp) < 0)
        {
            exception_ = true;
            log_error("exception: write post data body");
        }
    }
    fclose(tmp_fp);
    if (exception_)
    {
        unlink(data_pathname);
        return "";
    }
    return r;
}

void httpd::request_read_body_uploaded_dump_file(mail_parser::mime_node *mime, const std::string &name, const std::string &pathname)
{
    int wlen = 0, raw_len = 0;
    const std::string &encoding = mime->get_encoding();
    raw_len = mime->get_body_size();
    if (encoding.empty())
    {
        wlen = raw_len;
    }
    else
    {
        wlen = -1;
    }

    httpd_uploaded_file fo(*this);
    fo.name_ = name;
    fo.pathname_ = pathname;
    fo.size_ = wlen;
    fo.offset_ = mime->get_body_offset();
    fo.raw_size_ = raw_len;
    if (encoding == "base64")
    {
        fo.encoding_ = var_encoding_type_base64;
    }
    else if (encoding == "quoted-printable")
    {
        fo.encoding_ = var_encoding_type_qp;
    }
    else
    {
        fo.encoding_ = var_encoding_type_none;
    }
    request_uploaded_files_.push_back(fo);
}

void httpd::request_read_body_form_data_one_mime(mail_parser::mime_node *mime)
{
    const std::string &disposition = mime->get_disposition();
    if (disposition != "form-data")
    {
        return;
    }

    std::string content_disposition = mime->get_header_line_value("content-disposition");
    std::string tmp_bf;
    dict params;

    mail_parser::header_line_get_params(content_disposition, tmp_bf, params);
    std::string name = get_cstring(params, "name", "");

    const std::string &ctype = mime->get_content_type();
    if (std::strncmp(ctype.c_str(), "multipart/", 10))
    {
        std::string pathname = get_cstring(params, "filename", "");
        if (pathname.empty())
        {
            if (!name.empty())
            {
                request_post_vars_[name] = mime->get_decoded_content();
            }
        }
        else
        {
            request_read_body_uploaded_dump_file(mime, name, pathname);
        }
    }
    else
    {
        for (auto child = mime->get_child(); !child; child = child->get_next())
        {
            const std::string &ctype = child->get_content_type();
            if (!strncmp(ctype.c_str(), "multipart/", 10))
            {
                continue;
            }
            std::string content_disposition = mime->get_header_line_value("content-disposition");
            std::string tmp_bf;
            dict params;

            mail_parser::header_line_get_params(content_disposition, tmp_bf, params);
            std::string name = get_cstring(params, "name", "");

            std::string pathname = get_cstring(params, "filename", "");
            request_read_body_uploaded_dump_file(mime, name, pathname);
            if (exception_)
            {
                break;
            }
        }
    }
}

void httpd::request_read_body_form_data(const char *content_type_raw)
{
    if (!enable_form_data_)
    {
        request_read_body_disabled_form_data();
        return;
    }

    std::string data_pathname = request_read_body_save_tmpfile(content_type_raw);
    if (exception_)
    {
        return;
    }
    if (!data_pathname.empty())
    {
        post_data_parser_ = mail_parser::create_from_file(data_pathname);
    }

    auto &mvec = post_data_parser_->get_all_mimes();

    for (auto it = mvec.begin(); it != mvec.end(); it++)
    {
        if (exception_)
        {
            break;
        }
        request_read_body_form_data_one_mime(*it);
    }

    post_data_pathname_ = data_pathname;
}

void httpd::request_read_body()
{
    if (!request_read_body_prepare())
    {
        return;
    }

    const char *content_type = get_cstring(request_headers_, "content-type");
    if (empty(content_type))
    {
        return;
    }

    if (ZCC_STR_N_CASE_EQ(content_type, "application/x-www-form-urlencoded", 33))
    {
        request_read_body_x_www_form_urlencoded();
        return;
    }

    if (!ZCC_STR_N_CASE_EQ(content_type, "multipart/form-data", 19))
    {
        return;
    }

    request_read_body_form_data(content_type);
}

bool httpd::request_read_all()
{
    loop_clear();
    request_read_header();
    request_read_body();
    if (exception_ || stop_)
    {
        return false;
    }
    return true;
}

bool httpd::maybe_continue()
{
    if (exception_ || stop_ || (!request_keep_alive_))
    {
        return false;
    }
    return true;
}

int httpd::request_get_version_code_deal()
{
    if (version_code_ == -1)
    {
        if (std::strstr(version_.c_str(), "1.0"))
        {
            version_code_ = 0;
        }
        else
        {
            version_code_ = 1;
        }
    }
    return version_code_;
}

bool httpd::request_is_gzip_deal()
{
    if (!request_gzip_dealed_)
    {
        request_gzip_dealed_ = true;
        request_gzip_ = (strcasestr(get_cstring(request_headers_, "accept-encoding", ""), "gzip") ? true : false);
    }
    return request_gzip_;
}

bool httpd::request_is_deflate_deal()
{
    if (!request_deflate_dealed_)
    {
        request_deflate_dealed_ = true;
        request_deflate_ = (strcasestr(get_cstring(request_headers_, "accept-encoding", ""), "deflate") ? true : false);
    }
    return request_deflate_;
}

const dict &httpd::request_cookies_deal()
{
    if (!request_cookies_dealed_)
    {
        request_cookies_dealed_ = true;
        const char *ps = get_cstring(request_headers_, "cookie", "");
        request_cookies_ = http_cookie_parse(ps);
    }
    return request_cookies_;
}

const http_url &httpd::request_url_deal()
{
    if (!request_url_dealed_)
    {
        request_url_dealed_ = true;
        url_ = http_url(uri_);
    }
    return url_;
}

void httpd::response_header_initialization(const char *version, const char *status)
{
    if (!empty(version))
    {
        fp_->append(version);
    }
    else
    {
        fp_->append(version_);
    }
    fp_->append(" ").append(status).append("\r\n");
}

void httpd::response_header(const char *name, const char *value)
{
    fp_->append(name).append(": ").append(value).append("\r\n");
}

void httpd::response_header(const char *name, int64_t value)
{
    std::string r = std::to_string(value);
    response_header(name, r.c_str());
}

void httpd::response_header_date(const char *name, int64_t value)
{
    response_header(name, rfc1123_time(value));
}

void httpd::response_header_content_type(const char *value, const char *charset)
{
    std::string v;
    v.append(value);
    if (empty(charset))
    {
        charset = "UTF-8";
    }
    v.append("; charset=").append(charset);
    response_header("Content-Type", v);
}

void httpd::response_header_set_cookie(const char *name, const char *value, int64_t expires, const char *path, const char *domain, bool secure, bool httponly)
{
    std::string r = http_cookie_build_item(name, value, expires, path, domain, secure, httponly);
    response_header("Set-Cookie", r);
}

void httpd::response_header_over()
{
    fp_->append("\r\n");
}

httpd &httpd::response_write(const void *data, int len)
{
    fp_->write(data, len);
    return *this;
}

bool httpd::response_printf_1024(const char *format, ...)
{
    va_list ap;
    char buf[1024 + 1];
    int len;

    if (format == 0)
    {
        format = "";
    }
    va_start(ap, format);
    len = std::vsnprintf(buf, 1024, format, ap);
    len = ((len < 1024) ? len : (1024 - 1));
    va_end(ap);
    return fp_->write(buf, len) > 0;
}

bool httpd::response_flush()
{
    if (stop_ || exception_)
    {
        return false;
    }
    if (fp_->flush() < 0)
    {
        exception_ = true;
        log_error("exception: write");
        return false;
    }
    return true;
}

stream *httpd::detach_stream()
{
    stop_ = true;
    auto fp = fp_;
    fp_ = nullptr;
    return fp;
}

zcc_namespace_end;
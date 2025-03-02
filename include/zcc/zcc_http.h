/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2017-07-22
 * ================================
 */

#pragma once

#ifndef ZCC_LIB_INCLUDE_HTTP___
#define ZCC_LIB_INCLUDE_HTTP___

#include "./zcc_stdlib.h"
#include "./zcc_openssl.h"
#include "./zcc_stream.h"
#include "./zcc_mime.h"

#ifdef __cplusplus
#pragma pack(push, 4)
zcc_namespace_begin;

class http_url
{
public:
    static dict parse_query(const char *query, bool lowercase_mode = false);
    inline static dict parse_query(const std::string &query, bool lowercase_mode = false)
    {
        return parse_query(query.c_str(), lowercase_mode);
    }
    static std::string build_query(const dict &qs, bool strict = true);

public:
    http_url();
    http_url(const char *url, int len = -1);
    http_url(const std::string &url);
    ~http_url();
    void parse_url(const char *url, int len = -1);
    inline void parse_url(const std::string &url)
    {
        parse_url(url.c_str());
    }
    std::string build_url();
    void reset();
    void debug_show();

public:
    std::string protocol_;
    std::string destination_;
    std::string host_;
    std::string path_;
    std::string query_;
    std::string fragment_;
    dict querys_;
    int port_{-1};
};

// token
void http_token_encode(const void *src, int src_size, std::string &result, bool strict_flag = false);
inline void http_token_encode(const void *src, std::string &result, bool strict_flag = false)
{
    http_token_encode(src, -1, result, strict_flag);
}
inline void http_token_encode(const std::string &src, std::string &result, bool strict_flag = false)
{
    http_token_encode(src.c_str(), src.size(), result, strict_flag);
}
std::string http_token_encode(const void *src, int src_size, bool strict_flag = false);
inline std::string http_token_encode(const void *src, bool strict_flag = false)
{
    return http_token_encode(src, -1, strict_flag);
}
inline std::string http_token_encode(const std::string &src, bool strict_flag = false)
{
    return http_token_encode(src.c_str(), src.size(), strict_flag);
}

void http_token_decode(const void *src, int src_size, std::string &result);
inline void http_token_decode(const void *src, std::string &result)
{
    http_token_decode(src, -1, result);
}
inline void http_token_decode(const std::string &src, std::string &result)
{
    http_token_decode(src.c_str(), src.size(), result);
}
std::string http_token_decode(const void *src, int src_size = -1);
inline std::string http_token_decode(const std::string &src)
{
    return http_token_decode(src.c_str(), src.size());
}

// cookie
dict http_cookie_parse(const char *raw_cookie);
inline dict http_cookie_parse(const std::string &raw_cookie)
{
    return http_cookie_parse(raw_cookie.c_str());
}

std::string http_cookie_build_item(const char *name, const char *value = nullptr, int64_t expires = 0, const char *path = nullptr, const char *domain = nullptr, bool secure = false, bool httponly = false);
inline std::string http_cookie_build_item(const std::string &name, const std::string &value, int64_t expires, std::string &path, std::string &domain, bool secure, bool httponly)
{
    return http_cookie_build_item(name.c_str(), value.c_str(), expires, path.c_str(), domain.c_str(), secure, httponly);
}

// httpd uploaded file
class httpd_uploaded_file
{
    friend class httpd;

public:
    inline httpd_uploaded_file(httpd &hd) : httpd_(hd) {}
    inline ~httpd_uploaded_file() {}
    // 上传文件名称
    inline const std::string &get_name() { return name_; }
    // 上传文件路径名称
    inline const std::string &get_pathname() { return pathname_; }
    // 上传文件大小; -1: 错
    int64_t get_size();
    // 获取上传文件数据
    std::string get_data();
    // 保存上传文件到指定文件路径
    bool save_to(const char *pathname);
    inline bool save_to(const std::string &pathname)
    {
        return save_to(pathname.c_str());
    }

public:
    httpd &httpd_;
    std::string name_;
    std::string pathname_;
    int64_t size_{-1};

protected:
    int64_t raw_size_{-2};
    int64_t offset_{-1};
    int encoding_{0};
};
// httpd
class httpd
{
    friend class httpd_uploaded_file;

public:
    enum log_level
    {
        debug = 1,
        info,
        error,
    };

    struct response_options
    {
        std::string content_type;
        std::string charset;
        int64_t max_age{-1};
        bool is_gzip{false};
    };
    static response_options default_response_options;

public:
    httpd(stream &fp);
    virtual ~httpd();

    // 设置keep-alive超时时间, timeout: 秒
    inline void set_keep_alive_timeout(int timeout)
    {
        keep_alive_timeout_ = timeout;
    }

    // 设置post方式最大长度
    inline void set_max_length_for_post(int max_length)
    {
        max_length_for_post_ = max_length;
    }

    // 设置post方式临时目录
    inline void set_tmp_path(const char *tmp_path)
    {
        tmp_path_ = tmp_path;
    }
    inline void set_tmp_path(const std::string &tmp_path)
    {
        tmp_path_ = tmp_path;
    }

    // 设置支持form_data协议, 默认不支持
    inline void set_enable_form_data(bool tf = true)
    {
        enable_form_data_ = tf;
    }

    // 超时等待可读, 单位秒, timeout<0: 表示无限长
    inline void set_timeout(int wait_timeout)
    {
        fp_->set_timeout(wait_timeout);
    }

    // 是否可以继续循环读取下一个请求
    bool maybe_continue();

    inline void set_stop()
    {
        stop_ = true;
    }
    inline bool is_stop()
    {
        return stop_;
    }

    // 是否异常
    inline void set_exception()
    {
        exception_ = true;
    }
    inline bool is_exception()
    {
        if (exception_ || stop_ || fp_->is_exception())
        {
            return true;
        }
        return false;
    }

    //
    bool request_read_all();

    // 请求的方法: GET/POST/...
    inline const std::string &request_get_method()
    {
        return method_;
    }

    // 请求的主机名
    inline const std::string &request_get_host()
    {
        return host_;
    }

    //
    inline const http_url &request_get_url()
    {
        return request_url_dealed_ ? url_ : request_url_deal();
    }

    // 请求的url路径
    inline const std::string &request_get_path()
    {
        return request_url_dealed_ ? url_.path_ : request_url_deal().path_;
    }

    // 请求的URI
    inline const std::string &request_get_uri()
    {
        return uri_;
    }

    // http版本
    const std::string &request_get_version()
    {
        return version_;
    }

    // 0: 1.0;  1: 1.1
    inline int request_get_version_code()
    {
        return (version_code_ == 0XFF) ? request_get_version_code_deal() : version_code_;
    }

    // 分析http头Content-Lengtgh字段
    inline int64_t request_get_content_length()
    {
        return request_content_length_;
    }

    // 请求是否是 gzip/deflate
    inline bool request_is_gzip()
    {
        return request_gzip_dealed_ ? request_gzip_ : request_is_gzip_deal();
    }
    inline bool request_is_deflate()
    {
        return request_deflate_dealed_ ? request_deflate_ : request_is_deflate_deal();
    }
    // keep_alive
    inline bool request_is_keep_alive()
    {
        return request_keep_alive_;
    }

    // 全部http头
    inline const dict &request_get_headers()
    {
        return request_headers_;
    }

    // url queries, 解析后
    inline const dict &request_get_query_vars()
    {
        return request_url_dealed_ ? url_.querys_ : request_url_deal().querys_;
    }

    // post queries, 解析后
    inline const dict &request_get_post_vars()
    {
        return request_post_vars_;
    }

    // cookies, 解析后
    inline const dict &request_get_cookies()
    {
        return request_cookies_dealed_ ? request_cookies_ : request_cookies_deal();
    }

    // 获取全部上传文件的信息
    const std::vector<httpd_uploaded_file> &request_get_uploaded_files()
    {
        return request_uploaded_files_;
    }

    // 快速回复
    virtual void response_200(const char *data, int64_t size, const response_options &options = default_response_options);
    virtual inline void response_200(const char *data, const response_options &options = default_response_options)
    {
        response_200(data, -1, options);
    }
    virtual inline void response_200(const std::string &data, const response_options &options = default_response_options)
    {
        response_200(data.c_str(), data.size(), default_response_options);
    }
    virtual void response_301(const char *url);
    virtual void response_302(const char *url);
    virtual void response_304(const char *etag);
    virtual inline void response_304(const std::string &etag)
    {
        return response_304(etag.c_str());
    }
    virtual void response_404();
    virtual void response_416();
    virtual void response_500();
    virtual void response_501();

    //  输出 initialization; version: "http/1.0", "http/1.1", 0(采用请求值); status: 如 "200 XXX"
    void response_header_initialization(const char *version, const char *status);
    inline void response_header_initialization(const char *status)
    {
        response_header_initialization(nullptr, status);
    }
    inline void response_header_initialization(const std::string &status)
    {
        response_header_initialization(status.c_str());
    }

    // 输出header
    void response_header(const char *name, const char *value);
    void response_header(const char *name, int64_t value);
    inline void response_header(const std::string &name, const std::string &value)
    {
        response_header(name.c_str(), value.c_str());
    }
    inline void response_header(const std::string &name, int64_t value)
    {
        response_header(name.c_str(), value);
    }

    // 输出header, value为date类型
    void response_header_date(const char *name, int64_t value);
    inline void response_header_date(const std::string &name, int64_t value)
    {
        response_header_date(name.c_str(), value);
    }

    // 输出Content-Type; charset: 为空则取 UTF-8
    void response_header_content_type(const char *value, const char *charset = nullptr);
    inline void response_header_content_type(const std::string &value, const std::string &charset = var_blank_string)
    {
        response_header_content_type(value.c_str(), charset.c_str());
    }

    // 输出http体长度
    inline void response_header_content_length(int64_t length)
    {
        response_header("Content-Length", length);
    }

    // 设置cookie
    void response_header_set_cookie(const char *name, const char *value = nullptr, int64_t expires = 0, const char *path = nullptr, const char *domain = nullptr, bool secure = false, bool httponly = false);
    inline void response_header_unset_cookie(const char *name)
    {
        response_header_set_cookie(name);
    }

    // http头输出完毕
    void response_header_over();

    // 写http体
    httpd &response_write(const void *data, int len = -1);
    inline httpd &response_puts(const char *data)
    {
        return response_write(data, -1);
    }
    inline httpd &response_append(const std::string &bf)
    {
        return response_write(bf.c_str(), bf.size());
    }
    inline httpd &response_append(const char *data)
    {
        return response_write(data, -1);
    }
#ifdef __linux__
    bool __attribute__((format(gnu_printf, 2, 3))) response_printf_1024(const char *format, ...);
#else  // __linux__
    bool response_printf_1024(const char *format, ...);
#endif // __linux__
    bool response_flush();

    // 获取http的stream
    inline stream *get_stream() { return fp_; }

    // 从 httpd 中解耦 fp
    stream *detach_stream();

    // 输出一个文件
    bool response_file(const char *pathname, const response_options &options = default_response_options);
    bool response_file(const std::string &pathname, const response_options &options = default_response_options)
    {
        return response_file(pathname.c_str(), options);
    }

    void vlog_output(log_level ll, const char *fmt, va_list ap);
    void log_info(const char *fmt, ...);
    void log_error(const char *fmt, ...);

    // websocket 相关
    bool is_websocket_upgrade();
    int get_websocket_version();
    const char *get_websocket_key();
    bool websocket_shakehand();

protected:
    void loop_clear();
    void request_read_header();
    void request_read_body();
    virtual std::string get_log_prefix();

private:
    int request_get_version_code_deal();
    bool request_is_gzip_deal();
    bool request_is_deflate_deal();
    const dict &request_cookies_deal();
    const http_url &request_url_deal();
    void request_read_first_header_line();
    void request_read_other_header_line(bool &header_over);
    bool request_read_body_prepare();
    void request_read_body_x_www_form_urlencoded();
    void request_read_body_form_data(const char *content_type);
    void request_read_body_disabled_form_data();
    std::string request_read_body_save_tmpfile(const char *content_type);
    void request_read_body_form_data_one_mime(mail_parser::mime_node *mime);
    void request_read_body_uploaded_dump_file(mail_parser::mime_node *mime, const std::string &name, const std::string &pathname);

protected:
    stream *fp_{nullptr};
    http_url url_;
    std::string method_;
    std::string version_;
    std::string uri_;
    std::string host_;
    int port_{-1};
    int64_t request_content_length_;
    dict request_post_vars_;
    dict request_headers_;
    dict request_cookies_;
    std::vector<httpd_uploaded_file> request_uploaded_files_;
    mail_parser *post_data_parser_{nullptr};
    std::string post_data_pathname_;
    unsigned char version_code_{0XFF};
    bool request_gzip_dealed_{false};
    bool request_gzip_{false};
    bool request_deflate_dealed_{false};
    bool request_deflate_{false};
    bool request_cookies_dealed_{false};
    bool request_url_dealed_{false};
    bool stop_{false};
    bool first_request_{false};
    bool exception_{false};
    bool request_keep_alive_{false};
    bool enable_form_data_{false};
    bool debug_{false};
    //
    int header_line_max_size_{10240};
    int keep_alive_timeout_{30};
    int max_length_for_post_{-1};
    std::string tmp_path_;
};

class websocketd
{
public:
    static char opcode_continue;
    static char opcode_text;
    static char opcode_binary;
    static char opcode_close;
    static char opcode_ping;
    static char opcode_pong;

public:
    websocketd(stream &fp);
    virtual ~websocketd();
    inline bool get_header_fin()
    {
        return fin_;
    }
    inline char get_header_opcode()
    {
        return opcode_;
    }
    inline int64_t get_payload_len()
    {
        return payload_len_;
    }
    bool read_frame_header();
    int read_frame_data(void *data, int len);
    int read_frame_data(std::string &data, int len);
    //
    bool write_frame_head_with_flags(int len, bool fin_flag, int opcode);
    bool write_frame_head_text(int len);
    bool write_frame_head_binary(int len);
    bool write_frame_data(const void *data, int len = -1);
    inline bool write_frame_data(const std::string &data)
    {
        return write_frame_data((const void *)data.c_str(), data.size());
    }
    bool write_frame_flush();
    //
    bool send_data_with_opcode(const void *data, int len, int opcode);
    bool send_ping(const void *data, int len = -1);
    inline bool send_ping(const std::string &data)
    {
        return send_ping((const char *)data.c_str(), data.size());
    }
    bool send_pong(const void *data, int len = -1);
    inline bool send_pong(const std::string &data)
    {
        return send_pong((const char *)data.c_str(), data.size());
    }
    bool send_text(const void *data, int len = -1);
    inline bool send_text(const std::string &data)
    {
        return send_text((const char *)data.c_str(), data.size());
    }
    bool send_binary(const void *data, int len = -1);
    inline bool send_binary(const std::string &data)
    {
        return send_binary((const char *)data.c_str(), data.size());
    }
    bool send_text_printf_1024(const char *format, ...);

protected:
    stream *fp_{nullptr};
    bool fin_{false};
    bool rsv1_{false};
    bool rsv2_{false};
    bool rsv3_{false};
    bool mask_{false};
    char opcode_;
    char opcode_for_continue_;
    unsigned char masking_key_[4];
    int64_t payload_len_;
    int64_t readed_len_;
};

zcc_namespace_end;
#pragma pack(pop)
#endif // __cplusplus

#endif // ZCC_LIB_INCLUDE_HTTP___

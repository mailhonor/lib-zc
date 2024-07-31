/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2017-04-05
 * ================================
 */

#include "zcc/zcc_http.h"
#include "zcc/zcc_errno.h"
#include "sys/stat.h"

zcc_namespace_begin;

httpd::response_options default_response_options;

class http_response_file_ctx
{
public:
    http_response_file_ctx(httpd &_h, const char *_pathname, const httpd::response_options &_options);
    ~http_response_file_ctx();
    void parse_range();
    bool stage_1();
    bool output_data(int64_t offset1, int64_t offset2);
    bool do_no_range();
    bool do_one_range();
    bool do_more_range();
    int64_t do_more_range_compute_content_length();
    httpd &h;
    const char *pathname{nullptr};
    const httpd::response_options &options;
    FILE *fp{nullptr};
    struct stat st;
    int64_t size;
    std::string new_etag;
    std::string content_type;
    std::vector<int64_t> offsets;
    const char *range{nullptr};
    const char *old_etag{nullptr};
    bool output_ok{false};
};

http_response_file_ctx::http_response_file_ctx(httpd &_h, const char *_pathname, const httpd::response_options &_options) : h(_h), pathname(_pathname), options(_options)
{
}

http_response_file_ctx::~http_response_file_ctx()
{
}

void http_response_file_ctx::parse_range()
{
    std::vector<int64_t> tmp_offsets;
    const char *ps = std::strchr(range, '=');
    if (!ps)
    {
        return;
    }
    ps += 1;
    while (*ps)
    {
        const char *p = std::strchr(ps, '-');
        if (!p)
        {
            break;
        }
        p += 1;
        int64_t o1 = std::atol(ps);
        int64_t o2 = std::atol(p);
        if (o1 >= o2)
        {
            return;
        }
        if (o2 >= size)
        {
            return;
        }
        tmp_offsets.push_back(o1);
        tmp_offsets.push_back(o2);
        ps = strchr(p, ',');
        if (!ps)
        {
            break;
        }
        ps += 1;
    }
    offsets = tmp_offsets;
}

bool http_response_file_ctx::stage_1()
{
    int err;
    std::string tmp_value;
    if (!(fp = fopen(pathname, "rb")))
    {
        err = get_errno();
        if (err == ZCC_ENOENT)
        {
            h.response_404();
            return true;
        }
        else
        {
            h.response_500();
            return true;
        }
    }
    if (fstat(fileno(fp), &st) == -1)
    {
        h.response_500();
        return true;
    }
    size = st.st_size;

    range = get_cstring(h.request_get_headers(), "range", "");
    old_etag = get_cstring(h.request_get_headers(), "if-none-match", "");
    if (*range && *old_etag)
    {
        h.response_416();
        return true;
    }

    new_etag.append(std::to_string(st.st_mtime)).append("-").append(std::to_string(size));
    if (*old_etag)
    {
        if (old_etag == new_etag)
        {
            h.response_304(new_etag);
            output_ok = true;
            return true;
        }
    }

    if (*range && (!options.is_gzip))
    {
        parse_range();
    }

    if (offsets.empty())
    {
        h.log_info("200 %zd", size);
        h.response_header_initialization("200 content");
    }
    else
    {
        h.log_info("206 %s", range);
        h.response_header_initialization("206 Partial Content");
    }
    h.response_header("Etag", new_etag);
    if (st.st_mtime > 0)
    {
        h.response_header_date("Last-Modified", st.st_mtime);
    }
    if (options.max_age > 0)
    {
        tmp_value = "max-age=";
        tmp_value.append(std::to_string(options.max_age));
        h.response_header("Cache-Control", tmp_value);
        h.response_header_date("Expires", options.max_age + 1 + second());
    }
    else if (options.max_age == 0)
    {
        h.response_header("Cache-Control", "no-cache");
    }

    if (options.is_gzip)
    {
        h.response_header("Content-Encoding", "gzip");
    }

    if (h.request_is_keep_alive())
    {
        h.response_header("Connection", "keep-alive");
    }

    content_type = options.content_type;
    if (content_type.empty())
    {
        content_type = get_mime_type_from_pathname(pathname, var_default_mime_type);
    }

    return false;
}

bool http_response_file_ctx::output_data(int64_t offset, int64_t offset2)
{
    int64_t length = offset2 - offset + 1;
    if (length < 1)
    {
        return false;
    }
    if (fseek(fp, offset, SEEK_SET) < 0)
    {
        h.set_exception();
        return true;
    }
    auto http_fp = h.get_stream();
    for (int64_t i = 0; i < length; i++)
    {
        int ch = fgetc(fp);
        if (ch == EOF)
        {
            h.set_exception();
            return true;
        }
        http_fp->putc(ch);
        if (h.is_exception())
        {
            return true;
        }
    }
    return false;
}

bool http_response_file_ctx::do_no_range()
{
    h.response_header_content_length(size);
    h.response_header_content_type(content_type);
    h.response_header_over();
    if (output_data(0, size - 1))
    {
        return true;
    }
    return false;
}

bool http_response_file_ctx::do_one_range()
{
    std::string tmp_value = "bytes ";
    tmp_value.append(std::to_string(offsets[0])).append("-").append(std::to_string(offsets[1]));
    tmp_value.append("/").append(std::to_string(size));
    h.response_header("Content-Range", tmp_value);
    h.response_header_content_type(content_type);
    h.response_header_content_length(offsets[1] - offsets[0] + 1);
    h.response_header_over();
    if (output_data(offsets[0], offsets[1]))
    {
        return true;
    }
    return false;
}

int64_t http_response_file_ctx::do_more_range_compute_content_length()
{
    // --00000000000000000016
    // Content-Type: text/html
    // Content-Range: bytes 4-5/1305

    // ct
    // --00000000000000000016--
    int64_t r = 0;
    std::string tmpsize = std::to_string(size);
    for (auto it = offsets.begin(); it != offsets.end();)
    {
        int64_t i1 = *it++;
        int64_t i2 = *it++;
        std::string s1 = std::to_string(i1);
        std::string s2 = std::to_string(i2);
        r += (sizeof("--00000000000000000016") - 1) + 2;
        r += (sizeof("Content-Type: ") - 1) + content_type.size() + 2;
        r += (sizeof("Content-Range: bytes ") - 1) + s1.size() + 1 + s2.size() + 1 + tmpsize.size() + 2;
        r += 2;
        r += (i2 - i1 + 1) + 2;
    }
    r += (sizeof("--00000000000000000016") - 1) + 2;
    return r;
}

bool http_response_file_ctx::do_more_range()
{
    std::string boundary = build_unique_id();
    boundary.resize(20);
    h.response_header_content_length(do_more_range_compute_content_length());
    h.response_append("Content-Type: multipart/byteranges; boundary=").response_append(boundary).response_append("\r\n");
    h.response_header_over();

    for (auto it = offsets.begin(); it != offsets.end();)
    {
        int64_t i1 = *it++;
        int64_t i2 = *it++;
        h.response_append("--").response_append(boundary).response_append("\r\n");
        h.response_append("Content-Type: ").response_append(content_type).response_append("\r\n");
        h.response_append("Content-Range: bytes ").response_append(std::to_string(i1)).response_append("-");
        h.response_append(std::to_string(i2)).response_append("/").response_append(std::to_string(size)).response_append("\r\n");
        h.response_append("\r\n");
        if (output_data(i1, i2))
        {
            return true;
        }
        h.response_append("\r\n");
    }
    h.response_append("--").response_append(boundary).response_append("--\r\n");

    return false;
}

bool httpd::response_file(const char *pathname, const response_options &options)
{
    bool ended = false;
    std::string tmp_value;
    http_response_file_ctx ctx(*this, pathname, options);

    if (ctx.stage_1())
    {
        return ctx.output_ok;
    }

    if (ctx.offsets.empty())
    {
        ended = ctx.do_no_range();
    }
    else if (ctx.offsets.size() == 2)
    {
        ended = ctx.do_one_range();
    }
    else
    {
        ended = ctx.do_more_range();
    }
    response_flush();
    if (ended)
    {
        return exception_ ? false : true;
    }
    return false;
}

zcc_namespace_end;

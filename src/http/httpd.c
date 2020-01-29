/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2017-04-05
 * ================================
 */

#include "httpd.h"

static void zhttpd_loop_clear(zhttpd_t *httpd);
static void zhttpd_request_header_do(zhttpd_t * httpd, zbuf_t *linebuf);
static void zhttpd_request_data_do(zhttpd_t *httpd, zbuf_t *linebuf);

zbool_t zvar_httpd_debug = 0;
zbool_t zvar_httpd_no_cache = 0;

const char *zhttpd_uploaded_file_get_name(zhttpd_uploaded_file_t *fo)
{
    return fo->name;
}

const char *zhttpd_uploaded_file_get_pathname(zhttpd_uploaded_file_t *fo)
{
    return fo->pathname;
}

int zhttpd_uploaded_file_get_size(zhttpd_uploaded_file_t *fo)
{
    if (fo->size != -1) {
        return fo->size;
    }

    zhttpd_t *httpd = fo->httpd;
    zmmap_reader_t reader;
    if (zmmap_reader_init(&reader, httpd->uploaded_tmp_mime_pathname) < 1){
        return -1;
    }
    zbuf_t *data = zbuf_create(4096);
    if (fo->encoding == 'B') {
        zbase64_decode(reader.data + fo->offset, fo->raw_len, data, 0);
    } else if (fo->encoding == 'Q') {
        zqp_decode_2045(reader.data + fo->offset, fo->raw_len, data);
    } else {
        zbuf_memcpy(data, reader.data + fo->offset, fo->size);
    }
    zmmap_reader_fini(&reader);
    fo->size = zbuf_len(data);
    zfree(data);
    return fo->size;
}

int zhttpd_uploaded_file_save_to(zhttpd_uploaded_file_t *fo, const char *pathname)
{
    zhttpd_t *httpd = fo->httpd;
    zmmap_reader_t reader;
    if (zmmap_reader_init(&reader, httpd->uploaded_tmp_mime_pathname) < 1){
        return -1;
    }
    zbuf_t *data = zbuf_create(4096);
    if (fo->encoding == 'B') {
        zbase64_decode(reader.data + fo->offset, fo->raw_len, data, 0);
    } else if (fo->encoding == 'Q') {
        zqp_decode_2045(reader.data + fo->offset, fo->raw_len, data);
    } else {
        zbuf_memcpy(data, reader.data + fo->offset, fo->size);
    }
    zmmap_reader_fini(&reader);
    int ret = zfile_put_contents(pathname, zbuf_data(data), zbuf_len(data));
    zbuf_free(data);
    return ret;
}

int zhttpd_uploaded_file_get_data(zhttpd_uploaded_file_t *fo, zbuf_t *data)
{
    zhttpd_t *httpd = fo->httpd;
    zbuf_reset(data);
    zmmap_reader_t reader;
    if (zmmap_reader_init(&reader, httpd->uploaded_tmp_mime_pathname) < 1){
        return -1;
    }
    if (fo->encoding == 'B') {
        zbase64_decode(reader.data + fo->offset, fo->raw_len, data, 0);
    } else if (fo->encoding == 'Q') {
        zqp_decode_2045(reader.data + fo->offset, fo->raw_len, data);
    } else {
        zbuf_memcpy(data, reader.data + fo->offset, fo->size);
    }
    zmmap_reader_fini(&reader);
    return fo->size;
}

static void zhttpd_response_200_default(zhttpd_t *httpd, const char *data, int size);
static void zhttpd_response_304_default(zhttpd_t *httpd, const char *etag);
static void zhttpd_response_404_default(zhttpd_t *httpd);
static void zhttpd_response_500_default(zhttpd_t *httpd);
static void zhttpd_response_501_default(zhttpd_t *httpd);

static zhttpd_t * _zhttpd_malloc_struct_general()
{
    zhttpd_t * httpd = (zhttpd_t *)zcalloc(1, sizeof(zhttpd_t));
    httpd->fp = 0;
    httpd->prefix_log_msg = 0;
    httpd->method = zblank_buffer;
    httpd->host = zblank_buffer;
    httpd->port = -1;
    httpd->uri = zblank_buffer;
    httpd->version = zblank_buffer;
    httpd->path = zblank_buffer;

    httpd->request_query_vars = zdict_create();
    httpd->request_post_vars = zdict_create();
    httpd->request_headers = zdict_create();
    httpd->request_cookies = zdict_create();
    httpd->request_uploaded_files = zvector_create(-1);
    httpd->uploaded_tmp_mime_pathname = 0;

    httpd->version_code = -1;
    httpd->stop = 0;
    httpd->exception = 0;
    httpd->first = 1;
    httpd->unsupported_cmd = 0;
    httpd->request_keep_alive = 0;
    httpd->response_initialization = 0;
    httpd->response_content_type = 0;
    httpd->ssl_mode = 0;
    httpd->enable_form_data = 0;
    httpd->request_gzip = 0;
    httpd->request_deflate = 0;

    httpd->keep_alive_timeout = -1;
    httpd->request_header_timeout = -1;
    httpd->read_wait_timeout = -1;
    httpd->write_wait_timeout = -1;
    httpd->max_length_for_post = -1;
    httpd->tmp_path_for_post = zblank_buffer;
    httpd->gzip_file_suffix[0] = 0;

    httpd->handler_304 = zhttpd_response_304_default;
    httpd->handler_404 = zhttpd_response_404_default;
    httpd->handler_500 = zhttpd_response_500_default;
    httpd->handler_501 = zhttpd_response_501_default;
    httpd->handler_200 = zhttpd_response_200_default;
    httpd->handler = zhttpd_response_404;
    httpd->handler_HEAD = zhttpd_response_501;
    httpd->handler_OPTIONS = zhttpd_response_501;
    httpd->handler_DELETE = zhttpd_response_501;
    httpd->handler_TRACE = zhttpd_response_501;

    return httpd;
}

zhttpd_t *zhttpd_open_fd(int sock)
{
    zhttpd_t * httpd = _zhttpd_malloc_struct_general();
    httpd->fp = zstream_open_fd(sock);
    zstream_set_read_wait_timeout(httpd->fp, httpd->read_wait_timeout);
    zstream_set_write_wait_timeout(httpd->fp, httpd->write_wait_timeout);
    httpd->ssl_mode = 0;
    return httpd;
}

zhttpd_t *zhttpd_open_ssl(SSL *ssl)
{
    zhttpd_t * httpd = _zhttpd_malloc_struct_general();
    httpd->fp = zstream_open_ssl(ssl);
    zstream_set_read_wait_timeout(httpd->fp, httpd->read_wait_timeout);
    zstream_set_write_wait_timeout(httpd->fp, httpd->write_wait_timeout);
    httpd->ssl_mode = 1;
    return httpd;
}

void zhttpd_close(zhttpd_t *httpd, zbool_t close_fd_and_release_ssl)
{
    zhttpd_response_flush(httpd);
    zstream_close(httpd->fp, close_fd_and_release_ssl);
    zbuf_free(httpd->prefix_log_msg);
    zfree(httpd->method);
    zfree(httpd->host);
    zfree(httpd->path);

    zdict_free(httpd->request_query_vars);
    zdict_free(httpd->request_post_vars);
    zdict_free(httpd->request_headers);
    zdict_free(httpd->request_cookies);

    zfree(httpd->tmp_path_for_post);

    ZVECTOR_WALK_BEGIN(httpd->request_uploaded_files, zhttpd_uploaded_file_t *, fo) {
        zfree(fo->name);
        zfree(fo->pathname);
        zfree(fo);
    } ZVECTOR_WALK_END;
    zvector_free(httpd->request_uploaded_files);

    if (!zempty(httpd->uploaded_tmp_mime_pathname)){
        zunlink(httpd->uploaded_tmp_mime_pathname);
        zfree(httpd->uploaded_tmp_mime_pathname);
    }

    zfree(httpd);
}

void zhttpd_set_context(zhttpd_t *httpd, const void *context)
{
    httpd->context = (void *)context;
}

void *zhttpd_get_context(zhttpd_t *httpd)
{
    return httpd->context;
}

void zhttpd_set_HEAD_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd))
{
    httpd->handler_HEAD = (handler?handler:zhttpd_response_501);
}

void zhttpd_set_OPTIONS_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd))
{
    httpd->handler_OPTIONS = (handler?handler:zhttpd_response_501);
}

void zhttpd_set_DELETE_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd))
{
    httpd->handler_DELETE = (handler?handler:zhttpd_response_501);
}

void zhttpd_set_TRACE_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd))
{
    httpd->handler_TRACE = (handler?handler:zhttpd_response_501);
}

void zhttpd_set_PATCH_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd))
{
    httpd->handler_TRACE = (handler?handler:zhttpd_response_501);
}

void zhttpd_set_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd))
{
    httpd->handler = (handler?handler:zhttpd_response_404);
}

void zhttpd_set_keep_alive_timeout(zhttpd_t *httpd, int timeout)
{
    httpd->keep_alive_timeout = timeout;
}

void zhttpd_set_request_header_timeout(zhttpd_t *httpd, int timeout)
{
    httpd->request_header_timeout = timeout;
}

void zhttpd_set_read_wait_timeout(zhttpd_t *httpd, int read_wait_timeout)
{
    httpd->read_wait_timeout = read_wait_timeout;
    zstream_set_read_wait_timeout(httpd->fp, httpd->read_wait_timeout);
}

void zhttpd_set_write_wait_timeout(zhttpd_t *httpd, int write_wait_timeout)
{
    httpd->write_wait_timeout = write_wait_timeout;
    zstream_set_write_wait_timeout(httpd->fp, httpd->write_wait_timeout);
}

void zhttpd_set_max_length_for_post(zhttpd_t *httpd, int max_length)
{
    httpd->max_length_for_post = max_length;
}

void zhttpd_set_tmp_path_for_post(zhttpd_t *httpd, const char *tmp_path)
{
    zfree(httpd->tmp_path_for_post);
    httpd->tmp_path_for_post = zstrdup(tmp_path);
}

void zhttpd_set_gzip_file_suffix(zhttpd_t *httpd, const char *suffix)
{
    httpd->gzip_file_suffix[0] = 0;
    if (suffix) {
        int len = strlen(suffix);
        if (len > 7) {
            zfatal("FATAL zhttpd_set_gzip_file_suffix: %s'length > 7", suffix);
        }
        strcpy(httpd->gzip_file_suffix, suffix);
    }
}

void zhttpd_enable_form_data(zhttpd_t *httpd)
{
    httpd->enable_form_data = 1;
}

void zhttpd_run(zhttpd_t *httpd)
{
    zbuf_t *linebuf = zbuf_create(1024);
    while(1) {
        zhttpd_loop_clear(httpd);
        zhttpd_request_header_do(httpd, linebuf);
        zhttpd_request_data_do(httpd, linebuf);
        if (httpd->exception || httpd->stop) {
            break;
        }
        httpd->handler_protocal(httpd);
        zhttpd_response_flush(httpd);
        if (httpd->exception || httpd->stop || (httpd->request_keep_alive==0)) {
            break;
        }
    }
    zbuf_free(linebuf);
    return;
}

void zhttpd_set_stop(zhttpd_t *httpd)
{
    httpd->stop = 1;
}

const char *zhttpd_request_get_method(zhttpd_t *httpd)
{
    return httpd->method;
}

const char *zhttpd_request_get_host(zhttpd_t *httpd)
{
    return httpd->host;
}

const char *zhttpd_request_get_uri(zhttpd_t *httpd)
{
    return httpd->uri;
}

const char *zhttpd_request_get_path(zhttpd_t *httpd)
{
    return httpd->path;
}

const char *zhttpd_request_get_version(zhttpd_t *httpd)
{
    return httpd->version;
}

int zhttpd_request_get_version_code(zhttpd_t *httpd)
{
    if (httpd->version_code == -1) {
        if (strstr(httpd->version, "1.0")) {
            httpd->version_code = 0;
        } else {
            httpd->version_code = 1;
        }
    }
    return httpd->version_code;
}

zbool_t zhttpd_request_is_gzip(zhttpd_t *httpd)
{
    if (httpd->request_gzip == 0) {
        httpd->request_gzip = (strcasestr(zdict_get_str(httpd->request_headers,"accept-encoding", ""), "gzip")?1:2);
    }
    return ((httpd->request_gzip==1)?1:0);
}

zbool_t zhttpd_request_is_deflate(zhttpd_t *httpd)
{
    if (httpd->request_deflate == 0) {
        httpd->request_deflate = (strcasestr(zdict_get_str(httpd->request_headers,"accept-encoding", ""), "deflate")?1:2);
    }
    return ((httpd->request_deflate==1)?1:0);
}

long zhttpd_request_get_content_length(zhttpd_t *httpd)
{
    return httpd->request_content_length;
}

const zdict_t *zhttpd_request_get_headers(zhttpd_t *httpd)
{
    return httpd->request_headers;
}

const zdict_t *zhttpd_request_get_query_vars(zhttpd_t *httpd)
{
    return httpd->request_query_vars;
}

const zdict_t *zhttpd_request_get_post_vars(zhttpd_t *httpd)
{
    return httpd->request_post_vars;
}

const zdict_t *zhttpd_request_get_cookies(zhttpd_t *httpd)
{
    return httpd->request_cookies;
}

const zvector_t *zhttpd_request_get_uploaded_files(zhttpd_t *httpd)
{
    return httpd->request_uploaded_files;
}

/* response completely */
void zhttpd_set_304_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd, const char *etag))
{
    httpd->handler_304 = (handler?handler:zhttpd_response_304_default);
}

void zhttpd_set_404_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd))
{
    httpd->handler_404 = (handler?handler:zhttpd_response_404_default);
}

void zhttpd_set_500_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd))
{
    httpd->handler_500 = (handler?handler:zhttpd_response_500_default);
}

void zhttpd_set_501_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd))
{
    httpd->handler_501 = (handler?handler:zhttpd_response_501_default);
}

void zhttpd_set_200_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd, const char *data, int size))
{
    httpd->handler_200 = (handler?handler:zhttpd_response_200_default);
}

static void zhttpd_response_200_default(zhttpd_t *httpd, const char *data, int size)
{
    if (size < 0) {
        size = strlen(data);
    }
    zhttpd_show_log(httpd, "200 %d", size);
    zhttpd_response_header_content_length(httpd, size);
    if (httpd->request_keep_alive) {
        zhttpd_response_header(httpd, "Connection", "keep-alive");
    }
    zhttpd_response_header_over(httpd);
    if (size > 0) {
        zstream_write(httpd->fp, data, size);
    }
    zhttpd_response_flush(httpd);
}

void zhttpd_response_200(zhttpd_t *httpd, const char *data, int size)
{
    httpd->handler_200(httpd, data, size);
}

static void zhttpd_response_304_default(zhttpd_t *httpd, const char *etag)
{
    zhttpd_show_log(httpd, "304 -");
    zstream_puts_const(httpd->fp, "HTTP/1.1 304 Not Modified\r\nServer: LIBZC HTTPD\r\nEtag: ");
    zstream_puts(httpd->fp, etag);
    zstream_write(httpd->fp, "\r\n\r\n", 4);
    zhttpd_response_flush(httpd);
}

void zhttpd_response_304(zhttpd_t *httpd, const char *etag)
{
    httpd->handler_304(httpd, etag);
}

static void zhttpd_response_404_default(zhttpd_t *httpd)
{
    zhttpd_show_log(httpd, "404 -");
    zstream_puts(httpd->fp, httpd->version);
    zstream_puts(httpd->fp, " 404 Not Found\r\nServer: LIBZC HTTPD\r\nContent-Type: text/html\r\n");
    if (httpd->request_keep_alive) {
        zstream_puts(httpd->fp, "Connection: keep-alive\r\n");
    } else {
        zstream_puts(httpd->fp, "Connection: close\r\n");
    }
    zstream_puts(httpd->fp, "Content-Length: 39\r\n\r\n404 Not Found. <A HREF=\"/\">homepage</A>");
}

void zhttpd_response_404(zhttpd_t *httpd)
{
    httpd->handler_404(httpd);
}

static void zhttpd_response_500_default(zhttpd_t *httpd)
{
    zhttpd_show_log(httpd, "500 -");
    zstream_puts(httpd->fp, httpd->version);
    zstream_puts(httpd->fp, " 500 Error\r\nServer: LIBZC HTTPD\r\nContent-Type: text/html\r\n");
    if (httpd->request_keep_alive) {
        zstream_puts(httpd->fp, "Connection: keep-alive\r\n");
    } else {
        zstream_puts(httpd->fp, "Connection: close\r\n");
    }
    zstream_puts(httpd->fp, "Content-Length: 51\r\n\r\n500 Internal Server Error. <A HREF=\"/\">homepage</A>");
}

void zhttpd_response_500(zhttpd_t *httpd)
{
    httpd->handler_500(httpd);
}

static void zhttpd_response_501_default(zhttpd_t *httpd)
{
    zhttpd_show_log(httpd, "501 -");
    char output[] = " 501 Not implemented\r\nServer: LIBZC HTTPD\r\nConnection: close\r\n"
        "Content-Length: 19\r\n\r\n501 Not implemented";
    zstream_puts(httpd->fp, httpd->version);
    zstream_write(httpd->fp, output, sizeof(output) - 1);
}

void zhttpd_response_501(zhttpd_t *httpd)
{
    httpd->handler_501(httpd);
}

void zhttpd_response_header_initialization(zhttpd_t *httpd, const char *version, const char *status)
{
    char initialization[128];
    sprintf(initialization, "%s %s", version?version:httpd->version, status);
    zstream_puts(httpd->fp, initialization);
    zstream_puts_const(httpd->fp, "\r\n");
    httpd->response_initialization = 1;
}

void zhttpd_response_header(zhttpd_t *httpd, const char *name, const char *value)
{
    if (!httpd->response_initialization) {
        zhttpd_response_header_initialization(httpd, httpd->version, "200 OK");
    }
    zstream_puts(httpd->fp, name);
    zstream_puts_const(httpd->fp, ": ");
    zstream_puts(httpd->fp, value);
    zstream_puts_const(httpd->fp, "\r\n");
    if (!httpd->response_content_type) {
        if (ZSTR_CASE_EQ(name, "Content-Type")) {
            httpd->response_content_type = 1;
        }
    }
}

void zhttpd_response_header_date(zhttpd_t *httpd, const char *name, long value)
{
    char buf[zvar_rfc1123_date_string_size];
    zbuild_rfc1123_date_string(value, buf);
    zhttpd_response_header(httpd, name, buf);
}

void zhttpd_response_header_content_type(zhttpd_t *httpd, const char *value, const char *charset)
{
    zbuf_t *val = zbuf_create(1024);
    zbuf_puts(val, value);
    if (zempty(charset)) {
        charset = "UTF-8";
    }
    zbuf_puts(val, "; charset=");
    zbuf_puts(val, charset);
    zhttpd_response_header(httpd, "Content-Type", zbuf_data(val));
    zbuf_free(val);
}

void zhttpd_response_header_content_length(zhttpd_t *httpd, long length)
{
    char val[32];
    if (length > -1) {
        sprintf(val, "%ld", length);
        zhttpd_response_header(httpd, "Content-Length", val);
    }
}

void zhttpd_response_header_set_cookie(zhttpd_t *httpd, const char *name, const char *value, long expires, const char *path, const char *domain, zbool_t secure, zbool_t httponly)
{
    zbuf_t *result = zbuf_create(1024);
    zhttp_cookie_build_item(name, value, expires, path, domain, secure, httponly, result);
    zhttpd_response_header(httpd, "Set-Cookie", zbuf_data(result));
    zbuf_free(result);
}

void zhttpd_response_header_unset_cookie(zhttpd_t *httpd, const char *name)
{
    return zhttpd_response_header_set_cookie(httpd, name, 0, 0, 0, 0, 0, 0);
}

void zhttpd_response_header_over(zhttpd_t *httpd)
{
    if (!httpd->response_content_type) {
        zhttpd_response_header(httpd, "Content-Type", "text/html");
    }
    zstream_puts_const(httpd->fp, "\r\n");
}

void zhttpd_response_write(zhttpd_t *httpd, const void *data, int len)
{
    zstream_write(httpd->fp, data, len);
}

void zhttpd_response_puts(zhttpd_t *httpd, const char *data)
{
    zstream_write(httpd->fp, data, strlen(data));
}

void zhttpd_response_append(zhttpd_t *httpd, const zbuf_t *bf)
{
    zstream_write(httpd->fp, zbuf_data(bf), zbuf_len(bf));
}

void zhttpd_response_printf_1024(zhttpd_t *httpd, const char *format, ...)
{
    va_list ap;
    char buf[1024+1];
    int len;

    if (format == 0) {
        format = "";
    }
    va_start(ap, format);
    len = vsnprintf(buf, 1024, format, ap);
    len = ((len<1024)?len:(1024-1));
    va_end(ap);
    zstream_write(httpd->fp, buf, len);
}

void zhttpd_response_flush(zhttpd_t *httpd)
{
    if (httpd->stop || httpd->exception) {
        return;
    }
    if (zstream_flush(httpd->fp) < 0) {
        httpd->exception = 0;
        zhttpd_show_log(httpd, "exception write");
    }
}

zstream_t *zhttpd_get_stream(zhttpd_t *httpd)
{
    return httpd->fp;
}

static void zhttpd_loop_clear(zhttpd_t *httpd)
{

#define ___FR(m) zfree(m); m=zblank_buffer;
    ___FR(httpd->method);
    ___FR(httpd->host);
    ___FR(httpd->path);
#undef ___FR
    httpd->port = -1;

    httpd->version_code = -1;
    httpd->stop = 0;
    httpd->request_content_length = -1;
    zdict_reset(httpd->request_query_vars);
    zdict_reset(httpd->request_post_vars);
    zdict_reset(httpd->request_headers);
    zdict_reset(httpd->request_cookies);

    ZVECTOR_WALK_BEGIN(httpd->request_uploaded_files, zhttpd_uploaded_file_t *, fo) {
        zfree(fo->name);
        zfree(fo->pathname);
        zfree(fo);
    } ZVECTOR_WALK_END;
    zvector_reset(httpd->request_uploaded_files);

    if (!zempty(httpd->uploaded_tmp_mime_pathname)){
        zunlink(httpd->uploaded_tmp_mime_pathname);
        zfree(httpd->uploaded_tmp_mime_pathname);
        httpd->uploaded_tmp_mime_pathname = 0;
    }

    httpd->unsupported_cmd = 0;
    httpd->request_keep_alive = 0;
    httpd->response_initialization = 0;
    httpd->response_content_type = 0;
}

static void zhttpd_request_header_do(zhttpd_t * httpd, zbuf_t *linebuf)
{
    char *p, *ps;
    int ret, llen, first = httpd->first;

    /* read first header line */
    if (httpd->first) {
        httpd->first = 0;
        ret = zstream_timed_read_wait(httpd->fp, httpd->request_header_timeout);
        if (ret < 1) {
            zhttpd_show_log(httpd, "exception wait read");
            httpd->exception = 1;
            return;
        }
    } else {
        ret = zstream_timed_read_wait(httpd->fp, httpd->keep_alive_timeout);
        if (ret < 1) {
            httpd->exception = 1;
            return;
        }
    }
    zstream_timed_read_wait(httpd->fp, httpd->request_header_timeout);
    ret = zstream_gets(httpd->fp, linebuf, zvar_httpd_header_line_max_size);
    if (ret < 0) {
        zhttpd_show_log(httpd, "exception read banner line");
        httpd->exception = 1;
        return;
    }
    if (ret == 0) {
        if (first) {
            zhttpd_show_log(httpd, "exception read banner line");
            httpd->exception = 1;
            return;
        }
        httpd->stop = 1;
        return;
    }
    zbuf_trim_right_rn(linebuf);
    llen = zbuf_len(linebuf);
    httpd->method = zmemdupnull(zbuf_data(linebuf), llen);
    ps = httpd->method;
    p = strchr(ps, ' ');
    if (!p) {
        zhttpd_show_log(httpd, "exception banner no blank");
        httpd->exception = 1;
        return;
    }
    *p = 0;
    zstr_toupper(ps);
    if (ZSTR_EQ(ps, "GET")) {
        httpd->handler_protocal = httpd->handler;
    } else if (ZSTR_EQ(ps, "POST")) {
        httpd->handler_protocal = httpd->handler;
    } else if (ZSTR_EQ(ps, "OPTIONS")) {
        httpd->handler_protocal = httpd->handler_OPTIONS;
    } else if (ZSTR_EQ(ps, "HEAD")) {
        httpd->handler_protocal = httpd->handler_HEAD;
    } else if (ZSTR_EQ(ps, "PUT")) {
        httpd->handler_protocal = httpd->handler;
    } else if (ZSTR_EQ(ps, "TRACE")) {
        httpd->handler_protocal = httpd->handler_TRACE;
    } else if (ZSTR_EQ(ps, "DELETE")) {
        httpd->handler_protocal = httpd->handler_DELETE;
    } else if (ZSTR_EQ(ps, "PATCH")) {
        httpd->handler_protocal = httpd->handler_PATCH;
    } else {
        zhttpd_show_log(httpd, "exception unknown CMD");
        httpd->exception = 1;
        return;
    }
    llen -= (p - ps) + 1;
    ps = httpd->uri = p + 1;

    if (llen < 10) {
        zhttpd_show_log(httpd, "exception banner line too short");
        httpd->exception = 1;
        return;
    }
    p += llen;
    for (;p > ps; p--) {
        if (*p ==' ') {
            break;
        }
    }
    if (ps == p) {
        zhttpd_show_log(httpd, "exception banner line no version token");
        httpd->exception = 1;
        return;
    }
    *p = 0;
    httpd->version = p + 1;
    zstr_toupper(httpd->version);

    ps = httpd->uri;
    p = strchr(ps, '?');
    if (!p) {
        httpd->path = zstrdup(ps);
    } else {
        httpd->path = zmemdupnull(ps, p - ps);
        ps = p + 1;
        p = strchr(ps, '#');
        if (p) {
            *p = 0;
        }
        zurl_query_parse(ps, httpd->request_query_vars);
        if (p) {
            *p = '#';
        }
    }

    /* read other header lines */
    while(1) {
        if ((zstream_gets(httpd->fp, linebuf, zvar_httpd_header_line_max_size)) < 1) {
            zhttpd_show_log(httpd, "exception read header line");
            httpd->exception = 1;
            return;
        }
        if (zbuf_data(linebuf)[zbuf_len(linebuf)-1] != '\n') {
            zhttpd_show_log(httpd, "exception header line too long");
            httpd->exception = 1;
            return;
        }
        zbuf_trim_right_rn(linebuf);
        if (zbuf_len(linebuf) == 0) {
            break;
        }
        ps = zbuf_data(linebuf);
        p = strchr(ps, ':');
        if (!p) {
            continue;
        }
        *p = 0;
        zstr_tolower(ps);
        ps = p + 1;
        while (*ps == ' ') {
            ps++;
        }
        zdict_update_string(httpd->request_headers, zbuf_data(linebuf), ps, -1);
        p = zbuf_data(linebuf);
        if (p[0] == 'h') {
            if (!strcmp(p, "host")) {
                p = strchr(ps, ':');
                if (p) {
                    httpd->host = zstrndup(ps, p-ps);
                    httpd->port = atoi(p+1);
                } else {
                    httpd->host = zstrdup(ps);
                    httpd->port = -1;
                }
                zstr_tolower(httpd->host);
            }
        } else if (p[0] == 'c') {
            if (!strcmp(p, "content-length")) {
                httpd->request_content_length = atoi(ps);
            } else if (!strcmp(p, "cookie")) {
                zhttp_cookie_parse(ps, httpd->request_cookies);
            } else if (!strcmp(p, "connection")) {
                if (strcasestr(ps, "keep-alive")) {
                    httpd->request_keep_alive = 1;
                }
            }
        }
    }
}

static char *_zhttpd_request_data_do_save_tmpfile(zhttpd_t *httpd, zbuf_t *linebuf, char *content_type_raw)
{
    if (zempty(httpd->tmp_path_for_post)) {
        zbuf_memcpy(linebuf, "/tmp/", 5);
    } else {
        zbuf_strcpy(linebuf, httpd->tmp_path_for_post);
    }
    if (zbuf_data(linebuf)[zbuf_len(linebuf)-1] != '/') {
        zbuf_put(linebuf, '/');
    }
    zbuf_need_space(linebuf, zvar_unique_id_size);
    zbuild_unique_id(zbuf_data(linebuf)+zbuf_len(linebuf));
    char *data_pathname = zstrdup(zbuf_data(linebuf));

    zstream_t *tmp_fp = zstream_open_file(data_pathname, "w+");
    if (!tmp_fp) {
        zhttpd_show_log(httpd, "exception open temp file");
        httpd->exception = 1;
        zfree(data_pathname);
        return 0;
    }
    zstream_puts_const(tmp_fp, "Content-Type: "); 
    zstream_puts(tmp_fp, content_type_raw); 
    zstream_write(tmp_fp, "\r\n\r\n", 4);

    int left = httpd->request_content_length;
    while(left > 0) {
        int rlen = left;
        if (rlen > 4096) {
            rlen = 4096;
        }
        rlen = zstream_readn(httpd->fp, linebuf, rlen);
        if (rlen < 1) {
            zhttpd_show_log(httpd, "exception read body");
            httpd->exception = 1;
            break;
        }
        zstream_write(tmp_fp, zbuf_data(linebuf), zbuf_len(linebuf));
        left -= rlen;
    }
    if (httpd->exception == 0) {
        if (zstream_flush(tmp_fp) < 0) {
            zhttpd_show_log(httpd, "exception write");
            httpd->exception = 1;
        }
    }
    zstream_close(tmp_fp, 1);
    if (httpd->exception) {
        zunlink(data_pathname);
        zfree(data_pathname);
        return 0;
    }
    return data_pathname;
}

static zbool_t _zhttpd_request_data_do_prepare(zhttpd_t *httpd)
{
    if (httpd->exception || httpd->stop || zstream_is_exception(httpd->fp)) {
        return 0;
    }
    if (httpd->request_content_length < 1) {
        return 0;
    }
    if ((httpd->max_length_for_post > 0) && (httpd->request_content_length > httpd->max_length_for_post)) {
        zhttpd_show_log(httpd, "exception Content-Length>max_length_for_post, zhttpd_set_max_length_for_post");
        httpd->exception = 1;
        return 0;
    }

    return 1;
}

static void _zhttpd_request_data_do_x_www_form_urlencoded(zhttpd_t *httpd, zbuf_t *linebuf)
{
    /* FIXME too long */
    if (zstream_readn(httpd->fp, linebuf, httpd->request_content_length) < httpd->request_content_length) {
        zhttpd_show_log(httpd, "exception read");
        httpd->exception = 1;
        return;
    }
    zurl_query_parse(zbuf_data(linebuf), httpd->request_post_vars);
}

static void _zhttpd_request_data_do_disabled_form_data(zhttpd_t *httpd)
{
    char output[] = " 501 Not implemented\r\nServer: LIBZC HTTPD\r\nConnection: close\r\n"
        "Content-Length: 95\r\n\r\n"
        "501 Not implemented, unsupport multipart/form-data<br>(for developers: zhttpd_enable_form_data)";
    zstream_puts(httpd->fp, httpd->version);
    zstream_write(httpd->fp, output, sizeof(output) - 1);
    zhttpd_response_flush(httpd);
}

static void _zhttpd_uploaded_dump_file(zhttpd_t *httpd, zmime_t *mime, zbuf_t *linebuf, zbuf_t *tmpbf, const char *name, const char *pathname)
{
    int wlen = 0, raw_len = 0;
    const char *encoding = zmime_get_encoding(mime);
    raw_len = zmime_get_body_len(mime);
    if (zempty(encoding)) {
        wlen = raw_len;
    } else {
        wlen = -1;
    }

    zhttpd_uploaded_file_t *fo = (zhttpd_uploaded_file_t *)zcalloc(1, sizeof(zhttpd_uploaded_file_t));
    fo->httpd = httpd;
    fo->name = zstrdup(name);
    fo->pathname = zstrdup(pathname);
    fo->size = wlen;
    fo->offset = zmime_get_body_offset(mime);
    fo->raw_len = raw_len;
    if (!strcmp(encoding, "base64")) {
        fo->encoding = 'B';
    } else if (!strcmp(encoding, "quoted-printable")) {
        fo->encoding = 'Q';
    } else {
        fo->encoding = 0;
    }
    zvector_push(httpd->request_uploaded_files, fo);
}

static void _zhttpd_request_data_do_form_data_one_mime(zhttpd_t *httpd, zmime_t *mime, zbuf_t *linebuf)
{
    const char *disposition = zmime_get_disposition(mime);
    if (strncasecmp(disposition, "form-data", 9)) {
        return;
    }
    zbuf_reset(linebuf);
    if (zmime_get_header_line_value(mime, "content-disposition", linebuf, 0) < 1) {
        return;
    }
    zbuf_t *tmp_bf = zbuf_create(32);
    zdict_t *params = zdict_create();
    zmime_header_line_get_params(zbuf_data(linebuf), zbuf_len(linebuf), tmp_bf, params);
    zbuf_reset(tmp_bf);
    char *name = zdict_get_str(params, "name", "");
    if (!zempty(name)) {
        zstr_tolower(name);
    }
    const char *ctype = zmime_get_type(mime);
    if (strncmp(ctype, "multipart/", 10)) {
        char *pathname = zdict_get_str(params, "filename", "");
        if (zempty(pathname)) {
            if (!zempty(name)) {
                zmime_get_decoded_content(mime, tmp_bf);
                zdict_update(httpd->request_post_vars, name, tmp_bf);
            }
        } else {
            _zhttpd_uploaded_dump_file(httpd, mime, linebuf, tmp_bf, name, pathname);
        }
    } else {
        name = zstrdup(name);
        for (zmime_t *child = zmime_child(mime); child; child = zmime_next(child)) {
            ctype = zmime_get_type(child);
            if (!strncasecmp(ctype, "multipart/", 10)) {
                continue;
            }

            zbuf_reset(linebuf);
            zbuf_reset(tmp_bf);
            zdict_reset(params);
            if (zmime_get_header_line_value(child, "content-disposition", linebuf, 0) < 1) {
                /* continue; */
            }
            zmime_header_line_get_params(zbuf_data(linebuf), zbuf_len(linebuf), tmp_bf, params);
            zbuf_reset(tmp_bf);
            char *pathname = zdict_get_str(params, "filename", "");
            _zhttpd_uploaded_dump_file(httpd, child, linebuf, tmp_bf, name, pathname);
            if (httpd->exception) {
                break;
            }
        }
        zfree(name);
    }
    zbuf_free(tmp_bf);
    zdict_free(params);
}

static void _zhttpd_request_data_do_form_data(zhttpd_t *httpd, zbuf_t *linebuf, char *content_type_raw)
{
    if (httpd->enable_form_data == 0) {
        _zhttpd_request_data_do_disabled_form_data(httpd);
        return;
    }

    char *data_pathname = _zhttpd_request_data_do_save_tmpfile(httpd, linebuf, content_type_raw);
    zmail_t *mime_parser = 0;
    if (data_pathname) {
        mime_parser = zmail_create_parser_from_pathname(data_pathname, "");
    }
    if (!mime_parser) {
        zhttpd_show_log(httpd, "exception format error of from_data");
        httpd->exception = 1;
        goto over;
    }

    const zvector_t *mvec = zmail_get_all_mimes(mime_parser);
    ZVECTOR_WALK_BEGIN(mvec, zmime_t *, mime) {
        if (httpd->exception == 0) {
            _zhttpd_request_data_do_form_data_one_mime(httpd, mime, linebuf);
        }
    } ZVECTOR_WALK_END;

over:
    if (mime_parser) {
        zmail_free(mime_parser);
    }
    httpd->uploaded_tmp_mime_pathname = data_pathname;
}

static void zhttpd_request_data_do(zhttpd_t *httpd, zbuf_t *linebuf)
{
    if (_zhttpd_request_data_do_prepare(httpd) == 0) {
        return;
    }

    char *p = zdict_get_str(httpd->request_headers,"content-type", 0);

    if (!strncasecmp(p, "application/x-www-form-urlencoded", 33)) {
        _zhttpd_request_data_do_x_www_form_urlencoded(httpd, linebuf);
        return;
    }

    if (strncasecmp(p, "multipart/form-data", 19)) {
        return;
    }
    
    _zhttpd_request_data_do_form_data(httpd, linebuf, p);
}

/* log */
const char *zhttpd_get_prefix_log_msg(zhttpd_t *httpd)
{
    if (httpd->prefix_log_msg == 0) {
        httpd->prefix_log_msg = zbuf_create(512);
    }
    const char *ps;
    zbuf_t *logbf = httpd->prefix_log_msg;
    zbuf_reset(logbf);

    ps = zhttpd_request_get_method(httpd);
    if (zempty(ps)) {
        ps = "NIL";
    }
    zbuf_strcat(logbf, ps);

    ps = zhttpd_request_get_uri(httpd);
    if (zempty(ps)) {
        zbuf_strcat(logbf, " NIL");
    } else {
        zbuf_strcat(logbf, " \"");
        zbuf_strncat(logbf, ps, 512);
        zbuf_strcat(logbf, "\"");
    }

    int ip, port = 0;
    char ipstr[18];
    if (zget_peername(zstream_get_fd(zhttpd_get_stream(httpd)), &ip, &port) > 0)  {
        zget_ipstring(ip, ipstr);
    } else {
        ipstr[0] = '0';
        ipstr[1] = 0;
    }
    zbuf_printf_1024(logbf, " %s:%d%s", ipstr, port, (httpd->ssl_mode?"/ssl":""));

    return zbuf_data(logbf);
}

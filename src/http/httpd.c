/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-04-05
 * ================================
 */

#include "zc.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

struct zhttpd_uploaded_file_t {
    zhttpd_t *httpd;
    char *name;
    char *filename;
    int encoding;
    int raw_len;
    int offset;
    int size;
};

zbool_t zvar_httpd_debug = 0;
zbool_t zvar_httpd_no_cache = 0;
static const int zvar_httpd_header_line_max_size = 10240;

struct zhttpd_t {
    zstream_t *fp;
    char *method;
    char *host;
    char *uri;
    char *version;
    char *path;
    int port;
    int request_content_length;
    zdict_t *request_query_vars;
    zdict_t *request_post_vars;
    zdict_t *request_headers;
    zdict_t *request_cookies;
    zvector_t *request_uploaded_files; /* <zhttpd_uploaded_file_t *> */
    char *uploaded_tmp_mime_filename;
    /* */
    unsigned int stop:1;
    unsigned int exception:1;
    unsigned int unsupported_cmd:1;
    unsigned int request_keep_alive:1;
    unsigned int response_initialization:1;
    unsigned int response_content_type:1;
    unsigned int ssl_mode:1;
    unsigned int enable_form_data:1;
    unsigned int request_gzip:2;
    unsigned int request_deflate:2;
    /* */
    int response_max_age;
    int response_expires;
    int keep_alive_timeout;
    int request_header_timeout;
    int max_length_for_post;
    char *tmp_path_for_post;
    char gzip_file_suffix[8];
    void (*handler_304)(zhttpd_t * httpd, const char *etag);
    void (*handler_404)(zhttpd_t * httpd);
    void (*handler_500)(zhttpd_t * httpd);
    void (*handler_200)(zhttpd_t * httpd, const char *data, int size);
    void (*handler)(zhttpd_t * httpd);
    void *context;
};

static void zhttpd_loop_clear(zhttpd_t *httpd);
static void zhttpd_request_header_do(zhttpd_t * httpd, int first, zbuf_t *linebuf);
static void zhttpd_request_data_do(zhttpd_t *httpd, zbuf_t *linebuf);

const char *zhttpd_uploaded_file_get_name(zhttpd_uploaded_file_t *fo)
{
    return fo->name;
}

const char *zhttpd_uploaded_file_get_filename(zhttpd_uploaded_file_t *fo)
{
    return fo->filename;
}

int zhttpd_uploaded_file_get_size(zhttpd_uploaded_file_t *fo)
{
    if (fo->size != -1) {
        return fo->size;
    }

    zhttpd_t *httpd = fo->httpd;
    zmmap_reader_t reader;
    if (zmmap_reader_init(&reader, httpd->uploaded_tmp_mime_filename) < 1){
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

int zhttpd_uploaded_file_save_to(zhttpd_uploaded_file_t *fo, const char *filename)
{
    zhttpd_t *httpd = fo->httpd;
    zmmap_reader_t reader;
    if (zmmap_reader_init(&reader, httpd->uploaded_tmp_mime_filename) < 1){
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
    int ret = zfile_put_contents(filename, zbuf_data(data), zbuf_len(data));
    zbuf_free(data);
    return ret;
}

int zhttpd_uploaded_file_get_data(zhttpd_uploaded_file_t *fo, zbuf_t *data)
{
    zhttpd_t *httpd = fo->httpd;
    zbuf_reset(data);
    zmmap_reader_t reader;
    if (zmmap_reader_init(&reader, httpd->uploaded_tmp_mime_filename) < 1){
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

static zhttpd_t * _zhttpd_malloc_struct_general()
{
    zhttpd_t * httpd = (zhttpd_t *)zcalloc(1, sizeof(zhttpd_t));
    httpd->fp = 0;
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
    httpd->uploaded_tmp_mime_filename = 0;

    httpd->stop = 0;
    httpd->exception = 0;
    httpd->unsupported_cmd = 0;
    httpd->request_keep_alive = 0;
    httpd->response_initialization = 0;
    httpd->response_content_type = 0;
    httpd->ssl_mode = 0;
    httpd->enable_form_data = 0;
    httpd->request_gzip = 0;
    httpd->request_deflate = 0;

    httpd->response_max_age = -1;
    httpd->response_expires = -1;
    httpd->keep_alive_timeout = -1;
    httpd->request_header_timeout = -1;
    httpd->max_length_for_post = -1;
    httpd->tmp_path_for_post = zblank_buffer;
    httpd->gzip_file_suffix[0] = 0;

    httpd->handler_304 = zhttpd_response_304;
    httpd->handler_404 = zhttpd_response_404;
    httpd->handler_500 = zhttpd_response_500;
    httpd->handler_200 = zhttpd_response_200;
    httpd->handler = zhttpd_response_404;

    return httpd;
}

zhttpd_t *zhttpd_open_fd(int sock)
{
    zhttpd_t * httpd = _zhttpd_malloc_struct_general();
    httpd->fp = zstream_open_fd(sock);
    httpd->ssl_mode = 0;
    return httpd;
}

zhttpd_t *zhttpd_open_ssl(SSL *ssl)
{
    zhttpd_t * httpd = _zhttpd_malloc_struct_general();
    httpd->fp = zstream_open_ssl(ssl);
    httpd->ssl_mode = 1;
    return httpd;
}

void zhttpd_close(zhttpd_t *httpd, zbool_t close_fd_and_release_ssl)
{
    zhttpd_response_flush(httpd);
    zstream_close(httpd->fp, close_fd_and_release_ssl);
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
        zfree(fo->filename);
        zfree(fo);
    } ZVECTOR_WALK_END;
    zvector_free(httpd->request_uploaded_files);

    if (!zempty(httpd->uploaded_tmp_mime_filename)){
        zunlink(httpd->uploaded_tmp_mime_filename);
        zfree(httpd->uploaded_tmp_mime_filename);
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

void zhttpd_set_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd))
{
    httpd->handler = handler;
}

void zhttpd_set_keep_alive_timeout(zhttpd_t *httpd, int timeout)
{
    httpd->keep_alive_timeout = timeout;
}

void zhttpd_set_request_header_timeout(zhttpd_t *httpd, int timeout)
{
    httpd->request_header_timeout = timeout;
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
            zfatal("zhttpd_set_gzip_file_suffix: %s'length > 7", suffix);
        }
        strcpy(httpd->gzip_file_suffix, suffix);
    }
}

void zhttpd_enable_form_data(zhttpd_t *httpd)
{
    httpd->enable_form_data = 1;
}

void zhttpd_response_file_set_max_age(zhttpd_t *httpd, int left_second)
{
    httpd->response_max_age = left_second;
}

void zhttpd_response_file_set_expires(zhttpd_t *httpd, int left_second)
{
    httpd->response_expires = left_second;
}

void zhttps_set_exception(zhttpd_t *httpd)
{
    httpd->exception = 1;
}

void zhttpd_run(zhttpd_t *httpd)
{
    int first = 1;
    while(1) {
        zhttpd_loop_clear(httpd);
        zbuf_t *linebuf = zbuf_create(1024);
        zhttpd_request_header_do(httpd, first, linebuf);
        first = 0;
        zhttpd_request_data_do(httpd, linebuf);
        zbuf_free(linebuf);
        if (httpd->exception) {
            break;
        }
        httpd->handler(httpd);
        zhttpd_response_flush(httpd);
        if (httpd->stop || httpd->exception || httpd->request_keep_alive) {
            break;
        }
    }
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

zbool_t zhttpd_request_is_gzip(zhttpd_t *httpd)
{
    if (httpd->request_gzip == 0) {
        httpd->request_gzip = (strcasestr(zdict_get_str(httpd->request_headers,"accept-encoding", ""), "gzip")?1:2);
    }
    return (httpd->request_gzip==1);
}

zbool_t zhttpd_request_is_deflate(zhttpd_t *httpd)
{
    if (httpd->request_deflate == 0) {
        httpd->request_deflate = (strcasestr(zdict_get_str(httpd->request_headers,"accept-encoding", ""), "deflate")?1:2);
    }
    return (httpd->request_deflate==1);
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
    httpd->handler_304 = handler;
}

void zhttpd_set_404_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd))
{
    httpd->handler_404 = handler;
}

void zhttpd_set_500_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd))
{
    httpd->handler_500 = handler;
}

void zhttpd_set_200_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd, const char *data, int size))
{
    httpd->handler_200 = handler;
}

static const int response_file_flag_gzip = (1<<0);
static const int response_file_flag_try_gzip = (1<<1);
static const int response_file_flag_regular = (1<<2);
static void _zresponse_file_by_flag(zhttpd_t *httpd, const char *filename, const char *content_type, int flag, zbool_t *catch_missing)
{
    if (catch_missing) {
        *catch_missing = 0;
    }
    if (zempty(content_type)) {
        content_type = zget_mime_type_from_filename(filename, zvar_mime_type_application_cotec_stream);
    }
    int fd;
    struct stat st;
    zbool_t is_gzip = 0;
    if ((flag & response_file_flag_gzip) || (flag & response_file_flag_regular)) {
        while (((fd = open(filename, O_RDONLY)) == -1) && (errno == EINTR)) {
            continue;
        }
        if (fd == -1) {
            if (catch_missing) {
                *catch_missing = 1;
            } else {
                zhttpd_response_404(httpd);
            }
            return;
        }

        if (fstat(fd, &st) == -1) {
            zclose(fd);
            zhttpd_response_500(httpd);
            return;
        }
        if (flag & response_file_flag_gzip) {
            is_gzip = 1;
        }
    } else if (flag & response_file_flag_try_gzip) {
        int times;
        for (times = 0; times < 2; times++) {
            if (times == 0) {
                if (httpd->gzip_file_suffix[0] == 0) {
                    continue;
                }
                if (!strcasestr(zdict_get_str(httpd->request_headers,"accept-encoding", ""), "gzip")) {
                    continue;
                }
                zbuf_t *fn = zbuf_create(128);
                zbuf_strcpy(fn, filename);
                zbuf_put(fn, '.');
                zbuf_puts(fn, httpd->gzip_file_suffix);
                while (((fd = open(zbuf_data(fn), O_RDONLY)) == -1) && (errno == EINTR)) {
                    continue;
                }
                zbuf_free(fn);
                is_gzip = 1;
            } else {
                while (((fd = open(filename, O_RDONLY)) == -1) && (errno == EINTR)) {
                    continue;
                }
                is_gzip = 0;
            }
            if (fd == -1) {
                continue;
            }

            if (fstat(fd, &st) == -1) {
                zclose(fd);
                continue;
            }
            break;
        }
        if (times == 2) {
            if (catch_missing) {
                *catch_missing = 1;
            } else {
                zhttpd_response_404(httpd);
            }
            return;
        }
    } else {
        zhttpd_response_500(httpd);
        return;
    }

    char *rwdata = (char *)zmalloc(4096+1);

    char *old_etag = zdict_get_str(httpd->request_headers,"if-none-match", "");
    char *new_etag = rwdata;
    sprintf(new_etag, "%lx_%lx", st.st_size, st.st_mtime);
    if (!strcmp(old_etag, new_etag)) {
        if (zvar_httpd_no_cache == 0) {
            zhttpd_response_304(httpd, new_etag);
            zclose(fd);
            zfree(rwdata);
            return;
        }
    }

    zhttpd_response_header_content_type(httpd, content_type, 0);
    zhttpd_response_header_content_length(httpd, st.st_size);
    if (zvar_httpd_no_cache == 0) {
        zhttpd_response_header(httpd, "Etag", new_etag);
        zhttpd_response_header_date(httpd, "Last-Modified", st.st_mtime);
        if (httpd->response_max_age > 0) {
            sprintf(rwdata, "max-age=%d", httpd->response_max_age);
            zhttpd_response_header(httpd, "Cache-Control", rwdata);
        }
        if (httpd->response_expires > 0) {
            zhttpd_response_header_date(httpd, "Expires", httpd->response_expires + 1 + time(0));
        }
    }

    if (is_gzip) {
        zhttpd_response_header(httpd, "Content-Encoding", "gzip");
    }

    if (httpd->request_keep_alive) {
        zhttpd_response_header(httpd, "Connection", "keep-alive");
    }
    zhttpd_response_header_over(httpd);

    char *rwline = rwdata;
    long rlen_sum = 0;
    while(rlen_sum < st.st_size) {
        int rlen = st.st_size - rlen_sum;
        if (rlen > 4096) {
            rlen = 4096;
        }
        rlen = read(fd, rwline, rlen);
        if (rlen > 0) {
            rlen_sum += rlen;
            zstream_write(httpd->fp, rwline, rlen);
            if (zstream_is_exception(httpd->fp)) {
                break;
            }
            continue;
        }
        if (rlen == 0) {
            break;
        }
        if (errno == EINTR) {
            if (zvar_proc_stop) {
                zhttpd_set_stop(httpd);
                break;
            }
            continue;
        }
        break;
    }
    zclose(fd);
    zstream_flush(httpd->fp);
    if (rlen_sum != st.st_size) {
        zhttpd_set_stop(httpd);
    } else {
        zstream_flush(httpd->fp);
    }
    zfree(rwdata);
}

void zhttpd_response_file_with_gzip(zhttpd_t *httpd, const char *filename, const char *content_type, zbool_t *catch_missing)
{
    _zresponse_file_by_flag(httpd, filename, content_type, response_file_flag_gzip, catch_missing);
}

void zhttpd_response_file(zhttpd_t *httpd, const char *filename, const char *content_type, zbool_t *catch_missing)
{
    _zresponse_file_by_flag(httpd, filename, content_type, response_file_flag_regular, catch_missing);
}

void zhttpd_response_file_try_gzip(zhttpd_t *httpd, const char *filename, const char *content_type, zbool_t *catch_missing)
{
    _zresponse_file_by_flag(httpd, filename, content_type, response_file_flag_try_gzip, catch_missing);
}

void zhttpd_response_200(zhttpd_t *httpd, const char *data, int size)
{
    zhttpd_response_header_content_length(httpd, size);
    zhttpd_response_header_over(httpd);
    if (size > 0) {
        zstream_write(httpd->fp, data, size);
    }
    zhttpd_response_flush(httpd);
}

void zhttpd_response_404(zhttpd_t *httpd)
{
    char output[] = "HTTP/1.0 404 Not Found\r\n"
        "Server: LIBZC HTTPD\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n"
        "Content-Length: 13\r\n"
        "\r\n"
        "404 Not Found ";
    zstream_write(httpd->fp, output, sizeof(output) - 1);
    zhttpd_response_flush(httpd);
}

void zhttpd_response_304(zhttpd_t *httpd, const char *etag)
{
    zstream_puts_const(httpd->fp, "HTTP/1.1 304 Not Modified\r\nEtag: ");
    zstream_puts(httpd->fp, etag);
    zstream_write(httpd->fp, "\r\n", 2);
    zhttpd_response_flush(httpd);
}

void zhttpd_response_500(zhttpd_t *httpd)
{
    char output[] = "HTTP/1.0 500 Error\r\n"
        "Server: LIBZC HTTPD\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n"
        "Content-Length: 25\r\n"
        "\r\n"
        "500 Internal Server Error ";
    zstream_write(httpd->fp, output, sizeof(output) - 1);
    zhttpd_response_flush(httpd);
}

void zhttpd_response_header_initialization(zhttpd_t *httpd, const char *initialization)
{
    if (initialization == 0) {
        initialization = "HTTP/1.1 200 LIBZC";
    }
    zstream_puts(httpd->fp, initialization);
    zstream_puts_const(httpd->fp, "\r\n");
    httpd->response_initialization = 1;
}

void zhttpd_response_header(zhttpd_t *httpd, const char *name, const char *value)
{
    if (!httpd->response_initialization) {
        zhttpd_response_header_initialization(httpd, 0);
    }
    zstream_puts(httpd->fp, name);
    zstream_puts_const(httpd->fp, ": ");
    zstream_puts(httpd->fp, value);
    zstream_puts_const(httpd->fp, "\r\n");
    if (!httpd->response_content_type) {
        if (!strcasecmp(name, "Content-Type")) {
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

    httpd->stop = 0;
    httpd->request_content_length = -1;
    zdict_reset(httpd->request_query_vars);
    zdict_reset(httpd->request_post_vars);
    zdict_reset(httpd->request_headers);
    zdict_reset(httpd->request_cookies);

    ZVECTOR_WALK_BEGIN(httpd->request_uploaded_files, zhttpd_uploaded_file_t *, fo) {
        zfree(fo->name);
        zfree(fo->filename);
        zfree(fo);
    } ZVECTOR_WALK_END;
    zvector_reset(httpd->request_uploaded_files);

    if (!zempty(httpd->uploaded_tmp_mime_filename)){
        zunlink(httpd->uploaded_tmp_mime_filename);
        zfree(httpd->uploaded_tmp_mime_filename);
        httpd->uploaded_tmp_mime_filename = 0;
    }

    httpd->unsupported_cmd = 0;
    httpd->request_keep_alive = 0;
    httpd->response_initialization = 0;
    httpd->response_content_type = 0;
}

static void zhttpd_request_header_do(zhttpd_t * httpd, int first, zbuf_t *linebuf)
{
    char *p, *ps;
    int ret, llen;

    /* read first header line */
    if (first) {
        ret = zstream_timed_read_wait(httpd->fp, httpd->request_header_timeout);
    } else {
        ret = zstream_timed_read_wait(httpd->fp, httpd->keep_alive_timeout);
    }
    if (ret < 1) {
        httpd->exception = 1;
        return;
    }
    zstream_set_timeout(httpd->fp, httpd->request_header_timeout);
    if ((zstream_gets(httpd->fp, linebuf, zvar_httpd_header_line_max_size)) < 1) {
        httpd->exception = 1;
        return;
    }
    zbuf_trim_right_rn(linebuf);
    llen = zbuf_len(linebuf);
    httpd->method = zmemdupnull(zbuf_data(linebuf), llen);
    ps = httpd->method;
    p = strchr(ps, ' ');
    if (!p) {
        httpd->exception = 1;
        return;
    }
    *p = 0;
    zstr_toupper(ps);
    if (ZSTR_EQ(ps, "GET")) {
    } else if (ZSTR_EQ(ps, "POST")) {
    } else if (ZSTR_EQ(ps, "HEAD")) {
        httpd->unsupported_cmd = 1;
    } else if (ZSTR_EQ(ps, "PUT")) {
        httpd->unsupported_cmd = 1;
    } else if (ZSTR_EQ(ps, "TRACE")) {
        httpd->unsupported_cmd = 1;
    } else if (ZSTR_EQ(ps, "OPTIONS")) {
        httpd->unsupported_cmd = 1;
    } else {
        httpd->exception = 1;
        return;
    }
    llen -= (p - ps) + 1;
    ps = httpd->uri = p + 1;

    if (llen < 10) {
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
            httpd->exception = 1;
            return;
        }
        if (zbuf_data(linebuf)[zbuf_len(linebuf)-1] != '\n') {
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
    char *data_filename = zstrdup(zbuf_data(linebuf));

    zstream_t *tmp_fp = zstream_open_file(data_filename, "w+");
    if (!tmp_fp) {
        httpd->exception = 1;
        zfree(data_filename);
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
            httpd->exception = 1;
            break;
        }
        zstream_write(tmp_fp, zbuf_data(linebuf), zbuf_len(linebuf));
        left -= rlen;
    }
    if (httpd->exception == 0) {
        if (zstream_flush(tmp_fp) < 0) {
            httpd->exception = 1;
        }
    }
    zstream_close(tmp_fp, 1);
    if (httpd->exception) {
        zunlink(data_filename);
        zfree(data_filename);
        return 0;
    }
    return data_filename;
}

static zbool_t _zhttpd_request_data_do_prepare(zhttpd_t *httpd)
{
    if (httpd->exception || zstream_is_exception(httpd->fp)) {
        return 0;
    }
    if (httpd->request_content_length < 1) {
        return 0;
    }
    if ((httpd->max_length_for_post) > 0 && (httpd->request_content_length > httpd->max_length_for_post)) {
        httpd->exception = 1;
        return 0;
    }

    if (httpd->unsupported_cmd) {
        char output[] = "HTTP/1.0 501 Not implemented\r\n"
            "Server: LIBZC HTTPD\r\n"
            "Connection: close\r\n"
            "Content-Length: 19\r\n"
            "\r\n"
            "501 Not implemented ";
        zstream_write(httpd->fp, output, sizeof(output) - 1);
        zhttpd_response_flush(httpd);
        return 0;
    }

    zstream_set_timeout(httpd->fp, -1);
    return 1;
}

static void _zhttpd_request_data_do_x_www_form_urlencoded(zhttpd_t *httpd, zbuf_t *linebuf)
{
    /* FIXME too long */
    if (zstream_readn(httpd->fp, linebuf, httpd->request_content_length) < httpd->request_content_length) {
        httpd->exception = 1;
        return;
    }
    zurl_query_parse(zbuf_data(linebuf), httpd->request_post_vars);
}

static void _zhttpd_request_data_do_disabled_form_data(zhttpd_t *httpd)
{
    char output[] = "HTTP/1.0 501 Not implemented\r\n"
        "Server: LIBZC HTTPD\r\n"
        "Connection: close\r\n"
        "Content-Length: 95\r\n"
        "\r\n"
        "501 Not implemented, unsupport multipart/form-data<br>(for developers: zhttpd_enable_form_data) ";
    zstream_write(httpd->fp, output, sizeof(output) - 1);
    zhttpd_response_flush(httpd);
}

static void _zhttpd_uploaded_dump_file(zhttpd_t *httpd, zmime_t *mime, zbuf_t *linebuf, zbuf_t *tmpbf, const char *name, const char *filename)
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
    fo->filename = zstrdup(filename);
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
        char *filename = zdict_get_str(params, "filename", "");
        if (zempty(filename)) {
            if (!zempty(name)) {
                zmime_get_decoded_content(mime, tmp_bf);
                zdict_update(httpd->request_post_vars, name, tmp_bf);
            }
        } else {
            _zhttpd_uploaded_dump_file(httpd, mime, linebuf, tmp_bf, name, filename);
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
            char *filename = zdict_get_str(params, "filename", "");
            _zhttpd_uploaded_dump_file(httpd, child, linebuf, tmp_bf, name, filename);
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

    char *data_filename = _zhttpd_request_data_do_save_tmpfile(httpd, linebuf, content_type_raw);
    zmail_t *mime_parser = 0;
    if (data_filename) {
        mime_parser = zmail_create_parser_from_filename(data_filename, "");
    }
    if (!mime_parser) {
        httpd->exception = 1;
        zinfo("error open tmp file %s(%m)", data_filename);
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
    httpd->uploaded_tmp_mime_filename = data_filename;
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

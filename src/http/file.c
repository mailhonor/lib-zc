/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
 * 2017-04-05
 * ================================
 */

#include "httpd.h"

static void _response_416(zhttpd_t *httpd)
{
    zhttpd_show_log(httpd, "416 -");
    char output[] = " 416 Request Range Not Satisfiable\r\n"
        "Server: LIBZC HTTPD\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n"
        "Content-Length: 33\r\n"
        "\r\n"
        "416 Request Range Not Satisfiable";
    zstream_puts(httpd->fp, httpd->version);
    zstream_write(httpd->fp, output, sizeof(output) - 1);
    zhttpd_response_flush(httpd);
}

typedef struct _response_mix_t _response_mix_t;
struct _response_mix_t {
    const char *pathname;
    const char *content_type;
    int max_age;
    zbool_t is_gzip;
    const char *data;
    long size;
    long mtime;
    const char *new_etag;
};

/* 文件是否存在 */
static zbool_t _zhttpd_response_mix(zhttpd_t *httpd, _response_mix_t *ctx, int mix_type)
{
    int ret = 1, fd = -1, do_ragne = 0, max_age = ctx->max_age;
    long rlen_sum, rlen, offset1, offset2;
    struct stat st;
    char *rwdata = 0, *old_etag, *new_etag, *rwline, *range, *p;

    if (mix_type == 'f') {
        while (((fd = open(ctx->pathname, O_RDONLY)) == -1) && (errno == EINTR)) {
            continue;
        }
        if (fd == -1) {
            ret = 0;
            goto over;
        }
        if (fstat(fd, &st) == -1) {
            zhttpd_response_500(httpd);
            goto over;
        }
        if (!S_ISREG(st.st_mode)) {
            zhttpd_response_404(httpd);
            goto over;
        }
        ctx->mtime = st.st_mtime;
        ctx->size = st.st_size;
    } else /* if (mix_type == 'm') */ {
    }

    range = zdict_get_str(httpd->request_headers,"range", "");
    old_etag = zdict_get_str(httpd->request_headers,"if-none-match", "");
    if (*range && *old_etag) {
        _response_416(httpd);
        goto over;
    }

    rwdata = (char *)zmalloc(4096+1);
    if (mix_type == 'f') {
        new_etag = rwdata + 3000;
        sprintf(new_etag, "\"%lx_%lx\"", st.st_size, st.st_mtime);
    } else /* if (mix_type == 'm') */ {
        new_etag = (char *)(ctx->new_etag);
    }
    if (*old_etag) {
        if (!strcmp(old_etag, new_etag)) {
            if (zvar_httpd_no_cache == 0) {
                zhttpd_response_304(httpd, new_etag);
                goto over;
            }
        }
    }

    if (*range && ctx->size && (ctx->is_gzip==0)) {
        for (p = range; *p; p++) {
            if (*p == '=') {
                break;
            }
        }
        if (*p++ != '=') {
            _response_416(httpd);
            goto over;
        }
        if (strchr(p, ',')) {
            _response_416(httpd);
            goto over;
        }
        strncpy(rwdata, p, 128);
        p = strchr(rwdata, '-');
        if (!p){
            _response_416(httpd);
            goto over;
        }
        *p++ = 0;
        if (*rwdata) {
            offset1 = atol(rwdata);
            if (*p) {
                offset2 = atol(p);
            } else {
                offset2 = ctx->size -1;
            }
        } else {
            offset1 = ctx->size - atol(p);
            offset2 = ctx->size -1;
        }
        if ((offset1>offset2) || (offset1<0) || (offset1+1>ctx->size)) {
            _response_416(httpd);
            goto over;
        }
        if (offset2+1 > ctx->size) {
            offset2 = ctx->size - 1;
        }
        do_ragne = 1;
    } else {
        offset1 = 0;
        offset2 = ctx->size - 1;
    }
    
    if (do_ragne) {
        zhttpd_show_log(httpd, "206 %ld/%ld", offset1, offset2-offset1+1);
        zhttpd_response_header_initialization(httpd, 0, "206 Partial Content");
    } else {
        zhttpd_show_log(httpd, "200 %ld", offset2+1);
    }
    zhttpd_response_header_content_type(httpd, ctx->content_type, 0);
    zhttpd_response_header_content_length(httpd, offset2-offset1+1);
    if (zvar_httpd_no_cache == 0) {
        if (*new_etag) {
            zhttpd_response_header(httpd, "Etag", new_etag);
        }
        if (ctx->mtime > 0) {
            zhttpd_response_header_date(httpd, "Last-Modified", ctx->mtime);
        }
        if (max_age == -1) {
            max_age = 3600 * 24 * 10;
        }
        if (max_age > 0) {
            sprintf(rwdata, "max-age=%d", max_age);
            zhttpd_response_header(httpd, "Cache-Control", rwdata);
            zhttpd_response_header_date(httpd, "Expires", max_age + 1 + time(0));
        } else if (max_age == 0) {
            zhttpd_response_header(httpd, "Cache-Control", "no-cache");
        }
    }

    if (ctx->is_gzip) {
        zhttpd_response_header(httpd, "Content-Encoding", "gzip");
    }

    if (do_ragne) {
        sprintf(rwdata, "bytes %ld-%ld/%ld", offset1, offset2, ctx->size);
        zhttpd_response_header(httpd, "Content-Range", rwdata);
    }

    if (httpd->request_keep_alive) {
        zhttpd_response_header(httpd, "Connection", "keep-alive");
    }
    zhttpd_response_header_over(httpd);

    rwline = rwdata;
    rlen_sum = 0;
    int want_rlen_sum = offset2 - offset1 + 1;
    if (mix_type == 'f') {
        if (offset1) {
            if (lseek(fd, offset1, SEEK_SET) == (off_t) -1) {
                zhttpd_set_stop(httpd);
                goto over;
            }
        }
        while(offset1 <= offset2) {
            rlen = offset2 - offset1 + 1;
            if (rlen > 4096) {
                rlen = 4096;
            }
            rlen = read(fd, rwline, rlen);
            if (rlen > 0) {
                rlen_sum += rlen;
                offset1 += rlen;
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
                continue;
            }
            break;
        }
    } else /* if (mix_type == 'm') */ {
        rlen_sum = want_rlen_sum;
        zstream_write(httpd->fp, ctx->data + offset1, want_rlen_sum);
        if (zstream_is_exception(httpd->fp)) {
            rlen_sum = 0;
        }
    }

    zstream_flush(httpd->fp);
    if (rlen_sum != want_rlen_sum) {
        zhttpd_set_stop(httpd);
    } else {
        zstream_flush(httpd->fp);
    }

over:
    if (fd !=-1) {
        close(fd);
    }
    zfree(rwdata);
    return ret;
}

static zbool_t _zhttpd_response_file(zhttpd_t *httpd, const char *pathname, const char *content_type, int max_age, zbool_t is_gzip ) 
{
    _response_mix_t ctx;
    memset(&ctx, 0, sizeof(_response_mix_t));
    ctx.pathname = pathname;
    ctx.content_type = content_type;
    ctx.max_age = max_age;
    ctx.is_gzip = is_gzip;
    return _zhttpd_response_mix(httpd, &ctx, 'f');
}

int zhttpd_response_file_try_gzip(zhttpd_t *httpd, const char *pathname, const char *gzip_pathname, const char *content_type, int max_age)
{
    zbool_t ok = 0;

    if (zempty(pathname) && zempty(gzip_pathname)) {
        return zhttpd_response_500(httpd);
    }
    if (zempty(content_type) && zempty(pathname)) {
        return zhttpd_response_500(httpd);
    }

    if (zempty(content_type)) {
        content_type = zget_mime_type_from_pathname(pathname, zvar_mime_type_application_cotet_stream);
    }

    if ((!zempty(pathname)) && (zempty(gzip_pathname))) {
        ok = _zhttpd_response_file(httpd, pathname, content_type, max_age, 0);
    } else if (zempty(pathname) && (!zempty(gzip_pathname))){
        if (zhttpd_request_is_gzip(httpd) == 0) {
            zhttpd_response_500(httpd);
            ok = 1;
        } else {
            ok = _zhttpd_response_file(httpd, gzip_pathname, content_type, max_age, 1);
        }
    } else if (zhttpd_request_is_gzip(httpd) == 0) {
        ok = _zhttpd_response_file(httpd, pathname, content_type, max_age, 0);
    } else {
        ok = _zhttpd_response_file(httpd, gzip_pathname, content_type, max_age, 1);
        if (ok == 0) {
            ok = _zhttpd_response_file(httpd, pathname, content_type, max_age, 0);
        }
    }

    if (ok == 0) {
        return zhttpd_response_404(httpd);
    }

    return 1;
}

int zhttpd_response_file(zhttpd_t *httpd, const char *pathname, const char *content_type, int max_age)
{
    return zhttpd_response_file_try_gzip(httpd, pathname, 0, content_type, max_age);
}

int zhttpd_response_file_with_gzip(zhttpd_t *httpd, const char *gzip_pathname, const char *content_type, int max_age)
{
    return zhttpd_response_file_try_gzip(httpd, 0, gzip_pathname, content_type, max_age);
}

int zhttpd_response_file_data(zhttpd_t *httpd, const void *data, long size, const char *content_type, int max_age, long mtime, const char *etag, zbool_t is_gzip)
{
    _response_mix_t ctx;
    memset(&ctx, 0, sizeof(_response_mix_t));
    ctx.data = data;
    ctx.size = size;
    ctx.content_type = content_type;
    ctx.max_age = max_age;
    ctx.mtime = mtime;
    ctx.new_etag = (etag?etag:zblank_buffer);
    ctx.is_gzip = is_gzip;
     if (!_zhttpd_response_mix(httpd, &ctx, 'm')) {
        return 0;
     }
     return 1;
}

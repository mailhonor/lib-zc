/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-04-05
 * ================================
 */

#include "httpd.h"

static void _response_416(zhttpd_t *httpd)
{
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

static zbool_t _zhttpd_response_file(zhttpd_t *httpd, const char *filename, const char *content_type, int max_age, zbool_t is_gzip ) 
{
    int ret, fd = -1, do_ragne = 0;
    long rlen_sum, rlen, offset1, offset2;
    struct stat st;
    char *rwdata = 0, *old_etag, *new_etag, *rwline, *range, *p;

    while (((fd = open(filename, O_RDONLY)) == -1) && (errno == EINTR)) {
        continue;
    }
    if (fd == -1) {
        return 0;
    }

    ret = 1;
    if (fstat(fd, &st) == -1) {
        zhttpd_response_500(httpd);
        goto over;
    }

    range = zdict_get_str(httpd->request_headers,"range", "");
    old_etag = zdict_get_str(httpd->request_headers,"if-none-match", "");
    if (*range && *old_etag) {
        _response_416(httpd);
        goto over;
    }

    rwdata = (char *)zmalloc(4096+1);
    new_etag = rwdata + 3000;
    *new_etag = 0;
    if (*old_etag) {
        sprintf(new_etag, "\"%lx_%lx\"", st.st_size, st.st_mtime);
        if (!strcmp(old_etag, new_etag)) {
            if (zvar_httpd_no_cache == 0) {
                zhttpd_response_304(httpd, new_etag);
                goto over;
            }
        }
    }

    if (*range && st.st_size && (is_gzip==0)) {
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
                offset2 = st.st_size -1;
            }
        } else {
            offset1 = st.st_size - atol(p);
            offset2 = st.st_size -1;
        }
        if ((offset1>offset2) || (offset1<0) || (offset1+1>st.st_size)) {
            _response_416(httpd);
            goto over;
        }
        if (offset2+1 > st.st_size) {
            offset2 = st.st_size - 1;
        }
        do_ragne = 1;
    } else {
        offset1 = 0;
        offset2 = st.st_size - 1;
    }
    
    if (do_ragne) {
        zhttpd_response_header_initialization(httpd, 0, "206 Partial Content");
    }
    zhttpd_response_header_content_type(httpd, content_type, 0);
    zhttpd_response_header_content_length(httpd, offset2-offset1+1);
    if (zvar_httpd_no_cache == 0) {
        zhttpd_response_header(httpd, "Etag", new_etag);
        zhttpd_response_header_date(httpd, "Last-Modified", st.st_mtime);
        if (max_age > 0) {
            sprintf(rwdata, "max-age=%d", httpd->response_max_age);
            zhttpd_response_header(httpd, "Cache-Control", rwdata);
            zhttpd_response_header_date(httpd, "Expires", httpd->response_expires + 1 + time(0));
        } else if (max_age < 0) {
            zhttpd_response_header(httpd, "Cache-Control", "no-cache");
        }
    }

    if (is_gzip) {
        zhttpd_response_header(httpd, "Content-Encoding", "gzip");
    }

    if (do_ragne) {
        sprintf(rwdata, "bytes %ld-%ld/%ld", offset1, offset2, st.st_size);
        zhttpd_response_header(httpd, "Content-Range", rwdata);
    }

    if (httpd->request_keep_alive) {
        zhttpd_response_header(httpd, "Connection", "keep-alive");
    }
    zhttpd_response_header_over(httpd);

    rwline = rwdata;
    rlen_sum = 0;
    if (offset1) {
        if (lseek(fd, offset1, SEEK_SET) == (off_t) -1) {
            zhttpd_set_exception(httpd);
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

    if (zvar_proc_stop == 0) {
        zstream_flush(httpd->fp);
        if (rlen_sum != st.st_size) {
            zhttpd_set_stop(httpd);
        } else {
            zstream_flush(httpd->fp);
        }
    }

over:
    if (fd !=-1) {
        close(fd);
    }
    zfree(rwdata);
    return ret;
}

void zhttpd_response_file_try_gzip(zhttpd_t *httpd, const char *filename, const char *gzip_filename, const char *content_type, int max_age)
{
    zbool_t ok = 0;

    if (zempty(filename) && zempty(gzip_filename)) {
        zhttpd_response_500(httpd);
        return;
    }
    if (zempty(content_type) && zempty(filename)) {
        zhttpd_response_500(httpd);
        return;
    }

    if (zempty(content_type)) {
        content_type = zget_mime_type_from_filename(filename, zvar_mime_type_application_cotet_stream);
    }

    if ((!zempty(filename)) && (zempty(gzip_filename))) {
        ok = _zhttpd_response_file(httpd, filename, content_type, max_age, 0);
    } else if (zempty(filename) && (!zempty(gzip_filename))){
        if (zhttpd_request_is_gzip(httpd) == 0) {
            zhttpd_response_500(httpd);
            ok = 1;
        } else {
            ok = _zhttpd_response_file(httpd, gzip_filename, content_type, max_age, 1);
        }
    } else if (zhttpd_request_is_gzip(httpd) == 0) {
        ok = _zhttpd_response_file(httpd, filename, content_type, max_age, 0);
    } else {
        ok = _zhttpd_response_file(httpd, gzip_filename, content_type, max_age, 1);
        if (ok == 0) {
            ok = _zhttpd_response_file(httpd, filename, content_type, max_age, 0);
        }
    }

    if (ok == 0) {
        zhttpd_response_404(httpd);
    }

    return;
}

void zhttpd_response_file(zhttpd_t *httpd, const char *filename, const char *content_type, int max_age)
{
    zhttpd_response_file_try_gzip(httpd, filename, 0, content_type, max_age);
}

void zhttpd_response_file_with_gzip(zhttpd_t *httpd, const char *gzip_filename, const char *content_type, int max_age)
{
    zhttpd_response_file_try_gzip(httpd, 0, gzip_filename, content_type, max_age);
}

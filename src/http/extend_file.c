/*
 * ================================
 * eli960@qq.com
 * http://www.mailhonor.com/
 * 2017-04-05
 * ================================
 */

#include "httpd.h"

static zbool_t _zhttpd_response_file(zhttpd_t *httpd, const char *filename, const char *content_type, int max_age, zbool_t is_gzip ) 
{
    int fd, rlen;
    long rlen_sum;
    struct stat st;
    char *rwdata, *old_etag, *new_etag, *rwline;

    while (((fd = open(filename, O_RDONLY)) == -1) && (errno == EINTR)) {
        continue;
    }
    if (fd == -1) {
        return 0;
    }

    if (fstat(fd, &st) == -1) {
        zclose(fd);
        zhttpd_response_500(httpd);
        return 1;
    }

    rwdata = (char *)zmalloc(4096+1);

    old_etag = zdict_get_str(httpd->request_headers,"if-none-match", "");
    new_etag = rwdata;
    sprintf(new_etag, "%lx_%lx", st.st_size, st.st_mtime);
    if (!strcmp(old_etag, new_etag)) {
        if (zvar_httpd_no_cache == 0) {
            zhttpd_response_304(httpd, new_etag);
            zclose(fd);
            zfree(rwdata);
            return 1;
        }
    }

    zhttpd_response_header_content_type(httpd, content_type, 0);
    zhttpd_response_header_content_length(httpd, st.st_size);
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

    if (httpd->request_keep_alive) {
        zhttpd_response_header(httpd, "Connection", "keep-alive");
    }
    zhttpd_response_header_over(httpd);

    rwline = rwdata;
    rlen_sum = 0;
    while(rlen_sum < st.st_size) {
        rlen = st.st_size - rlen_sum;
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
    zfree(rwdata);

    if (zvar_proc_stop == 0) {
        zstream_flush(httpd->fp);
        if (rlen_sum != st.st_size) {
            zhttpd_set_stop(httpd);
        } else {
            zstream_flush(httpd->fp);
        }
    }

    return 1;
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

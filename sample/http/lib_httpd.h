/*
 * ================================
 * eli960@qq.com
 * https://blog.csdn.net/eli960
 * 2019-11-11
 * ================================
 */

#ifdef ___INNER_USE_SSL___
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif

#include "zc.h"
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <signal.h>

#ifdef ___INNER_USE_SSL___
static int is_ssl = 0;
SSL_CTX *var_openssl_server_ctx;
#endif

typedef void (*cmd_fn_t)(zhttpd_t * httpd);

static void usage()
{
#ifdef ___INNER_USE_SSL___
    printf("USAGE: %s -listen 0:8899 [ --ssl [ -cert ./ssl.cert, -key ./ssl.key ] ]\n", zvar_progname);
#else
    printf("USAGE: %s -listen 0:8899\n", zvar_progname);
#endif
    exit(1);
}

static void main_page(zhttpd_t *httpd)
{
    zhttpd_response_file_try_gzip(httpd, "resource_httpd/main.html", "resource_httpd/main.html.gz", "text/html", 0);
}

static zbuf_t *explore_data_1, *explore_data_2;
static void explore_data_init()
{
    zbuf_t *tmpbf = zbuf_create(-1);
    zfile_get_contents_sample("resource_httpd/explore.html", tmpbf);
    char *ps = zbuf_data(tmpbf), *p = strstr(ps, "___DISPLAY_DATA___");
    if (p) {
        *p = 0;
        p += 19;
    } else {
        p = zblank_buffer;
    }
    explore_data_1 = zbuf_create(strlen(ps));
    zbuf_strcpy(explore_data_1, ps);
    explore_data_2 = zbuf_create(strlen(p));
    zbuf_strcpy(explore_data_2, p);
    zbuf_free(tmpbf);
}

static void explore_data_fini()
{
    zbuf_free(explore_data_1);
    zbuf_free(explore_data_2);
}

static void explore_page(zhttpd_t *httpd)
{
    char uri[4096+1];
    struct stat st;
    strncpy(uri, zhttpd_request_get_uri(httpd) + 8, 4096);
    char *p = strchr(uri, '?');
    if (p) {
        *p = 0;
    }

    if (stat(uri, &st) < 0) {
        zhttpd_response_500(httpd);
        return;
    }
    if (S_ISREG(st.st_mode)) {
        if (strstr(uri, "..")) {
            zhttpd_response_404(httpd);
            return;
        }
        zhttpd_response_file(httpd, uri, 0, 0);
    } else if (S_ISDIR(st.st_mode)) {
        DIR *dir = opendir(uri);
        if (!dir) {
            zhttpd_response_404(httpd);
            return;
        }
        zjson_t *js = zjson_create();
        struct dirent *ent;
        while((ent = readdir(dir))) {
            char *name = ent->d_name;
            if ((!strcmp(name, ".")) || (!strcmp(name, ".."))) {
                continue;
            }
            zjson_t *o = zjson_create();
            zjson_object_add(o, "name", zjson_create_string(name, -1), 0);
            const char *type = "unknown";
            unsigned char d_type = ent->d_type;
            if (d_type == DT_BLK) {
                type = "block device";
            } else if (d_type == DT_CHR) {
                type = "character device";
            } else if (d_type == DT_DIR) {
                type = "directory";
            } else if (d_type == DT_FIFO) {
                type = "fifo";
            } else if (d_type == DT_LNK) {
                type = "symbolic link";
            } else if (d_type == DT_REG) {
                type = "regular file";
            } else if (d_type == DT_SOCK) {
                type = "domain socket";
            }
            zjson_object_add(o, "type", zjson_create_string(type, -1), 0);
            zjson_array_push(js, o);
        }
        closedir(dir);
        zbuf_t *bf = zbuf_create(-1);
        zbuf_append(bf, explore_data_1);
        zjson_serialize(js, bf, 0);
        zbuf_append(bf, explore_data_2);
        zjson_free(js);
        zhttpd_response_200(httpd, zbuf_data(bf), zbuf_len(bf));
        zbuf_free(bf);
    } else {
        zhttpd_response_404(httpd);
    }
}

static void upload_page(zhttpd_t *httpd)
{
    zhttpd_response_file(httpd, "resource_httpd/upload_page.html", "text/html", 0);
}

static zjson_t *___dict_to_json(const zdict_t *dict)
{
    zjson_t *js = zjson_create();
    ZDICT_WALK_BEGIN(dict, k, v) {
        zjson_object_update(js, k, zjson_create_string(zbuf_data(v), zbuf_len(v)), 0);
    } ZDICT_WALK_END;
    return js;
}

static void upload_do(zhttpd_t *httpd)
{
    zjson_t *js = zjson_create();
    zjson_object_update(js, "headers", ___dict_to_json(zhttpd_request_get_headers(httpd)), 0);
    zjson_object_update(js, "cookies", ___dict_to_json(zhttpd_request_get_cookies(httpd)), 0);
    zjson_object_update(js, "query_vars", ___dict_to_json(zhttpd_request_get_query_vars(httpd)), 0);
    zjson_object_update(js, "post_vars", ___dict_to_json(zhttpd_request_get_post_vars(httpd)), 0);

    zjson_t *files_js = zjson_object_update(js, "files", zjson_create(), 0);
    const zvector_t *files = zhttpd_request_get_uploaded_files(httpd);
    int file_id = 0;
    ZVECTOR_WALK_BEGIN(files, zhttpd_uploaded_file_t *, fo) {
        zjson_t *tmpjs = zjson_create();
        zjson_object_update(tmpjs, "name", zjson_create_string(zhttpd_uploaded_file_get_name(fo), -1), 0);
        zjson_object_update(tmpjs, "pathname", zjson_create_string(zhttpd_uploaded_file_get_pathname(fo), -1), 0);
        zjson_object_update(tmpjs, "size", zjson_create_long(zhttpd_uploaded_file_get_size(fo)), 0);
        char saved_pathname[1024];
        sprintf(saved_pathname, "uploaded_files/%d.dat", file_id++);
        zjson_object_update(tmpjs, "saved_pathname", zjson_create_string(saved_pathname, -1), 0);
        zjson_array_push(files_js, tmpjs);
        zhttpd_uploaded_file_save_to(fo, saved_pathname);
    } ZVECTOR_WALK_END;

    zbuf_t *bf = zbuf_create(-1);

    zbuf_strcpy(bf, "<!DOCTYPE html><html xmlns=\"http://www.w3.org/1999/xhtml\"><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"><title>LIBZC HTTPD UPLOAD RESULT</title><script charset='UTF-8'>var data=");
    zjson_serialize(js, bf, 0);
    zbuf_strcat(bf, ";console.log(data);</script></head><body>SEE console.log</body></html>");
    zhttpd_response_200(httpd, zbuf_data(bf), zbuf_len(bf));
    zbuf_free(bf);
    zjson_free(js);
}

static cmd_fn_t get_cmd_fn(zhttpd_t *httpd)
{
    const char *uri = zhttpd_request_get_uri(httpd);
    if (*uri++ != '/') {
        return 0;
    }
    if (*uri == 0) {
        return main_page;
    }
    if (!strncmp(uri, "explore/", 8)) {
        return explore_page;
    }
    if (!strcmp(uri, "upload_page")) {
        return upload_page;
    }
    if (!strcmp(uri, "upload_do")) {
        return upload_do;
    }
    return 0;
}

static void httpd_handler(zhttpd_t *httpd)
{
    cmd_fn_t cmd = get_cmd_fn(httpd);
    if (cmd) {
        cmd(httpd);
    } else {
        zhttpd_response_404(httpd);
    }
}

typedef union {
    void *ptr;
    struct {
        int fd;
        unsigned int sock_type:8;
        unsigned int is_ssl:1;
    } sockinfo;
} long_info_t;

static void *do_httpd(void *arg)
{
    long_info_t linfo;
    linfo.ptr = arg;

    int fd = linfo.sockinfo.fd;

    zhttpd_t *httpd;
#ifdef ___INNER_USE_SSL___
    if (is_ssl) {
        SSL *ssl = zopenssl_SSL_create(var_openssl_server_ctx, fd);

        if (zopenssl_timed_accept(ssl, 10, 10) < 1) {
            int ip = 0, port = 0;
            char ipstr[18];
            if (zget_peername(fd, &ip, &port) > 0)  {
                zget_ipstring(ip, ipstr);
            } else {
                ipstr[0] = '0';
                ipstr[1] = 0;
            }
            char error_buf[1024];
            zopenssl_get_error(0, error_buf, 1024);
            zinfo("SSL handshake error with %s:%d, %s", ipstr, port, error_buf);
            zopenssl_SSL_free(ssl);
            return 0; 
        }
        httpd = zhttpd_open_ssl(ssl);
    } else
#endif
    {
        httpd = zhttpd_open_fd(fd);
    }

    zhttpd_set_handler(httpd, httpd_handler);
    zhttpd_enable_form_data(httpd);
    zhttpd_run(httpd);
    zhttpd_close(httpd, 1);

    return 0;
}

static void load_ssl()
{
#ifdef ___INNER_USE_SSL___
    is_ssl = zconfig_get_bool(zvar_default_config, "ssl", 0);
    if (!is_ssl) {
        return;
    }

    zopenssl_init();
    char *cert = zconfig_get_str(zvar_default_config, "cert", "ssl.cert");
    char *key = zconfig_get_str(zvar_default_config, "key", "ssl.key");
    var_openssl_server_ctx = zopenssl_SSL_CTX_create_server(cert, key);
    if (!var_openssl_server_ctx) {
        char error_buf[1024];
        zopenssl_get_error(0, error_buf, 1024);
        printf("ERR can load cert/key:%s", error_buf);
        usage();
    } 
#endif
}


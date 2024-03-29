/*
 * ================================
 * eli960@qq.com
 * http:/linuxmail.cn/
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
    char *pathname;
    int encoding;
    int raw_len;
    int offset;
    int size;
};

static const int zvar_httpd_header_line_max_size = 10240;

struct zhttpd_t {
    zstream_t *fp;
    zbuf_t *prefix_log_msg;
    char *method;
    char *host;
    char *uri;
    char *version;
    char *path;
    int port;
    ssize_t request_content_length;
    zdict_t *request_query_vars;
    zdict_t *request_post_vars;
    zdict_t *request_headers;
    zdict_t *request_cookies;
    zvector_t *request_uploaded_files; /* <zhttpd_uploaded_file_t *> */
    char *uploaded_tmp_mime_pathname;
    /* */
    int version_code:3;
    unsigned int request_gzip:2;
    unsigned int request_deflate:2;
    unsigned int stop:1;
    unsigned int first:1;
    unsigned int exception:1;
    unsigned int unsupported_cmd:1;
    unsigned int request_keep_alive:1;
    unsigned int response_initialization:1;
    unsigned int response_content_type:1;
    unsigned int ssl_mode:1;
    unsigned int enable_form_data:1;
    /* */
    int keep_alive_timeout;
    int read_wait_timeout;
    int write_wait_timeout;
    int max_length_for_post;
    char *tmp_path_for_post;
    char gzip_file_suffix[8];
    void *context;
};

zhttpd_t * _zhttpd_malloc_struct_general();

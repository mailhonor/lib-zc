## http工具和服务

源码见 src/http/

---

```
typedef struct zurl_t zurl_t;
typedef struct zhttpd_uploaded_file_t zhttpd_uploaded_file_t;
typedef struct zhttpd_t zhttpd_t;
```

---


```
/* url 解析 */
struct zurl_t {
    char *scheme;
    char *destination;
    char *host;
    char *path;
    char *query;
    char *fragment;
    int port;
};

/* 解析url字符换 */
zurl_t *zurl_parse(const char *url_string);
void zurl_free(zurl_t *url);

/* debug输出 */
void zurl_debug_show(zurl_t *url);

/* 解析query部分, 保存在query_vars, 并返回; 如果 query_vars==0, 则创建 */
zdict_t *zurl_query_parse(const char *query, zdict_t *query_vars);

/* 从词典query_vars, 组成query字符串保存在query_result; strict_flag: 是否严格编码 */
char *zurl_query_build(const zdict_t *query_vars, zbuf_t *query_result, zbool_t strict);
```

---


```
/* 解析cookie字符串, 保存在cookies, 并返回; 如果 cookies == 0, 则创建 */
zdict_t *zhttp_cookie_parse(const char *raw_cookie, zdict_t *cookies);

/* 生成一个cookie条目, 保存在 cookie_result */
/* name: 名称; */
/* value: 值; 为空则删除 */
/* expires: 设置过期时间, >0 生效 */
/* path: cookie 路径, 为空则忽略 */
/* domain: 作用域, 为空则忽略 */
/* secure: 是否安全 */
/* httponly: 是否httponly */
char *zhttp_cookie_build_item(const char *name, const char *value, long expires, const char *path, const char *domain, zbool_t secure, zbool_t httponly, zbuf_t *cookie_result);
```

---

```
extern zbool_t zvar_httpd_debug;
extern zbool_t zvar_httpd_no_cache;

/* 从fd, 或ssl 创建http对象 */
zhttpd_t *zhttpd_open_fd(int sock);
zhttpd_t *zhttpd_open_ssl(SSL *ssl);
void zhttpd_close(zhttpd_t *httpd, zbool_t close_fd_and_release_ssl);

/* 执行 */
void zhttpd_run(zhttpd_t *httpd);

/* 设置handler */
void zhttpd_set_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd));
void zhttpd_set_HEAD_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd));
void zhttpd_set_OPTIONS_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd));
void zhttpd_set_DELETE_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd));
void zhttpd_set_TRACE_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd));
void zhttpd_set_PATCH_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd));

/* 设置/获取上下文 */
void zhttpd_set_context(zhttpd_t *httpd, const void *context);
void *zhttpd_get_context(zhttpd_t *httpd);

/* 设httpd出现异常 */
void zhttpd_set_exception(zhttpd_t *httpd);

/* 停止httpd */
void zhttpd_set_stop(zhttpd_t *httpd);

/* 设置keep-alive时间, timeout: 秒 */
void zhttpd_set_keep_alive_timeout(zhttpd_t *httpd, int timeout);

/* 获取http协议头超时时间 */
void zhttpd_set_request_header_timeout(zhttpd_t *httpd, int timeout);

/* 通用超时等待可读, 单位秒, timeout<0: 表示无限长 */
void zhttpd_set_read_wait_timeout(zhttpd_t *httpd, int read_wait_timeout);

/* 通用超时等待可写, 单位秒 */
void zhttpd_set_write_wait_timeout(zhttpd_t *httpd, int write_wait_timeout);

/* 设置post方式最大长度 */
void zhttpd_set_max_length_for_post(zhttpd_t *httpd, int max_length);

/* 设置post方式临时目录 */
void zhttpd_set_tmp_path_for_post(zhttpd_t *httpd, const char *tmp_path);

/* 设置支持form_data协议, 默认不支持 */
void zhttpd_enable_form_data(zhttpd_t *httpd);

/* 请求的方法: GET/POST/... */
const char *zhttpd_request_get_method(zhttpd_t *httpd);

/* 请求的主机名 */
const char *zhttpd_request_get_host(zhttpd_t *httpd);

/* 请求的url路径 */
const char *zhttpd_request_get_path(zhttpd_t *httpd);

/* 请求的URI */
const char *zhttpd_request_get_uri(zhttpd_t *httpd);

/* http版本 */
const char *zhttpd_request_get_version(zhttpd_t *httpd);


/* 0: 1.0;  1: 1.1 */
int zhttpd_request_get_version_code(zhttpd_t *httpd);

/* 分析http头Content-Lengtgh字段 */
long zhttpd_request_get_content_length(zhttpd_t *httpd);

/* 请求是否是 gzip/deflate */
zbool_t zhttpd_request_is_gzip(zhttpd_t *httpd);
zbool_t zhttpd_request_is_deflate(zhttpd_t *httpd);

/* 全部http头 */
const zdict_t *zhttpd_request_get_headers(zhttpd_t *httpd);

/* url queries, 解析后 */
const zdict_t *zhttpd_request_get_query_vars(zhttpd_t *httpd);

/* post queries, 解析后 */
const zdict_t *zhttpd_request_get_post_vars(zhttpd_t *httpd);

/* cookies, 解析后 */
const zdict_t *zhttpd_request_get_cookies(zhttpd_t *httpd);

/* 获取全部上传文件的信息 */
const zvector_t *zhttpd_request_get_uploaded_files(zhttpd_t *httpd); /* zhttpd_uploaded_file * */

/* 快速回复 */
void zhttpd_response_200(zhttpd_t *httpd, const char *data, int size);
void zhttpd_response_304(zhttpd_t *httpd, const char *etag);
void zhttpd_response_404(zhttpd_t *httpd);
void zhttpd_response_500(zhttpd_t *httpd);
void zhttpd_response_501(zhttpd_t *httpd);

/* 设置 默认handler */
void zhttpd_set_200_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd, const char *data, int size));
void zhttpd_set_304_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd, const char *etag));
void zhttpd_set_404_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd));
void zhttpd_set_500_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd));
void zhttpd_set_501_handler(zhttpd_t *httpd, void (*handler)(zhttpd_t * httpd));

/* 输出 initialization; version: "http/1.0", "http/1.1", 0: 采用请求值; status: 如 "200 XXX"  */
void zhttpd_response_header_initialization(zhttpd_t *httpd, const char *version, const char *status);

/* 输出header */
void zhttpd_response_header(zhttpd_t *httpd, const char *name, const char *value);

/* 输出header, value为date类型 */
void zhttpd_response_header_date(zhttpd_t *httpd, const char *name, long value);

/* 输出Content-Type; charset: 为空则取 UTF-8 */
void zhttpd_response_header_content_type(zhttpd_t *httpd, const char *value, const char *charset);

/* 输出http体长度 */
void zhttpd_response_header_content_length(zhttpd_t *httpd, long length);

/* 设置cookie */
void zhttpd_response_header_set_cookie(zhttpd_t *httpd, const char *name, const char *value, long expires, const char *path, const char *domain, zbool_t secure, zbool_t httponly);

/* 删除cookie */
void zhttpd_response_header_unset_cookie(zhttpd_t *httpd, const char *name);

/* http头输出完毕 */
void zhttpd_response_header_over(zhttpd_t *httpd);

/* 写http体 */
void zhttpd_response_write(zhttpd_t *httpd, const void *data, int len);
void zhttpd_response_puts(zhttpd_t *httpd, const char *data);
void zhttpd_response_append(zhttpd_t *httpd, const zbuf_t *bf);
void zhttpd_response_printf_1024(zhttpd_t *httpd, const char *format, ...);
void zhttpd_response_flush(zhttpd_t *httpd);

/* 获取http的stream */
zstream_t *zhttpd_get_stream(zhttpd_t *httpd);

/* 上传文件路径名 */
const char *zhttpd_uploaded_file_get_pathname(zhttpd_uploaded_file_t *fo);

/* 上传文件名称 */
const char *zhttpd_uploaded_file_get_name(zhttpd_uploaded_file_t *fo);

/* 上传文件大小; -1: 错 */
int zhttpd_uploaded_file_get_size(zhttpd_uploaded_file_t *fo);

/* 保存上传文件到指定文件路径 */
int zhttpd_uploaded_file_save_to(zhttpd_uploaded_file_t *fo, const char *pathname);

/* 获取上传文件数据 */
int zhttpd_uploaded_file_get_data(zhttpd_uploaded_file_t *fo, zbuf_t *data);

/* 输出一个文件 */
void zhttpd_response_file(zhttpd_t *httpd, const char *pathname, const char *content_type, int max_age);

/* 输出一个文件, 带gzip */
void zhttpd_response_file_with_gzip(zhttpd_t *httpd, const char *gzip_pathname, const char *content_type, int max_age);

/* 输出一个文件 */
void zhttpd_response_file_try_gzip(zhttpd_t *httpd, const char *pathname, const char *gzip_pathname, const char *content_type, int max_age);
```

---

### 例子
见源码 sample/http/
